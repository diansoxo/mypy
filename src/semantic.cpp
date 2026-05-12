#include "semantic.hpp"
#include <algorithm>

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
