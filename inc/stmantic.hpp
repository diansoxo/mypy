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
