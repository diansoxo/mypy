#pragma once
// Синтаксический анализатор: преобразует поток токенов в AST
// Метод: рекурсивный спуск (Recursive Descent)

#include "tokens.hpp"
#include "lexer.hpp"
#include "ast.hpp"
#include <vector>
#include <expected>

namespace parser {
using lexer::Token;
using lexer::TokenType;
struct Diagnostic {
    std::string filename;
    int line;
    int col;
    std::string message;

    std::string format() const {
        return filename + ":" + std::to_string(line) + ":" +
               std::to_string(col) + ": error: " + message;
    }
    void print() const { std::cerr << format() << "\n"; }
};

class Parser {
public:
Parser(std::vector<Token> tokens, const std::string& filename);//список токенов из лексера
struct ParseResult {
    Program program;
    std::vector<Diagnostic> errors;
    bool ok() const { return errors.empty(); }
};
ParseResult parse();//дерево получаем или ошибка

private:
    std::vector<Token> tokens_; //все токены от лексера
    size_t pos_;// текущая позиция в tokens_
    std::string filename_;// имя файла для диагностики
    std::vector<Diagnostic> diagnostics_;//изм
    //Навигация
    const Token& current() const;
    const Token& peek(int offset = 1) const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    std::expected<Token, Diagnostic> expect(
        TokenType type, const std::string& msg);

    void skipNewlines();

    // Создать диагностику на текущей позиции
    Diagnostic makeDiag(const std::string& msg) const;
void emitDiag(const std::string& msg); //изм
void synchronize();//изм
    // объявления
    std::expected<DeclPtr, Diagnostic> parseDecl();
    std::expected<std::unique_ptr<FuncDef>, Diagnostic> parseFuncDef();
    std::expected<std::unique_ptr<StructDecl>,Diagnostic> parseStructDecl();
    std::expected<std::unique_ptr<EnumDecl>,Diagnostic> parseEnumDecl();
    std::expected<std::unique_ptr<ImplDecl>, Diagnostic> parseImplDecl();
    std::expected<std::unique_ptr<NamespaceDecl>, Diagnostic> parseNamespaceDecl();
    std::expected<std::unique_ptr<TypeAlias>, Diagnostic> parseTypeAlias();
    

    // инструкции
    std::expected<std::unique_ptr<Block>, Diagnostic> parseBlock();
    std::expected<StmtPtr, Diagnostic> parseStmt();
    std::expected<StmtPtr, Diagnostic> parseVarDecl();
    std::expected<StmtPtr, Diagnostic> parseAssignOrExprStmt();
    std::expected<StmtPtr, Diagnostic> parseReturn();
    std::expected<StmtPtr, Diagnostic> parseIf();
    std::expected<StmtPtr, Diagnostic> parseWhile();
    std::expected<StmtPtr, Diagnostic> parseFor();
    std::expected<StmtPtr, Diagnostic> parseMatch();

//приоритеты
    std::expected<ExprPtr, Diagnostic> parseExpr();
    std::expected<ExprPtr, Diagnostic> parseLogicalOr();
    std::expected<ExprPtr, Diagnostic> parseLogicalAnd();
    std::expected<ExprPtr, Diagnostic> parseEquality();
    std::expected<ExprPtr, Diagnostic> parseComparison();
    std::expected<ExprPtr, Diagnostic> parseSum();
    std::expected<ExprPtr, Diagnostic> parseTerm();
    std::expected<ExprPtr, Diagnostic> parseUnary();
    std::expected<ExprPtr, Diagnostic> parsePostfixExpr();
    std::expected<ExprPtr, Diagnostic> parsePrimary();

    std::expected<ExprPtr, Diagnostic> parseIdentOrCall();
    std::expected<ExprPtr, Diagnostic> parsePostfix(ExprPtr expr); 
    std::expected<ExprPtr, Diagnostic> parseArrayLiteral();
    std::expected<ExprPtr, Diagnostic> parseTupleOrParen();

    std::expected<std::string, Diagnostic> parseType();
};

}
