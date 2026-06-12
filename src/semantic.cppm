module;
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <unordered_set>
export module semantic;
import ast;
import parser;
export namespace semantic {
using Diagnostic = parser::Diagnostic;

struct VarInfo {
    std::string type_name;
    bool is_mut;
    int line;
    int col;
};

struct EnumInfo {
    std::vector<std::string> variants; // список имён вариантов
};

struct FuncInfo {
    std::vector<std::string> param_types;
    std::string return_type;
};

struct StructInfo {
    std::unordered_map<std::string, std::string> fields; // словарь: имя поля-тип поля, важно быстро найти поле по имени, поэтому unordered_map
};

struct AnalysisResult {
    std::vector<Diagnostic> errors;//все ошибки
    bool ok() const {
        return errors.empty(); //смотрим пустой или нет список ошибок
    }
};

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(const std::string& filename);
    AnalysisResult analyze(const parser::Program& program);// Принимает готовую программу из парсера возвращает результат

private:
    std::string filename_;//имя файла для диагностики
    std::vector<Diagnostic> diagnostics_;//список всех ошибок найденных
    std::vector<std::unordered_map<std::string, VarInfo>> scopes_;//какие переменные сейчас видны?
    std::unordered_map<std::string, FuncInfo> functions_;
    std::unordered_map<std::string, StructInfo> structs_;//какие структуры существуют?
    std::unordered_map<std::string, EnumInfo> enums_;//какие enum существуют?
    std::unordered_map<std::string, std::string> type_aliases_;//какие псевдонимы типов существуют?
    std::string current_return_type_;//какой тип должен вернуть текущий return?
    int loop_depth_ = 0;//можно ли писать break/continue прямо сейчас?

//области видимости
    void pushScope();// создать новую область видимости при входе в блок { }
    void popScope();// удалить область видимости при выходе из блока { }
    bool declareVar(const std::string& name, VarInfo info);// объявить новую переменную в текущем блоке возвращает false если имя уже занято в этом же блоке — это ошибка
    const VarInfo* lookupVar(const std::string& name) const;// найти переменную по имени — ищем снизу вверх по стеку(nullptr если переменная не найдена нигде)

//диагностика 
    void error(int line, int col, const std::string& msg);
//первый проход
    void collectDecl(const parser::Decl& decl);

//проверка объявлений 
    void checkDecl(const parser::Decl& decl);

    void checkFuncDef(const parser::FuncDef& fd);
    void checkStructDecl(const parser::StructDecl& sd);
    void checkEnumDecl(const parser::EnumDecl& ed);
    void checkImplDecl(const parser::ImplDecl& id);
    void checkNamespaceDecl(const parser::NamespaceDecl& nd);

    // проверка инструкций
    void checkBlock(const parser::Block& block);
    void checkStmt(const parser::Stmt& stmt);

