#pragma once
// Лексический анализатор: преобразует исходный текст в поток токенов
// Ошибки возвращаются через std::expected<T, Diagnostic>
// исключения для диагностики не используются (требование ТЗ)
#include "tokens.hpp"
#include <string>
#include <vector>
#include <expected>
#include <iostream>

namespace lexer {
    struct Diagnostic {//если ошибка то на какой строке и какой символ
    std::string filename;
    int line;
    int col;
    std::string message;

    std::string format() const {
        return filename + ":" + std::to_string(line) + ":" + std::to_string(col) + ": error: " + message;
    }
    void print() const { std::cerr << format() << "\n"; }
};

class Lexer {
public:
    Lexer(const std::string& source, const std::string& filename = "<stdin>");

    // Основной метод: токенизирует весь исходный текст
    // Возвращает список токенов (включая EOF_TOKEN) или диагностику первой ошибки
    std::expected<std::vector<Token>, Diagnostic> tokenize();

private:
    std::string source_;
    std::string filename_;
    size_t pos_;
    int line_;
    int col_;

    // навигация
    bool isAtEnd() const;
    char current() const;// текущий символ ('\0' если конец)
    char peek(int offset = 1) const;// заглянуть вперёд без сдвига
    char advance();// прочитать символ и сдвинуть позицию

    // создание диагностики
    Diagnostic makeDiag(const std::string& msg) const;

    // пропуск незначимых символов
    void skipWhitespaceAndComments();

    // чтение отдельных видов токенов
    std::expected<Token, Diagnostic> readNumber();
    std::expected<Token, Diagnostic> readString();
    Token readIdentOrKeyword();

    //следующий токен
    std::expected<Token, Diagnostic> nextToken();
};

}