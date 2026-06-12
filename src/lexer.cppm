module;
#include <string>
#include <vector>
#include <expected>
#include <iostream>
#include <cctype>
export module lexer;
import tokens;  

export namespace lexer {

struct Diagnostic {
    std::string filename;
    int line, col;
    std::string message;
    std::string format() const {
        return filename + ":" + std::to_string(line) + ":"
            + std::to_string(col) + ": error: " + message;
    }
void print() const {
    std::cerr << format() << "\n";
}
};

class Lexer {
public:
    Lexer(const std::string& source, const std::string& filename = "<stdin>");
    std::expected<std::vector<Token>, Diagnostic> tokenize();

private:
    std::string source_, filename_;
    size_t pos_;
    int line_, col_;

    bool isAtEnd() const;
    char current() const;
    char peek(int offset = 1) const;   // дефолт ТОЛЬКО здесь
    char advance();
    Diagnostic makeDiag(const std::string& msg) const;
    void skipWhitespaceAndComments();
    std::expected<Token, Diagnostic> readNumber();
    std::expected<Token, Diagnostic> readChar();
    std::expected<Token, Diagnostic> readString();
    Token readIdentOrKeyword();
    std::expected<Token, Diagnostic> nextToken();
};

Lexer::Lexer(const std::string& source, const std::string& filename)
    : source_(source), filename_(filename), pos_(0), line_(1), col_(1)
{}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}

char Lexer::current() const {
    return isAtEnd() ? '\0' : source_[pos_];
}

char Lexer::peek(int offset) const {
    int idx = pos_ + offset;
    if (idx < 0 || idx >= static_cast<int>(source_.size())) return '\0';
    return source_[idx];
}

char Lexer::advance() {
    char ch = source_[pos_++];
    if (ch == '\n') { line_++; col_ = 1;}
    else { col_++; }
    return ch;
}

Diagnostic Lexer::makeDiag(const std::string& msg) const {
    return Diagnostic{filename_, line_, col_, msg};
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char ch = current();

        if (ch == '#') {
            while (!isAtEnd() && current() != '\n')// однострочный комментарий: до конца строки, \n не поглощаем
                advance();
            continue;
        }

        
        if (ch == '/' && peek() == '*') {//доп6 блочные комментарии /* */
            advance(); // /
            advance(); // *
            int depth = 1; // уровень вложенности
            while (!isAtEnd() && depth > 0) {
                if (current() == '/' && peek() == '*') {
                    advance(); advance();
                    depth++;
                } else if (current() == '*' && peek() == '/') {
                    advance(); advance();
                    depth--;
                } else {
                    advance();
                }
            }
            continue;
        }

        if (ch == '\t') {//таб запрещен
            break;
        }
        if (ch == ' ' || ch == '\r') {
            advance();
            continue;
        }
        break;
    }
}