    void checkVarDecl(const parser::VarDecl& vd);
    void checkAssign(const parser::Assign& as);
    void checkReturn(const parser::Return& ret);
    void checkIf(const parser::If& node);
    void checkWhile(const parser::While& node);
    void checkFor(const parser::For& node);
    void checkMatch(const parser::Match& node);

//проверка выражений
    std::string checkExpr(const parser::Expr& expr);
    std::string checkBinaryOp(const parser::BinaryOp& node);
    std::string checkUnaryOp(const parser::UnaryOp& node);
    std::string checkCall(const parser::Call& node);
    std::string checkArrayAccess(const parser::ArrayAccess& node);
    std::string checkFieldAccess(const parser::FieldAccess& node);
    std::string checkCast(const parser::Cast& node);
    std::string checkStructLiteral(const parser::StructLiteral& node);
    std::string checkArrayLiteral(const parser::ArrayLiteral& node);
    std::string checkLValue(const parser::Expr& target, bool check_mut);//изм2

//вспомогательные проверки типов
    std::string resolveAlias(const std::string& type_name) const;// разворачивает псевдонимы: "///" → "int32"
    bool isNumericType(const std::string& t) const;// является ли тип числовым (int*, uint*, float*)
    bool isIntegerType(const std::string& t) const;// является ли тип целочисленным (int*, uint*) — нужно для индексов массивов
    bool typesCompatible(const std::string& a, const std::string& b) const;// совместимы ли два типа (с учётом псевдонимов)
    bool isKnownType(const std::string& type_name) const;// объявлен ли такой тип вообще (встроенный, структура, enum, псевдоним)
    std::string checkBuiltin(const std::string& name, const std::vector<parser::ExprPtr>& args);// проверка встроенных функций: print, input, exit, panic
};


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

    {
        auto it = functions_.find("main");//изм2
        if (it == functions_.end()) {
            diagnostics_.push_back(Diagnostic{filename_, 0, 0,
                "отсутствует функция 'main'"});
        } else {
            auto& info = it->second;
            if (info.return_type != "int32" && info.return_type != "int64")
                diagnostics_.push_back(Diagnostic{filename_, 0, 0,
                    "функция 'main' должна возвращать int32 или int64"});
            if (!info.param_types.empty())
                diagnostics_.push_back(Diagnostic{filename_, 0, 0,
                    "функция 'main' не должна принимать параметры"});
            }
        }


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
        for (auto& d : nd->decls){
            if (auto* fd = dynamic_cast<const parser::FuncDef*>(d.get())) {//изм3
                std::string full_name = nd->name + "." + fd->name;
                if (functions_.count(full_name)) {
                    error(fd->pos.line, fd->pos.col,
                        "функция '" + full_name + "' уже объявлена");
                    continue;
                }
                FuncInfo info;//изм3
                for (auto& p : fd->params)
                    info.param_types.push_back(p.type_name);
                info.return_type = fd->return_type.empty() ? "void" : fd->return_type;
                functions_[full_name] = std::move(info);
            }
        }
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
    if (!r.empty() && r.front() == '[')// массив: начинается с '[' изм2
        return true;

    if (!r.empty() && r.front() == '(')// кортеж: начинается с '('
        return true;

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
void SemanticAnalyzer::checkFuncDef(const parser::FuncDef& fd) {
    // проверяем что типы параметров существуют
    for (auto& p : fd.params)
        if (!isKnownType(p.type_name))
            error(fd.pos.line, fd.pos.col, "параметр '" + p.name + "': неизвестный тип '" + p.type_name + "'");
    std::string ret = fd.return_type.empty() ? "void" : fd.return_type;// нормализуем return type: пустая строка = void
    if (!isKnownType(ret))
        error(fd.pos.line, fd.pos.col, "неизвестный тип возврата '" + ret + "'");
    // сохраняем предыдущий return type и ставим текущий
    // checkReturn будет сравнивать с current_return_type_
    std::string prev = current_return_type_;
    current_return_type_ = ret;
    pushScope();
    // параметры — это переменные внутри функции, объявляем их до тела
    for (auto& p : fd.params)
        declareVar(p.name, VarInfo{ p.type_name, false, fd.pos.line, fd.pos.col });
    if (fd.body)
        checkBlock(*fd.body);
    popScope();

    current_return_type_ = prev; // восстанавливаем
}

void SemanticAnalyzer::checkStructDecl(const parser::StructDecl& sd) {// проверяем что типы всех полей существуют
    for (auto& f : sd.fields) {
        if (!isKnownType(f.type_name))
            error(sd.pos.line, sd.pos.col, "поле '" + f.name + "': неизвестный тип '" + f.type_name + "'");
    }
}

void SemanticAnalyzer::checkEnumDecl(const parser::EnumDecl& ed) {// проверяем что варианты не повторяются
    std::unordered_set<std::string> seen;
    for (auto& v : ed.variants) {
        if (seen.count(v.name))
            error(ed.pos.line, ed.pos.col, "вариант '" + v.name + "' уже объявлен в перечислении '" + ed.name + "'");
        seen.insert(v.name);
    }
}

