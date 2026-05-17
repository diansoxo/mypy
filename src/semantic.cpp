#include "semantic.hpp"
#include <algorithm>
#include <unordered_set>

namespace semantic {
SemanticAnalyzer::SemanticAnalyzer(const std::string& filename)
    : filename_(filename) //запоминаем имя файла
{}

void SemanticAnalyzer::pushScope() {
    scopes_.emplace_back(); // добавляем новый пустой словарь в конец вектора
}

void SemanticAnalyzer::popScope() {
    if (!scopes_.empty())
        scopes_.pop_back(); // удаляем верхний словарь из вектора
}

bool SemanticAnalyzer::declareVar(const std::string& name, VarInfo info) {//проверяет может ли быть объявлена переменная(аписывает новую переменную в текущий блок)
    if (scopes_.empty()) return false; // пустой быть не должен всегда есть pushScope

    auto& top = scopes_.back(); // верхний словарь текущий блок back() возвращает последний элемент вектора чтобы изменения попали в реальный словарь
    if (top.count(name)) return false; // count() возвращает 1 если ключ есть, 0 если нет. Если переменная уже есть в этом блоке возвр false
    top[name] = std::move(info); // записываем в словарь
    return true;
}

const VarInfo* SemanticAnalyzer::lookupVar(const std::string& name) const {//ищет переменную по имени
    for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {// идём с верхнего листа вниз
        auto it = scopes_[i].find(name); //ищем на этом листе
        if (it != scopes_[i].end()) // нашли?
            return &it->second;// возвращаем адрес VarInfo
    }
    return nullptr; //не нашли
}

//диагностика 
void SemanticAnalyzer::error(int line, int col, const std::string& msg) {
    diagnostics_.push_back(Diagnostic{ filename_, line, col, msg });
}

//analyse
AnalysisResult SemanticAnalyzer::analyze(const parser::Program& program) {
    // первый проход: собираем все объявления верхнего уровня.
    // нужно чтобы функции могли вызывать друг друга
    // независимо от порядка в файле.
    for (auto& d : program.decls)
        collectDecl(*d);

    // второй проход: проверяем тела функций
    for (auto& d : program.decls)
        checkDecl(*d);

    // упаковываем все найденные ошибки в результат и возвращаем
    return AnalysisResult{ std::move(diagnostics_) };
}

void SemanticAnalyzer::collectDecl(const parser::Decl& decl) {//только записываем имена

    if (auto* fd = dynamic_cast<const parser::FuncDef*>(&decl)) {
        // проверяем что функция с таким именем ещё не объявлена
        if (functions_.count(fd->name)) {
            error(fd->pos.line, fd->pos.col,
                  "функция '" + fd->name + "' уже объявлена");
            return;
        }
        // собираем сигнатуру- только типы параметров и тип возврата
        FuncInfo info;
        for (auto& p : fd->params)
            info.param_types.push_back(p.type_name);
        info.return_type = fd->return_type.empty() ? "void" : fd->return_type;
        // записываем в глобальную таблицу функций
        functions_[fd->name] = std::move(info);
        return;
    }

    if (auto* sd = dynamic_cast<const parser::StructDecl*>(&decl)) {
        if (structs_.count(sd->name)) {
            error(sd->pos.line, sd->pos.col,
                  "структура '" + sd->name + "' уже объявлена");
            return;
        }
        StructInfo info;
        for (auto& f : sd->fields)
            info.fields[f.name] = f.type_name; // имя поля -тип поля
        structs_[sd->name] = std::move(info);
        return;
    }

    if (auto* ed = dynamic_cast<const parser::EnumDecl*>(&decl)) {
        if (enums_.count(ed->name)) {
            error(ed->pos.line, ed->pos.col,
                  "перечисление '" + ed->name + "' уже объявлено");
            return;
        }
        EnumInfo info;
        for (auto& v : ed->variants)
            info.variants.push_back(v.name);
        enums_[ed->name] = std::move(info);
        return;
    }

    if (auto* ta = dynamic_cast<const parser::TypeAlias*>(&decl)) {// просто запоминаем: "Meters" -"int32"
        type_aliases_[ta->name] = ta->type_name;
        return;
    }
    
    if (auto* nd = dynamic_cast<const parser::NamespaceDecl*>(&decl)) {
        for (auto& d : nd->decls)
            collectDecl(*d); //рекурсивно собираем функции внутри
        return;
    }
    
    if (auto* id = dynamic_cast<const parser::ImplDecl*>(&decl)) {
        for (auto& m : id->methods)
            collectDecl(*m); // рекурсивно собираем методы внутри
        return;
    }
}

void SemanticAnalyzer::checkDecl(const parser::Decl& decl) {// будет проверять содержимое( см на тип узла и вызываем нужную функцию)
    if (auto* fd = dynamic_cast<const parser::FuncDef*>(&decl))
        return checkFuncDef(*fd);
    if (auto* sd = dynamic_cast<const parser::StructDecl*>(&decl))
        return checkStructDecl(*sd);
    if (auto* ed = dynamic_cast<const parser::EnumDecl*>(&decl))
        return checkEnumDecl(*ed);
    if (auto* id = dynamic_cast<const parser::ImplDecl*>(&decl))
        return checkImplDecl(*id);
    if (auto* nd = dynamic_cast<const parser::NamespaceDecl*>(&decl))
        return checkNamespaceDecl(*nd);
}
std::string SemanticAnalyzer::resolveAlias(const std::string& type_name) const {
    std::string cur = type_name;
    int limit = 32; // защита от type A = B, type B = A -бесконечной цепочки
    while (type_aliases_.count(cur) && limit-- > 0)
        cur = type_aliases_.at(cur);
    return cur;
}

bool SemanticAnalyzer::isKnownType(const std::string& type_name) const {//существует ли этот тип вообще
    std::string r = resolveAlias(type_name);
    static const std::unordered_set<std::string> builtins = {
        "int8","int16","int32","int64", 
        "uint8","uint16","uint32","uint64",
        "float32","float64","bool","string","void","char"
    };//Тип считается известным если он встроенный , или объявленная структура, enum, или псевдоним
    return builtins.count(r) || structs_.count(r) || enums_.count(r) || type_aliases_.count(type_name);
}

bool SemanticAnalyzer::typesCompatible(const std::string& a, const std::string& b) const {
    if (a.empty() || b.empty()) return true; // пустая строка = ошибка уже записана, не дублируем
    return resolveAlias(a) == resolveAlias(b); //раскрывает оба типа через псевдонимы и сравнивает
}
bool SemanticAnalyzer::isNumericType(const std::string& t) const {
    std::string r = resolveAlias(t);
    return r=="int8"||r=="int16"||r=="int32"||r=="int64" ||r=="uint8"||r=="uint16"||r=="uint32"||r=="uint64" ||r=="float32"||r=="float64"; //для арифметики чтобы a + b оба числа
}
bool SemanticAnalyzer::isIntegerType(const std::string& t) const {
    std::string r = resolveAlias(t);
    return r=="int8"||r=="int16"||r=="int32"||r=="int64" ||r=="uint8"||r=="uint16"||r=="uint32"||r=="uint64";//нужна для индексов массивов arr[3.14] ошибка, arr[3] надо
}

}