std::expected<Token, Diagnostic> Lexer::readNumber() {//число или ошибка?
    int  startLine = line_;
    int  startCol= col_;
    std::string buf;
    bool isFloat = false;

    if (current() == '0' && (peek() == 'x' || peek() == 'X')) {//изм2
         buf += advance();
         buf += advance();
        if (!std::isxdigit(current()))
            return std::unexpected(makeDiag("ожидается hex-цифра после '0x'"));
        while (!isAtEnd() && std::isxdigit(current()))
            buf += advance();
        int64_t val = std::stoll(buf, nullptr, 16);// конвертируем в десятичное для хранения
        return Token{TokenType::INT_LITERAL, std::to_string(val), startLine, startCol};
    }

    if (current() == '0' && (peek() == 'b' || peek() == 'B')) {//изм2
        buf += advance();
        buf += advance();
        if (current() != '0' && current() != '1')
            return std::unexpected(makeDiag("ожидается 0 или 1 после '0b'"));
        while (!isAtEnd() && (current() == '0' || current() == '1'))
            buf += advance();
        int64_t val = std::stoll(buf.substr(2), nullptr, 2);
        return Token{TokenType::INT_LITERAL, std::to_string(val), startLine, startCol};
    }

    while (!isAtEnd() && std::isdigit(current()))//изм2
        buf += advance();

    if (current() == '.' && std::isdigit(peek())) {//изм2
        isFloat = true;
        buf += advance();
        while (!isAtEnd() && std::isdigit(current()))
            buf += advance();
    }
    
    if (!isFloat && (current() == 'e' || current() == 'E')) {// экспоненциальная нотация изм2
        isFloat = true;
        buf += advance();
        if (current() == '+' || current() == '-') buf += advance();
        if (!std::isdigit(current()))
            return std::unexpected(makeDiag("ожидается цифра в экспоненте"));
        while (!isAtEnd() && std::isdigit(current()))
            buf += advance();
    } else if (isFloat && (current() == 'e' || current() == 'E')) {
        buf += advance();
        if (current() == '+' || current() == '-') buf += advance();
        if (!std::isdigit(current()))
            return std::unexpected(makeDiag("ожидается цифра в экспоненте"));
        while (!isAtEnd() && std::isdigit(current()))
            buf += advance();
    }

    TokenType t = isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL;
    return Token{t, buf, startLine, startCol};
}

std::expected<Token, Diagnostic> Lexer::readChar() {//изм 
    int startLine = line_;
    int startCol  = col_;
    advance();

    if (isAtEnd())
        return std::unexpected(makeDiag("незакрытый символьный литерал"));

    char value;
    char ch = current();

    if (ch == '\\') {
        advance();
        if (isAtEnd())
            return std::unexpected(makeDiag("незавершённая escape-последовательность"));
        char esc = current();
        switch (esc) {
            case 'n':  value = '\n'; break;
            case 't':  value = '\t'; break;
            case '\'': value = '\''; break;
            case '\\': value = '\\'; break;
            case '0':  value = '\0'; break;
            default: {
                std::string msg = "неизвестная escape-последовательность: \\";
                msg += esc;
                return std::unexpected(makeDiag(msg));
            }
        }
        advance();
    } else if (ch == '\'') {
        return std::unexpected(makeDiag("пустой символьный литерал"));
    } else if (ch == '\n') {
        return std::unexpected(makeDiag("символьный литерал не может содержать перевод строки"));
    } else {
        value = ch;
        advance();
    }
    if (current() != '\'')
        return std::unexpected(makeDiag("ожидается закрывающая ' после символа"));
    advance();

    std::string buf(1, value);
    return Token{TokenType::CHAR_LITERAL, buf, startLine, startCol};
}

std::expected<Token, Diagnostic> Lexer::readString() {//читаем строку
    int startLine = line_;
    int startCol= col_;
    advance();// открывающая " 
    std::string buf;

    while (true) {
        if (isAtEnd())
            return std::unexpected(makeDiag("незакрытая строка"));

        char ch = current();

        if (ch == '\n')
            return std::unexpected(makeDiag("строка не может содержать перевод строки"));

        if (ch == '"') {
            advance(); //закрывающая "
            break;
        }

        if (ch == '\\') {
            advance();// обратный слэш
            if (isAtEnd())
                return std::unexpected(makeDiag("незавершённая escape-последовательность"));

            char esc = current();
            switch (esc) {
                case 'n': buf += '\n'; break;
                case 't': buf += '\t'; break;
                case '"': buf += '"'; break;
                case '\\': buf += '\\'; break;
                default: {
                    std::string msg = "неизвестная escape-последовательность: \\";
                    msg += esc;
                    return std::unexpected(makeDiag(msg));
                }
            }
            advance();
            continue;
        }
        buf += advance();
    }
    return Token{TokenType::STRING_LITERAL, buf, startLine, startCol};
}

