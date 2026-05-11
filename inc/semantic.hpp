#pragma once
#include "ast.hpp"
#include "parser.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace semantic {
using Diagnostic = parser::Diagnostic;

struct VarInfo {
    std::string type_name;
    bool is_mut;
    int line;
    int col;
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
