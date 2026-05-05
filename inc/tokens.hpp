#pragma once
// Определение типов токенов и структуры Token для лексического анализатора
// Все определения находятся в namespace Lexer

#include <string>
#include <unordered_map>

namespace lexer {
enum class TokenType {
    INT_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    BOOL_LITERAL,

    IDENTIFIER,

    FUNC,
    LET,
    MUT,
    RETURN,
    IF,
    ELSE,
    WHILE,
    FOR,
    IN,
    BREAK,
    CONTINUE,
    PASS,
    AND,
    OR,
    NOT,
    MATCH,
    CASE,
    STRUCT,
    ENUM,
    IMPL,
    TYPE,
    NAMESPACE,
    AS,

    INT8,// Встроенные типы
    INT16,
    INT32,
    INT64,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    FLOAT32,
    FLOAT64,
    BOOL_TYPE,
    STRING_TYPE,
    VOID,

    PLUS,// Арифметические операторы
    MINUS,
    STAR,
    SLASH, 
    PERCENT,

    EQ,
    NEQ,
    LT,// <
    GT,// >
    LTE,// <=
    GTE,// >=

    ASSIGN,// Присваивание


    ARROW,// -> Специальные
    DOTDOT,// .. (составной оператор)

    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,// ]
    LBRACE,// {
    RBRACE,
    COLON,// :
    COMMA,
    DOT,
    SEMICOLON,

    NEWLINE,
    EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string value;// лексема
    int line;
    int col;

    std::string toString() const;
};


TokenType lookupKeyword(const std::string& word);// Возвращает TokenType для ключевого слова или IDENT если слово не найдено
std::string tokenTypeName(TokenType t);// Возвращает строковое имя TokenType (для --dump-tokens и отладки)
}