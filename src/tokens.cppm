module;
#include <string>
#include <unordered_map>
#include <sstream>
export module tokens;

export namespace lexer {
enum class TokenType {//типы токенов
    INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, BOOL_LITERAL,
    IDENTIFIER, CHAR_LITERAL,
    FUNC, LET, MUT, RETURN,
    IF, ELSE, WHILE, FOR, IN, BREAK, CONTINUE, PASS,
    AND, OR, NOT,
    MATCH, CASE,
    STRUCT, ENUM, IMPL, TYPE, NAMESPACE, AS,
    INT8, INT16, INT32, INT64,
    UINT8, UINT16, UINT32, UINT64,
    FLOAT32, FLOAT64,
    BOOL_TYPE, STRING_TYPE, CHAR_TYPE, VOID,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQ, NEQ, LT, GT, LTE, GTE,
    ASSIGN, ARROW, DOTDOT,
    LPAREN, RPAREN, LBRACKET, RBRACKET, LBRACE, RBRACE,
    COLON, COMMA, DOT, SEMICOLON,
    NEWLINE, EOF_TOKEN
};

std::string tokenTypeName(TokenType t);

struct Token {
    TokenType type;
    std::string value; // лексема
    int line;
    int col;
    std::string toString() const;
};

}

namespace lexer {
static const std::unordered_map<std::string, TokenType> KEYWORDS = { //словарь
    // управляющие конструкции
    {"func", TokenType::FUNC},
    {"let", TokenType::LET},
    {"mut", TokenType::MUT},
    {"return", TokenType::RETURN},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"for", TokenType::FOR},
    {"in", TokenType::IN},
    {"break", TokenType::BREAK},
    {"continue", TokenType::CONTINUE},
    {"pass", TokenType::PASS},
    // логические операторы (ключевые слова)
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"not", TokenType::NOT},
    // сопоставление с образцом
    {"match", TokenType::MATCH},
    {"case", TokenType::CASE},
    // объявления типов
    {"struct", TokenType::STRUCT},
    {"enum", TokenType::ENUM},
    {"impl", TokenType::IMPL},
    {"type", TokenType::TYPE},
    {"namespace", TokenType::NAMESPACE},
    // приведение типа
    {"as", TokenType::AS},
    // булевы литералы
    {"true", TokenType::BOOL_LITERAL},
    {"false", TokenType::BOOL_LITERAL},
    // встроенные типы
    {"int8", TokenType::INT8},
    {"int16", TokenType::INT16},
    {"int32", TokenType::INT32},
    {"int64", TokenType::INT64},
    {"uint8", TokenType::UINT8},
    {"uint16", TokenType::UINT16},
    {"uint32", TokenType::UINT32},
    {"uint64", TokenType::UINT64},
    {"float32", TokenType::FLOAT32},
    {"float64", TokenType::FLOAT64},
    {"bool", TokenType::BOOL_TYPE},
    {"char", TokenType::CHAR_TYPE},
    {"string", TokenType::STRING_TYPE},
    {"void", TokenType::VOID},
};
}

export namespace lexer {//всё внутри блока автоматически стало публичным интерфейсом модуля
TokenType lookupKeyword(const std::string& word){ //если не нашли нужный тип то возвращаем возвращаем тип IDENTIFIER
    auto it = KEYWORDS.find(word);
    return (it != KEYWORDS.end()) ? it->second : TokenType::IDENTIFIER;
}

std::string tokenTypeName(TokenType t) {//принимает тип и возвращает его строкой
    switch (t) {
        case TokenType::INT_LITERAL:   return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::BOOL_LITERAL:  return "BOOL_LITERAL";
        case TokenType::IDENTIFIER:return "IDENTIFIER";
        case TokenType::FUNC: return "FUNC";
        case TokenType::LET: return "LET";
        case TokenType::MUT: return "MUT";
        case TokenType::RETURN: return "RETURN";
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::FOR: return "FOR";
        case TokenType::IN: return "IN";
        case TokenType::BREAK: return "BREAK";
        case TokenType::CONTINUE: return "CONTINUE";
        case TokenType::PASS: return "PASS";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::MATCH: return "MATCH";
        case TokenType::CASE: return "CASE";
        case TokenType::STRUCT: return "STRUCT";
        case TokenType::ENUM: return "ENUM";
        case TokenType::IMPL: return "IMPL";
        case TokenType::TYPE: return "TYPE";
        case TokenType::NAMESPACE:return "NAMESPACE";
        case TokenType::AS:return "AS";
        case TokenType::INT8: return "INT8";
        case TokenType::INT16: return "INT16";
        case TokenType::INT32: return "INT32";
        case TokenType::INT64: return "INT64";
        case TokenType::UINT8:return "UINT8";
        case TokenType::UINT16:return "UINT16";
        case TokenType::UINT32:return "UINT32";
        case TokenType::UINT64: return "UINT64";
        case TokenType::FLOAT32:return "FLOAT32";
        case TokenType::FLOAT64: return "FLOAT64";
        case TokenType::BOOL_TYPE: return "BOOL_TYPE";
        case TokenType::STRING_TYPE: return "STRING_TYPE";
        case TokenType::CHAR_LITERAL: return "CHAR_LITERAL";
        case TokenType::CHAR_TYPE: return "CHAR_TYPE";
        case TokenType::VOID: return "VOID";
        case TokenType::PLUS:return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::PERCENT: return "PERCENT";
        case TokenType::EQ: return "EQ";
        case TokenType::NEQ: return "NEQ";
        case TokenType::LT: return "LT";
        case TokenType::GT: return "GT";
        case TokenType::LTE: return "LTE";
        case TokenType::GTE: return "GTE";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::ARROW: return "ARROW";
        case TokenType::DOTDOT: return "DOTDOT";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::COLON: return "COLON";
        case TokenType::COMMA: return "COMMA";
        case TokenType::DOT: return "DOT";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::EOF_TOKEN: return "EOF";
        default: return "UNKNOWN";
    }
}

std::string Token::toString() const {
    return "Token(" + tokenTypeName(type) + ", \"" + value + "\"" + ", " + std::to_string(line) + ":" + std::to_string(col) + ")";
}
}