void SemanticAnalyzer::checkImplDecl(const parser::ImplDecl& id) {// проверяем что тип для которого пишем методы существует
    if (!structs_.count(id.name))
        error(id.pos.line, id.pos.col, "impl для неизвестной структуры '" + id.name + "'");
    for (auto& m : id.methods)//проверяем каждый метод
        checkDecl(*m);
}
void SemanticAnalyzer::checkNamespaceDecl(const parser::NamespaceDecl& nd) {
    for (auto& d : nd.decls)
        checkDecl(*d);
}
// Блок и инструкции

void SemanticAnalyzer::checkBlock(const parser::Block& block) {
    pushScope();
    for (auto& s : block.stmts)
        checkStmt(*s);
    popScope();
}
void SemanticAnalyzer::checkStmt(const parser::Stmt& stmt) {// смотрим тип узла и вызываем нужную функцию
    if (auto* vd = dynamic_cast<const parser::VarDecl*>(&stmt))
        return checkVarDecl(*vd);
    if (auto* as = dynamic_cast<const parser::Assign*>(&stmt))
        return checkAssign(*as);
    if (auto* ret = dynamic_cast<const parser::Return*>(&stmt))
        return checkReturn(*ret);
    if (auto* ifs = dynamic_cast<const parser::If*>(&stmt))
        return checkIf(*ifs);
    if (auto* wh = dynamic_cast<const parser::While*>(&stmt))
        return checkWhile(*wh);
    if (auto* fr = dynamic_cast<const parser::For*>(&stmt))
        return checkFor(*fr);
    if (auto* mt = dynamic_cast<const parser::Match*>(&stmt))
        return checkMatch(*mt);
    if (auto* blk = dynamic_cast<const parser::Block*>(&stmt))
        return checkBlock(*blk);
    if (auto* es = dynamic_cast<const parser::ExprStmt*>(&stmt)) {
        checkExpr(*es->expr); // проверяем выражение тип не важен
        return;
    }
    if (dynamic_cast<const parser::Break*>(&stmt)) {
        if (loop_depth_ == 0)
            error(stmt.pos.line, stmt.pos.col, "'break' вне цикла");
        return;
    }
    if (dynamic_cast<const parser::Continue*>(&stmt)) {
        if (loop_depth_ == 0)
            error(stmt.pos.line, stmt.pos.col, "'continue' вне цикла");
        return;
    }
}
void SemanticAnalyzer::checkVarDecl(const parser::VarDecl& vd) {// проверяем начальное значение и узнаём его тип
    std::string init_type = checkExpr(*vd.init);    
    if (!vd.type_name.empty()) {// если тип явно указан то проверяем что он существует и совместим
        if (!isKnownType(vd.type_name))
            error(vd.pos.line, vd.pos.col, "неизвестный тип '" + vd.type_name + "'");
        else if (!typesCompatible(vd.type_name, init_type))
            error(vd.pos.line, vd.pos.col, "несовместимые типы: переменная '" + vd.name + "' имеет тип '" + vd.type_name + "', но присваивается '" + init_type + "'");
    }
    
    std::string actual_type = vd.type_name.empty() ? init_type : vd.type_name;// объявляем переменную в текущей области видимости
    if (!declareVar(vd.name, VarInfo{ actual_type, vd.is_mut, vd.pos.line, vd.pos.col }))// если имя уже занято в этом блоке — ошибка
        error(vd.pos.line, vd.pos.col,
              "переменная '" + vd.name + "' уже объявлена в этом блоке");
}

