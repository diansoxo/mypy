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

//вспомогательные проверки типов
    std::string resolveAlias(const std::string& type_name) const;// разворачивает псевдонимы: "///" → "int32"
    bool isNumericType(const std::string& t) const;// является ли тип числовым (int*, uint*, float*)
    bool isIntegerType(const std::string& t) const;// является ли тип целочисленным (int*, uint*) — нужно для индексов массивов
    bool typesCompatible(const std::string& a, const std::string& b) const;// совместимы ли два типа (с учётом псевдонимов)
    bool isKnownType(const std::string& type_name) const;// объявлен ли такой тип вообще (встроенный, структура, enum, псевдоним)
    std::string checkBuiltin(const std::string& name, const std::vector<parser::ExprPtr>& args);// проверка встроенных функций: print, input, exit, panic
};
}
