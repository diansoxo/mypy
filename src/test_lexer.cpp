#include <cassert>
#include <iostream>
import lexer;
import tokens;

static std::vector<lexer::Token> tokenize(const std::string& src) {
    lexer::Lexer lex(src, "<test>");
    auto result = lex.tokenize();
    assert(result && "токенизация упала с ошибкой");
    return *result;
}

// Тест 1: целые и вещественные числа
void test_numbers() {
    auto tokens = tokenize("42 3.14");
    assert(tokens.size() == 3);
    assert(tokens[0].type  == lexer::TokenType::INT_LITERAL);
    assert(tokens[0].value == "42");
    assert(tokens[1].type  == lexer::TokenType::FLOAT_LITERAL);
    assert(tokens[1].value == "3.14");
    assert(tokens[2].type  == lexer::TokenType::EOF_TOKEN);
    std::cout << "[LEXER] test_numbers PASSED\n";
}

// Тест 2: ключевые слова
void test_keywords() {
    auto tokens = tokenize("func let mut return if else");
    assert(tokens[0].type == lexer::TokenType::FUNC);
    assert(tokens[1].type == lexer::TokenType::LET);
    assert(tokens[2].type == lexer::TokenType::MUT);
    assert(tokens[3].type == lexer::TokenType::RETURN);
    assert(tokens[4].type == lexer::TokenType::IF);
    assert(tokens[5].type == lexer::TokenType::ELSE);
    assert(tokens[6].type == lexer::TokenType::EOF_TOKEN);
    std::cout << "[LEXER] test_keywords PASSED\n";
}

// Тест 3: строка с escape-последовательностью
void test_string_escape() {
    auto tokens = tokenize("\"hello\\nworld\"");
    assert(tokens[0].type  == lexer::TokenType::STRING_LITERAL);
    assert(tokens[0].value == "hello\nworld");
    std::cout << "[LEXER] test_string_escape PASSED\n";
}

int main() {
    test_numbers();
    test_keywords();
    test_string_escape();
    std::cout << "\n[LEXER] Все тесты прошли.\n";
    return 0;
}