std::string SemanticAnalyzer::checkLValue(const parser::Expr& target, bool check_mut) {//изм2
    if (auto* id = dynamic_cast<const parser::Identifier*>(&target)) {
        const VarInfo* info = lookupVar(id->name);
        if (!info) {
            error(id->pos.line, id->pos.col, "переменная '" + id->name + "' не объявлена");
            return "";
        }
        if (check_mut && !info->is_mut)
            error(id->pos.line, id->pos.col,
                  "переменная '" + id->name + "' неизменяемая: объявите через 'let mut'");
        return info->type_name;
    }
    if (auto* aa = dynamic_cast<const parser::ArrayAccess*>(&target)) {
        std::string idx_type = checkExpr(*aa->index);
        if (!idx_type.empty() && !isIntegerType(idx_type))
            error(aa->pos.line, aa->pos.col,
                  "индекс массива должен быть целым числом, получено '" + idx_type + "'");
        std::string base_type = checkLValue(*aa->base, check_mut);
        if (!base_type.empty() && base_type.front() == '[') {
            auto semi = base_type.find(';');
            if (semi != std::string::npos) {
                std::string elem = base_type.substr(1, semi - 1);
                while (!elem.empty() && elem.back() == ' ') elem.pop_back();
                return elem;
            }
        }
        return "";
    }
    if (auto* fa = dynamic_cast<const parser::FieldAccess*>(&target)) {
        std::string obj_type = checkLValue(*fa->object, check_mut);
        if (obj_type.empty()) return "";
        std::string resolved = resolveAlias(obj_type);
        auto it = structs_.find(resolved);
        if (it == structs_.end()) {
            error(fa->pos.line, fa->pos.col, "тип '" + obj_type + "' не является структурой");
            return "";
        }
        auto fit = it->second.fields.find(fa->field);
        if (fit == it->second.fields.end()) {
            error(fa->pos.line, fa->pos.col,
                  "поле '" + fa->field + "' не найдено в структуре '" + obj_type + "'");
            return "";
        }
        return fit->second;
    }
    error(target.pos.line, target.pos.col, "левая часть присваивания не является lvalue");
    return "";
}

void SemanticAnalyzer::checkAssign(const parser::Assign& as) {//изм2
    std::string lval_type = checkLValue(*as.target, true);
    std::string val_type  = checkExpr(*as.value);
    if (!lval_type.empty() && !val_type.empty() && !typesCompatible(lval_type, val_type))
        error(as.pos.line, as.pos.col,
              "несовместимые типы при присваивании: ожидается '" +
              lval_type + "', получено '" + val_type + "'");
}

void SemanticAnalyzer::checkReturn(const parser::Return& ret) {
    if (ret.value) {
        std::string val_type = checkExpr(*ret.value);// return с выражением
        if (!typesCompatible(current_return_type_, val_type))
            error(ret.pos.line, ret.pos.col, "неверный тип возврата: ожидается '" + current_return_type_ + "', получено '" + val_type + "'");
    } else {   
        if (current_return_type_ != "void")// return без значения функция возвр void
            error(ret.pos.line, ret.pos.col, "функция должна вернуть значение типа '" + current_return_type_ + "'");
    }
}
void SemanticAnalyzer::checkIf(const parser::If& node) {
    std::string cond_type = checkExpr(*node.condition);
    if (!cond_type.empty() && resolveAlias(cond_type) != "bool")
        error(node.pos.line, node.pos.col, "условие if должно быть bool, получено '" + cond_type + "'");

    checkBlock(*node.then_block);
    if (node.else_block)
        checkBlock(*node.else_block);
}
 
void SemanticAnalyzer::checkWhile(const parser::While& node) {
    std::string cond_type = checkExpr(*node.condition);
    if (!cond_type.empty() && resolveAlias(cond_type) != "bool")
        error(node.pos.line, node.pos.col, "условие while должно быть bool, получено '" + cond_type + "'");
 
    loop_depth_++;
    checkBlock(*node.body);
    loop_depth_--;
}