Token Lexer::readIdentOrKeyword() {
    int startLine = line_;
    int startCol  = col_;
    std::string buf;

    while (!isAtEnd() && (std::isalnum(current()) || current() == '_'))
        buf += advance();
    if (buf == "inf" || buf == "NaN")//изм2
        return Token{TokenType::FLOAT_LITERAL, buf, startLine, startCol};

    return Token{lookupKeyword(buf), buf, startLine, startCol};
}

std::expected<Token, Diagnostic> Lexer::nextToken() {
    skipWhitespaceAndComments();

    if (isAtEnd())
        return Token{TokenType::EOF_TOKEN, "", line_, col_};

    int  line= line_;
    int  col = col_;
    char ch= current();

    if (ch == '\n') {// явный токен для конца строки
        advance();
        return Token{TokenType::NEWLINE, "\\n", line, col};
    }

    if (ch == '\t')//таб запрещены
        return std::unexpected(makeDiag("табуляция запрещена: используйте пробелы для отступов"));

    if (ch == '\'')
        return readChar();

    if (ch == '"')
        return readString();

    if (std::isdigit(ch))
        return readNumber();

    if (std::isalpha(ch) || ch == '_')
        return readIdentOrKeyword();

    advance();// операторы и разделители сначала сдвигаемся потом смотрим следующий символ
    char nch = current();

    if (ch == '=' && nch == '=') { advance(); return Token{TokenType::EQ, "==", line, col};}// составные операторы
    if (ch == '!' && nch == '=') { advance(); return Token{TokenType::NEQ,"!=", line, col};}
    if (ch == '<' && nch == '=') { advance(); return Token{TokenType::LTE, "<=", line, col};}
    if (ch == '>' && nch == '=') { advance(); return Token{TokenType::GTE, ">=", line, col};}
    if (ch == '-' && nch == '>') { advance(); return Token{TokenType::ARROW, "->", line, col};}
    if (ch == '.' && nch == '.') { advance(); return Token{TokenType::DOTDOT, "..", line, col};}

    switch (ch) {// одиночные символы
        case '+': return Token{TokenType::PLUS, "+", line, col};
        case '-': return Token{TokenType::MINUS, "-", line, col};
        case '*': return Token{TokenType::STAR, "*", line, col};
        case '/': return Token{TokenType::SLASH, "/", line, col};
        case '%': return Token{TokenType::PERCENT, "%", line, col};
        case '<': return Token{TokenType::LT,"<", line, col};
        case '>': return Token{TokenType::GT, ">", line, col};
        case '=': return Token{TokenType::ASSIGN, "=", line, col};
        case '(': return Token{TokenType::LPAREN, "(", line, col};
        case ')': return Token{TokenType::RPAREN, ")", line, col};
        case '[': return Token{TokenType::LBRACKET, "[", line, col};
        case ']': return Token{TokenType::RBRACKET, "]", line, col};
        case '{': return Token{TokenType::LBRACE, "{", line, col};
        case '}': return Token{TokenType::RBRACE, "}", line, col};
        case ':': return Token{TokenType::COLON, ":", line, col};
        case ',': return Token{TokenType::COMMA, ",", line, col};
        case '.': return Token{TokenType::DOT, ".", line, col};
        case ';': return Token{TokenType::SEMICOLON, ";", line, col};
        default:  break;
    }

    std::string msg = "неожиданный символ: '";
    msg += ch;
    msg += "'";
    return std::unexpected(makeDiag(msg));//возвращаем ошибку
}

std::expected<std::vector<Token>, Diagnostic> Lexer::tokenize() {
    std::vector<Token> tokens;// складываем все токены

    while (true) {
        auto result = nextToken();

        if (!result)//если ошибка
            return std::unexpected(result.error());// достаем диагностику

        tokens.push_back(*result);// если не ошибка
        if (result->type == TokenType::EOF_TOKEN)
            break;
    }
    return tokens;
}
}
