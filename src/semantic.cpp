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

bool SemanticAnalyzer::declareVar(const std::string& name, VarInfo info) {//проверяет может ли быть объявлена переменная
    if (scopes_.empty()) return false; // пустой быть не должен всегда есть pushScope

    auto& top = scopes_.back(); // верхний словарь текущий блок back() возвращает последний элемент вектора чтобы изменения попали в реальный словарь
    if (top.count(name)) return false; // count() возвращает 1 если ключ есть, 0 если нет. Если переменная уже есть в этом блоке возвр false
    top[name] = std::move(info); // записываем в словарь
    return true;
}