void SemanticAnalyzer::checkFor(const parser::For& node) { // итерируемое имя должно быть объявлено
    const VarInfo* iter = lookupVar(node.iter_name);
    if (!iter)
        error(node.pos.line, node.pos.col, "переменная '" + node.iter_name + "' не объявлена");
    loop_depth_++;
    pushScope();
    std::string elem_type = "";

    const VarInfo* iter_info = lookupVar(node.iter_name);//изм2
    if (iter_info) {
        const std::string& arr_type = iter_info->type_name;
        if (!arr_type.empty() && arr_type.front() == '[') {
            auto semi = arr_type.find(';');
            if (semi != std::string::npos)
                elem_type = arr_type.substr(1, semi - 1);
        }
    }

    declareVar(node.var_name, VarInfo{ elem_type, false, node.pos.line, node.pos.col });
    checkBlock(*node.body);
    popScope();
    loop_depth_--;
}

 
void SemanticAnalyzer::checkMatch(const parser::Match& node) {
    std::string expr_type = checkExpr(*node.expr);
 
    for (auto& arm : node.arms) {
        if (!arm.is_wildcard && arm.pattern) {
            std::string pat_type = checkExpr(*arm.pattern);
            // образец должен быть совместим с выражением
            if (!typesCompatible(expr_type, pat_type))
                error(node.pos.line, node.pos.col, "образец типа '" + pat_type + "' несовместим с выражением типа '" + expr_type + "'");
        }
        checkStmt(*arm.body);
    }
}

// Выражения 
std::string SemanticAnalyzer::checkExpr(const parser::Expr& expr) {
    if (auto* n = dynamic_cast<const parser::IntLiteral*>(&expr))
        return "int64";//изм3
    if (auto* n = dynamic_cast<const parser::FloatLiteral*>(&expr))
        return "float64";
    if (auto* n = dynamic_cast<const parser::StringLiteral*>(&expr))
        return "string";
    if (auto* n = dynamic_cast<const parser::BoolLiteral*>(&expr))
        return "bool";
    if (auto* n = dynamic_cast<const parser::CharLiteral*>(&expr))  // ← добавить
        return "char";
 
    if (auto* n = dynamic_cast<const parser::Identifier*>(&expr)) {
    if (enums_.count(n->name))//изм2 имя enum допустимо как префикс перед точкой
        return n->name;

        const VarInfo* info = lookupVar(n->name);
        if (!info) {
            error(n->pos.line, n->pos.col, "переменная '" + n->name + "' не объявлена");
            return "";
        }
        return info->type_name;
    }
 
    if (auto* n = dynamic_cast<const parser::BinaryOp*>(&expr))
        return checkBinaryOp(*n);
    if (auto* n = dynamic_cast<const parser::UnaryOp*>(&expr))
        return checkUnaryOp(*n);
    if (auto* n = dynamic_cast<const parser::Call*>(&expr))
        return checkCall(*n);
    if (auto* n = dynamic_cast<const parser::ArrayAccess*>(&expr))
        return checkArrayAccess(*n);
    if (auto* n = dynamic_cast<const parser::FieldAccess*>(&expr))
        return checkFieldAccess(*n);
    if (auto* n = dynamic_cast<const parser::Cast*>(&expr))
        return checkCast(*n);
    if (auto* n = dynamic_cast<const parser::StructLiteral*>(&expr))
        return checkStructLiteral(*n);
    if (auto* n = dynamic_cast<const parser::ArrayLiteral*>(&expr))
        return checkArrayLiteral(*n);
 
    if (auto* n = dynamic_cast<const parser::EnumLiteral*>(&expr)) {
        auto it = enums_.find(n->enum_name);// проверяем что enum существует и вариант в нём есть
        if (it == enums_.end()) {
            error(n->pos.line, n->pos.col, "неизвестное перечисление '" + n->enum_name + "'");
            return "";
        }
        auto& variants = it->second.variants;
        bool found = std::find(variants.begin(), variants.end(), n->variant_name) != variants.end();
        if (!found)
            error(n->pos.line, n->pos.col, "вариант '" + n->variant_name + "' не найден в перечислении '" + n->enum_name + "'");
        return n->enum_name;
    }
 
    // TupleLiteral просто проверяем элементы, тип не выводим
    if (auto* n = dynamic_cast<const parser::TupleLiteral*>(&expr)) {
        std::string result = "(";//изм2
        for (size_t i = 0; i < n->elements.size(); ++i){
            if (i) result += ", ";
            result += checkExpr(*n->elements[i]);
        }
        result += ")";//изм2
        return result;
    }
    return "";
}

std::string SemanticAnalyzer::checkBinaryOp(const parser::BinaryOp& node) {
    std::string lt = checkExpr(*node.left);
    std::string rt = checkExpr(*node.right);
 
    using B = parser::BinOp;
 
    switch (node.op) {
        case B::Add://изм2
            if (!lt.empty() && !rt.empty() &&
                resolveAlias(lt) == "string" && resolveAlias(rt) == "string")
                return "string";
            if (!lt.empty() && !isNumericType(lt))
                error(node.pos.line, node.pos.col, "'+': ожидается числовой тип или string, получено '" + lt + "'");
            if (!rt.empty() && !isNumericType(rt))
                error(node.pos.line, node.pos.col, "'+': ожидается числовой тип или string, получено '" + rt + "'");
            if (!typesCompatible(lt, rt))
                 error(node.pos.line, node.pos.col, "несовместимые типы в '+': '" + lt + "' и '" + rt + "'");
            return lt;

        case B::Sub: case B::Mul:// арифметика
        case B::Div: case B::Mod:
            if (!lt.empty() && !isNumericType(lt))
                error(node.pos.line, node.pos.col, "арифметическая операция требует числового типа, получено '" + lt + "'");
            if (!rt.empty() && !isNumericType(rt))
                error(node.pos.line, node.pos.col, "арифметическая операция требует числового типа, получено '" + rt + "'");
            if (!typesCompatible(lt, rt))
                error(node.pos.line, node.pos.col, "несовместимые типы в операции: '" + lt + "' и '" + rt + "'");
            return lt;
 
        case B::Eq: case B::Ne:// сравнение
        case B::Lt: case B::Gt:
        case B::Le: case B::Ge:
            if (!typesCompatible(lt, rt))
                error(node.pos.line, node.pos.col, "нельзя сравнивать '" + lt + "' и '" + rt + "'");
            return "bool";
 
        
        case B::And: case B::Or:// логика
            if (!lt.empty() && resolveAlias(lt) != "bool")
                error(node.pos.line, node.pos.col, "логическая операция требует bool, получено '" + lt + "'");
            if (!rt.empty() && resolveAlias(rt) != "bool")
                error(node.pos.line, node.pos.col, "логическая операция требует bool, получено '" + rt + "'");
            return "bool";
    }
    return "";
}
 
std::string SemanticAnalyzer::checkUnaryOp(const parser::UnaryOp& node) {
    std::string t = checkExpr(*node.operand);
 
    if (node.op == parser::UnOp::Neg) {// унарный минус только числа
        
        if (!t.empty() && !isNumericType(t))
            error(node.pos.line, node.pos.col, "унарный минус требует числового типа, получено '" + t + "'");
        return t;
    }// логическое отрицание только bool
    if (node.op == parser::UnOp::Not) {
        
        if (!t.empty() && resolveAlias(t) != "bool")
            error(node.pos.line, node.pos.col, "'not' требует bool, получено '" + t + "'");
        return "bool";
    }
    return t;
}
 
std::string SemanticAnalyzer::checkCall(const parser::Call& node) {
    if (auto* fa = dynamic_cast<const parser::FieldAccess*>(node.callee.get())) {//доп1
    std::string obj_type = checkExpr(*fa->object);
    std::string resolved = resolveAlias(obj_type);
    auto sit = structs_.find(resolved);
    if (sit == structs_.end()) {
        error(node.pos.line, node.pos.col, "тип '" + obj_type + "' не является структурой");
        return "";
    }
    std::string method_key = resolved + "." + fa->field;
    auto fit = functions_.find(method_key);
    if (fit == functions_.end()) {
        error(node.pos.line, node.pos.col, "метод '" + fa->field + "' не найден в '" + resolved + "'");
        return "";
    }
    // проверяем аргументы (без self)
    auto& info = fit->second;
    size_t expected = info.param_types.size() > 0 ? info.param_types.size() - 1 : 0;
    if (node.args.size() != expected)
        error(node.pos.line, node.pos.col, "метод '" + fa->field + "' ожидает " + std::to_string(expected) + " аргументов");
    for (size_t i = 0; i < node.args.size(); ++i) {
        std::string at = checkExpr(*node.args[i]);
        size_t pi = i + 1; // +1 потому что 0-й это self
        if (pi < info.param_types.size() && !typesCompatible(info.param_types[pi], at))
            error(node.pos.line, node.pos.col, "аргумент " + std::to_string(i+1) + ": ожидается '" + info.param_types[pi] + "', получено '" + at + "'");
    }
    return info.return_type;
}

    auto* callee_id = dynamic_cast<const parser::Identifier*>(node.callee.get());
    if (!callee_id) {
        error(node.pos.line, node.pos.col, "вызов только по имени функции");
        return "";
    }
    const std::string& name = callee_id->name;
    
    std::string builtin = checkBuiltin(name, node.args);
    if (!builtin.empty()) return builtin;
 
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        error(node.pos.line, node.pos.col,
              "функция '" + name + "' не объявлена");
        // всё равно проверяем аргументы чтобы не пропустить ошибки внутри
        for (auto& a : node.args) checkExpr(*a);
        return "";
    }
 
    auto& info = it->second;
 
    // количество аргументов
    if (node.args.size() != info.param_types.size())
        error(node.pos.line, node.pos.col,"функция '" + name + "' ожидает " + std::to_string(info.param_types.size()) + " аргументов, передано " + std::to_string(node.args.size()));
 
    // типы аргументов
    for (size_t i = 0; i < node.args.size(); ++i) {
        std::string at = checkExpr(*node.args[i]);
        if (i < info.param_types.size() && !typesCompatible(info.param_types[i], at))
            error(node.pos.line, node.pos.col, "аргумент " + std::to_string(i + 1) + " функции '" + name + "': ожидается '" + info.param_types[i] + "', получено '" + at + "'");
    }
 
    return info.return_type;
}
 
std::string SemanticAnalyzer::checkArrayAccess(const parser::ArrayAccess& node) {
    std::string base_type = checkExpr(*node.base);
    std::string idx_type = checkExpr(*node.index);
    if (!idx_type.empty() && !isIntegerType(idx_type))
        error(node.pos.line, node.pos.col, "индекс массива должен быть целым числом, получено '" + idx_type + "'");
    
    if (!base_type.empty() && base_type.front() == '[') {//изм2  base_type выглядит как "[int32; 10]" — достаём тип элемента
        auto semi = base_type.find(';');
        if (semi != std::string::npos)
            return base_type.substr(1, semi - 1);
    }
    return "";
}

std::string SemanticAnalyzer::checkFieldAccess(const parser::FieldAccess& node) {
    std::string obj_type = checkExpr(*node.object);
    if (obj_type.empty()) return "";
 
    std::string resolved = resolveAlias(obj_type);

    if (enums_.count(resolved)) {//изм2
        auto& variants = enums_.at(resolved).variants;
        bool found = std::find(variants.begin(), variants.end(), node.field) != variants.end();
        if (!found)
            error(node.pos.line, node.pos.col,
                  "вариант '" + node.field + "' не найден в перечислении '" + resolved + "'");
        return resolved;
    }


    auto it = structs_.find(resolved);
    if (it == structs_.end()) {
        error(node.pos.line, node.pos.col, "тип '" + obj_type + "' не является структурой");
        return "";
    }
 
    auto& fields = it->second.fields;
    auto fit = fields.find(node.field);
    if (fit == fields.end()) {
        error(node.pos.line, node.pos.col, "поле '" + node.field + "' не найдено в структуре '" + obj_type + "'");
        return "";
    }
    return fit->second;
}
 
std::string SemanticAnalyzer::checkCast(const parser::Cast& node) {
    checkExpr(*node.expr);
    if (!isKnownType(node.type))
        error(node.pos.line, node.pos.col,"приведение к неизвестному типу '" + node.type + "'");
    return node.type;
}
 
std::string SemanticAnalyzer::checkStructLiteral(const parser::StructLiteral& node) {
    auto it = structs_.find(node.name);
    if (it == structs_.end()) {
        error(node.pos.line, node.pos.col, "неизвестная структура '" + node.name + "'");
        for (auto& f : node.fields) checkExpr(*f.value);
        return "";
    }
 
    auto& expected_fields = it->second.fields;
 
    // проверяем что все переданные поля существуют и имеют правильный тип
    for (auto& fi : node.fields) {
        auto fit = expected_fields.find(fi.name);
        if (fit == expected_fields.end()) {
            error(node.pos.line, node.pos.col, "поле '" + fi.name + "' не существует в структуре '" + node.name + "'");
            checkExpr(*fi.value);
            continue;
        }
        std::string val_type = checkExpr(*fi.value);
        if (!typesCompatible(fit->second, val_type))
            error(node.pos.line, node.pos.col,"поле '" + fi.name + "': ожидается '" + fit->second + "', получено '" + val_type + "'");
    }
 
    return node.name;
}
 
std::string SemanticAnalyzer::checkArrayLiteral(const parser::ArrayLiteral& node) {
    if (node.elements.empty()) return "";
 
    std::string first_type = checkExpr(*node.elements[0]);
    for (size_t i = 1; i < node.elements.size(); ++i) {
        std::string t = checkExpr(*node.elements[i]);
        if (!typesCompatible(first_type, t))
            error(node.pos.line, node.pos.col, "все элементы массива должны быть одного типа: ожидается '" +
                  first_type + "', получено '" + t + "'");
    }
    return "[" + first_type + "; " + std::to_string(node.elements.size()) + "]";//изм2
}

//встроенные функции
    
std::string SemanticAnalyzer::checkBuiltin(
    const std::string& name,
    const std::vector<parser::ExprPtr>& args)
{
    if (name == "print" || name == "println") {
        for (auto& a : args) checkExpr(*a);
        return "void";
    }
    if (name == "input") {
        if (!args.empty())
            error(0, 0, "'input' не принимает аргументов");
        return "string";
    }
    if (name == "exit") {
        if (args.size() != 1)
            error(0, 0, "'exit' принимает ровно 1 аргумент");
        else {
            std::string t = checkExpr(*args[0]);
            if (!isIntegerType(t))
                error(0, 0, "'exit' ожидает целый тип, получено '" + t + "'");
        }
        return "void";
    }
    if (name == "panic") {
        if (args.size() != 1)
            error(0, 0, "'panic' принимает ровно 1 аргумент");
        else {
            std::string t = checkExpr(*args[0]);
            if (resolveAlias(t) != "string")
                error(0, 0, "'panic' ожидает string, получено '" + t + "'");
        }
        return "void";
    }
    if (name == "assert") {
        if (args.size() != 1)
            error(0, 0, "'assert' принимает ровно 1 аргумент");
        else {
            std::string t = checkExpr(*args[0]);
            if (resolveAlias(t) != "bool")
                error(0, 0, "'assert' ожидает bool, получено '" + t + "'");
        }
        return "void";
    }

    if (name == "len") {//изм2
        if (args.size() != 1)
            error(0, 0, "'len' принимает ровно 1 аргумент");
        else {
            std::string t = checkExpr(*args[0]);
            if (!t.empty() && resolveAlias(t) != "string" && t.front() != '[')
                error(0, 0, "'len' ожидает string или массив, получено '" + t + "'");
        }
        return "int64";//изм3
    }
    return ""; // не встроенная
}

}
