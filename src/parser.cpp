#include "parser.hpp"
#include "lexer.hpp"
#include <cassert>

namespace parser {
using lexer::Token;
using lexer::TokenType;
Parser::Parser(std::vector<Token> tokens, const std::string& filename)//конструктор для иницализации полей до хождения в цикл
    : tokens_(std::move(tokens)) // вектор токенов
    , pos_(0) // начинаем с первого
    , filename_(filename)// запоминаем имя файла входные данные
{}

const Token& Parser::current() const {// Текущий токен
    return tokens_[pos_];
}

const Token& Parser::peek(int offset) const {// Заглянуть вперёд на offset позиций без сдвига закладки
    size_t idx = pos_ + offset;
    if (idx >= tokens_.size())
        return tokens_.back();// это EOF
    return tokens_[idx];
}

const Token& Parser::advance() {// Взять текущий токен и сдвинуть закладку вперёд
    if (pos_ + 1 < tokens_.size())
        pos_++;
    return tokens_[pos_ - 1];//возвращаем то что было до сдвига
}

// Возвращает true если тип совпадает
bool Parser::check(TokenType type) const {// Проверить тип текущего токена без сдвига
    return current().type == type;
}

bool Parser::match(TokenType type) {// Используется для необязательных токенов
    if (check(type)) {
        advance();
        return true;// Если текущий токен совпадает с type взять его и вернуть true
    }
    return false;
}

std::expected<Token, Diagnostic> Parser::expect(// Используется когда токен обязателен по грамматике
    TokenType type, const std::string& msg)
{
    if (check(type)) {
        return advance(); //берем и возвращаем токен
    } 
    return std::unexpected(makeDiag(msg));// но если токена нет ошибка
}

void Parser::skipNewlines() { //пропускаем переносы строки между объявлениями и внутри выражений
    while (check(TokenType::NEWLINE))
        advance();
}

Diagnostic Parser::makeDiag(const std::string& msg) const {//объект ошибки на текущем токене
    return Diagnostic{
        filename_,
        current().line,
        current().col,
        msg
    };
}

//изм Записать ошибку в список и продолжить
void Parser::emitDiag(const std::string& msg) {
    diagnostics_.push_back(makeDiag(msg));
}

//изм Паника: пропускаем токены до ближайшего якоря
void Parser::synchronize() {
    while (!check(TokenType::EOF_TOKEN)) {
        // якоря — места откуда можно безопасно продолжить
        if (check(TokenType::NEWLINE)  ||
            check(TokenType::SEMICOLON)||
            check(TokenType::RBRACE)   ||
            check(TokenType::FUNC)     ||
            check(TokenType::STRUCT)   ||
            check(TokenType::ENUM)     ||
            check(TokenType::IMPL)     ||
            check(TokenType::NAMESPACE)||
            check(TokenType::TYPE))
        {
            if (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
                advance(); // берем сам разделитель
            return;
        }
        advance();
    }
}

// Разбор выражений
// Цепочка вызовов снизу вверх по приоритету parseExpr -parseLogicalOr - parseLogicalAnd - parseComparison - parseSum - parseTerm - parseUnary - parsePrimary
//макрос вызвать функцию и проверяет на ошибку сразу чтобы ее вернуть до компиляции
#define TRY_EXPR(var, expr) \
    auto var##_res = (expr); \
    if (!var##_res) \
        return std::unexpected(var##_res.error()); \
    auto var = std::move(*var##_res);

std::expected<ExprPtr, Diagnostic> Parser::parseExpr() {// уровень 1 начинаем с самого низкого приоритета
    return parseLogicalOr();
}

std::expected<ExprPtr, Diagnostic> Parser::parseLogicalOr() {// Уровень 2 or
    auto left_res = parseLogicalAnd();
    if (!left_res) return std::unexpected(left_res.error());
    auto left = std::move(*left_res);
 
    while (check(TokenType::OR)) {
        advance();
        auto right_res = parseLogicalAnd();
        if (!right_res) return std::unexpected(right_res.error());
        auto right = std::move(*right_res);
 
        auto node = std::make_unique<BinaryOp>();
        node->pos = left->pos;
        node->op = "or";
        node->left = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}
 
std::expected<ExprPtr, Diagnostic> Parser::parseLogicalAnd() {// Уровень 3 and
    auto left_res = parseEquality();
    if (!left_res) return std::unexpected(left_res.error());
    auto left = std::move(*left_res);
 
    while (check(TokenType::AND)) {
        advance();
        auto right_res = parseEquality();
        if (!right_res) return std::unexpected(right_res.error());
        auto right = std::move(*right_res);
 
        auto node = std::make_unique<BinaryOp>();
        node->pos = left->pos;
        node->op = "and";
        node->left = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

std::expected<ExprPtr, Diagnostic> Parser::parseEquality() {//уровень 4 только == != //изм
    auto left_res = parseComparison(); // вызывает уровень ниже
    if (!left_res) return std::unexpected(left_res.error());
    auto left = std::move(*left_res);

    std::string op;
    if      (check(TokenType::EQ))  op = "==";
    else if (check(TokenType::NEQ)) op = "!=";

    if (!op.empty()) {
        advance();
        auto right_res = parseComparison();
        if (!right_res) return std::unexpected(right_res.error());
        auto node = std::make_unique<BinaryOp>();
        node->pos = left->pos;
        node->op = op;
        node->left = std::move(left);
        node->right = std::move(right_res.value());
        return node;
    }
    return left;
}

std::expected<ExprPtr, Diagnostic> Parser::parseComparison() {//уровень 5 >= <= < > //изм
    auto left_res = parseSum(); // вызывает уровень ниже
    if (!left_res) return std::unexpected(left_res.error());
    auto left = std::move(*left_res);

    std::string op;
    if      (check(TokenType::LT))  op = "<";
    else if (check(TokenType::GT))  op = ">";
    else if (check(TokenType::LTE)) op = "<=";
    else if (check(TokenType::GTE)) op = ">=";

    if (!op.empty()) {
        advance();
        auto right_res = parseSum();
        if (!right_res) return std::unexpected(right_res.error());
        auto node = std::make_unique<BinaryOp>();
        node->pos = left->pos;
        node->op = op;
        node->left = std::move(left);
        node->right = std::move(right_res.value());
        return node;
    }
    return left;
}

std::expected<ExprPtr, Diagnostic> Parser::parseSum() {// Уровень 6 + -
    auto left_res = parseTerm();
    if (!left_res) return std::unexpected(left_res.error());
    auto left = std::move(*left_res);
 
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = check(TokenType::PLUS) ? "+" : "-";
        advance();
        auto right_res = parseTerm();
        if (!right_res) return std::unexpected(right_res.error());
        auto right = std::move(*right_res);
 
        auto node = std::make_unique<BinaryOp>();
        node->pos = left->pos;
        node->op = op;
        node->left = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}
 
std::expected<ExprPtr, Diagnostic> Parser::parseTerm() {// Уровень 7 * / %
    auto left_res = parseUnary();
    if (!left_res) return std::unexpected(left_res.error());
    auto left = std::move(*left_res);
 
    while (check(TokenType::STAR) ||
           check(TokenType::SLASH) ||
           check(TokenType::PERCENT))
    {
        std::string op;
        if      (check(TokenType::STAR)) op = "*";
        else if (check(TokenType::SLASH)) op = "/";
        else op = "%";
        advance();
        auto right_res = parseUnary();
        if (!right_res) return std::unexpected(right_res.error());
        auto right = std::move(*right_res);
 
        auto node = std::make_unique<BinaryOp>();
        node->pos = left->pos;
        node->op = op;
        node->left = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}
 
std::expected<ExprPtr, Diagnostic> Parser::parseUnary() {// Уровень 8 унарные - и not
    if (check(TokenType::MINUS)) {
        auto pos = Position{current().line, current().col};
        advance();
        auto operand_res = parseUnary();
        if (!operand_res) return std::unexpected(operand_res.error());
        auto node = std::make_unique<UnaryOp>();
        node->pos = pos;
        node->op = "-";
        node->operand = std::move(*operand_res);
        return node;
    }
    if (check(TokenType::NOT)) {
        auto pos = Position{current().line, current().col};
        advance();
        auto operand_res = parseUnary();
        if (!operand_res) return std::unexpected(operand_res.error());
        auto node = std::make_unique<UnaryOp>();
        node->pos = pos;
        node->op = "not";
        node->operand = std::move(*operand_res);
        return node;
    }
    return parsePostfixExpr();//изм
}

std::expected<ExprPtr, Diagnostic> Parser::parsePrimary() {//изм
    auto pos = Position{current().line, current().col};

    if (check(TokenType::INT_LITERAL)) {
        auto tok = advance();
        auto node = std::make_unique<IntLiteral>();
        node->pos = pos;
        node->value = std::stoll(tok.value);
        return node;
    }
    if (check(TokenType::FLOAT_LITERAL)) {
        auto tok = advance();
        auto node = std::make_unique<FloatLiteral>();
        node->pos = pos;
        node->value = std::stod(tok.value);
        return node;
    }
    if (check(TokenType::STRING_LITERAL)) {
        auto tok = advance();
        auto node = std::make_unique<StringLiteral>();
        node->pos = pos;
        node->value = tok.value;
        return node;
    }
    if (check(TokenType::BOOL_LITERAL)) {
        auto tok = advance();
        auto node = std::make_unique<BoolLiteral>();
        node->pos = pos;
        node->value = (tok.value == "true");
        return node;
    }
    if (check(TokenType::IDENTIFIER)) {
        return parseIdentOrCall();
    }
    if (check(TokenType::LBRACKET)) {
        return parseArrayLiteral();
    }
    if (check(TokenType::LPAREN)) {
        return parseTupleOrParen();
    }
    return std::unexpected(makeDiag(
        "ожидается выражение, получен: '" + current().value + "'"));
}

std::expected<ExprPtr, Diagnostic> Parser::parsePostfixExpr() {
    auto expr_res = parsePrimary();
    if (!expr_res) return std::unexpected(expr_res.error());
    return parsePostfix(std::move(*expr_res));
}
 
// идентификатор, вызов, enum литерал, struct литерал
std::expected<ExprPtr, Diagnostic> Parser::parseIdentOrCall() {
    auto pos = Position{current().line, current().col};
    std::string name = current().value;
    advance(); // съедаем имя
 
    // вызов: name(args)
    if (check(TokenType::LPAREN)) {
        advance(); // съедаем "("
        auto node = std::make_unique<Call>();
        node->pos = pos;
        node->name = name;
        if (!check(TokenType::RPAREN)) {
            while (true) {
                auto arg_res = parseExpr();
                if (!arg_res) return std::unexpected(arg_res.error());
                node->args.push_back(std::move(*arg_res));
                if (!match(TokenType::COMMA)) break;
            }
        }
        auto rp = expect(TokenType::RPAREN, "ожидается ')' после аргументов");
        if (!rp) return std::unexpected(rp.error());
        return node;
    }
 
    // enum литерал: Name.Variant
    if (check(TokenType::DOT) &&
        peek(1).type == TokenType::IDENTIFIER)
    {
        advance(); // съедаем "."
        std::string variant = current().value;
        advance();
        auto node = std::make_unique<EnumLiteral>();
        node->pos = pos;
        node->enum_name = name;
        node->variant_name = variant;
        return node;
    }
 
    if (check(TokenType::LBRACE)) {// struct литерал: Name { field = expr }
        advance(); // съедаем "{"
        auto node = std::make_unique<StructLiteral>();
        node->pos = pos;
        node->name = name;
        if (!check(TokenType::RBRACE)) {
            while (true) {
                if (!check(TokenType::IDENTIFIER))
                    return std::unexpected(makeDiag("ожидается имя поля"));
                std::string fname = current().value;
                advance();
                auto eq = expect(TokenType::ASSIGN, "ожидается '='");
                if (!eq) return std::unexpected(eq.error());
                auto val_res = parseExpr();
                if (!val_res) return std::unexpected(val_res.error());
                node->fields.push_back(FieldInit{fname, std::move(*val_res)});
                if (!match(TokenType::COMMA)) break;
            }
        }
        auto rb = expect(TokenType::RBRACE, "ожидается '}'");
        if (!rb) return std::unexpected(rb.error());
        return node;
    }
 
    auto node = std::make_unique<Identifier>();// просто переменная: x
    node->pos = pos;
    node->name = name;
    return node;
}

std::expected<ExprPtr, Diagnostic> Parser::parsePostfix(ExprPtr expr) {// постфиксы
    while (true) {
        // индексация: expr[index]
        if (check(TokenType::LBRACKET)) {
            advance();
            auto idx_res = parseExpr();
            if (!idx_res) return std::unexpected(idx_res.error());
            auto rb = expect(TokenType::RBRACKET, "ожидается ']'");
            if (!rb) return std::unexpected(rb.error());
            auto node = std::make_unique<ArrayAccess>();
            node->pos = expr->pos;
            if (auto* id = dynamic_cast<Identifier*>(expr.get()))
                node->name = id->name;
            node->index = std::move(*idx_res);
            expr = std::move(node);
            continue;
        }
        // поле: expr.field
        if (check(TokenType::DOT)) {
            advance();
            if (!check(TokenType::IDENTIFIER))
                return std::unexpected(makeDiag("ожидается имя поля после '.'"));
            std::string field = current().value;
            advance();
            auto node = std::make_unique<FieldAccess>();
            node->pos = expr->pos;
            node->object = std::move(expr);
            node->field = field;
            expr = std::move(node);
            continue;
        }  
        // приведение: expr as type
        if (check(TokenType::AS)) {
            advance();
            auto type_res = parseType();
            if (!type_res) return std::unexpected(type_res.error());
            auto node = std::make_unique<Cast>();
            node->pos = expr->pos;
            node->expr = std::move(expr);
            node->type = *type_res;
            expr = std::move(node);
            continue;
        }
        break;
    }
    return expr;
}
 
std::expected<ExprPtr, Diagnostic> Parser::parseArrayLiteral() {// массив
    auto pos = Position{current().line, current().col};
    advance(); //берем скобку кв
    auto node = std::make_unique<ArrayLiteral>();//созд узел с внутренними выражениями
    node->pos = pos;
    if (!check(TokenType::RBRACKET)){
        while (true) {
            auto el_res = parseExpr();
            if (!el_res) return std::unexpected(el_res.error());
            node->elements.push_back(std::move(*el_res));
            if (!match(TokenType::COMMA)) break;
        }
    }
    auto rb = expect(TokenType::RBRACKET, "ожидается ']'");
    if (!rb) return std::unexpected(rb.error());
    return node;
}

std::expected<ExprPtr, Diagnostic> Parser::parseTupleOrParen() {// скобки или кортеж: (expr) или (a, b)
    auto pos = Position{current().line, current().col};
    advance(); // берем (
    auto first_res = parseExpr();
    if (!first_res) return std::unexpected(first_res.error());
    auto first = std::move(*first_res);
    // нет запятой — просто скобки
    if (!check(TokenType::COMMA)) {
        auto rp = expect(TokenType::RPAREN, "ожидается ')'");
        if (!rp) return std::unexpected(rp.error());
        return first;
    }
    // есть запятая — кортеж
    auto node = std::make_unique<TupleLiteral>();
    node->pos = pos;
    node->elements.push_back(std::move(first));
    while (match(TokenType::COMMA)) {
        auto el_res = parseExpr();
        if (!el_res) return std::unexpected(el_res.error());
        node->elements.push_back(std::move(*el_res));
    }
    auto rp = expect(TokenType::RPAREN, "ожидается ')'");
    if (!rp) return std::unexpected(rp.error());
    return node;
}

std::expected<std::string, Diagnostic> Parser::parseType() {
    
    if (check(TokenType::LBRACKET)) {// массив: [type; size]
        advance();
        auto et_res = parseType();
        if (!et_res) return std::unexpected(et_res.error());
        auto sc = expect(TokenType::SEMICOLON, "ожидается ';' в типе массива");
        if (!sc) return std::unexpected(sc.error());
        if (!check(TokenType::INT_LITERAL))
            return std::unexpected(makeDiag("ожидается размер массива"));
        std::string size = current().value;
        advance();
        auto rb = expect(TokenType::RBRACKET, "ожидается ']'");
        if (!rb) return std::unexpected(rb.error());
        return "[" + *et_res + "; " + size + "]";
    }

    if (check(TokenType::LPAREN)) {// кортеж: (type, type)
        advance();
        std::string result = "(";
        auto t1_res = parseType();
        if (!t1_res) return std::unexpected(t1_res.error());
        result += *t1_res;
        while (match(TokenType::COMMA)) {
            auto t2_res = parseType();
            if (!t2_res) return std::unexpected(t2_res.error());
            result += ", " + *t2_res;
        }
        auto rp = expect(TokenType::RPAREN, "ожидается ')'");
        if (!rp) return std::unexpected(rp.error());
        return result + ")";
    }
    // встроенные типы
    using TT = TokenType;
    if (check(TT::INT8)  || check(TT::INT16)  || check(TT::INT32)  || check(TT::INT64)  ||
        check(TT::UINT8) || check(TT::UINT16) || check(TT::UINT32) || check(TT::UINT64) ||
        check(TT::FLOAT32) || check(TT::FLOAT64) ||
        check(TT::BOOL_TYPE) || check(TT::STRING_TYPE) || check(TT::VOID))
    {
        std::string name = current().value;
        advance();
        return name;
    }
    // пользовательский тип: MyStruct
    if (check(TokenType::IDENTIFIER)) {
        std::string name = current().value;
        advance();
        return name;
    }
    return std::unexpected(makeDiag("ожидается тип"));
}
 
}

// Разбор инструкций
namespace parser {
using lexer::Token;
using lexer::TokenType;

std::expected<std::unique_ptr<Block>, Diagnostic> Parser::parseBlock() {//тело функций
    auto pos = Position{current().line, current().col};
 
    auto lb = expect(TokenType::LBRACE, "ожидается '{'");
    if (!lb) return std::unexpected(lb.error());
 
    auto block = std::make_unique<Block>();
    block->pos = pos;
 
    skipNewlines();

// изм записать ошибку и продолжить со следующей инструкции
while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
    auto stmt_res = parseStmt();
    if (!stmt_res) {
        diagnostics_.push_back(stmt_res.error()); // записываем
        synchronize();//ищем следующую инструкцию
    } else {
        block->stmts.push_back(std::move(*stmt_res));
    }
    skipNewlines();
}

    auto rb = expect(TokenType::RBRACE, "ожидается '}'");
    if (!rb) return std::unexpected(rb.error());
 
    return block;
}

std::expected<StmtPtr, Diagnostic> Parser::parseStmt(){// диспетчер см на текущий токен и выбирает нужный метод
    using TT = TokenType;
    skipNewlines();
    if (check(TT::LET)) return parseVarDecl();
    if (check(TT::RETURN)) return parseReturn();
    if (check(TT::IF)) return parseIf();
    if (check(TT::WHILE)) return parseWhile();
    if (check(TT::FOR)) return parseFor();
    if (check(TT::MATCH)) return parseMatch();
    if (check(TT::BREAK)){
        auto pos = Position{current().line, current().col};
        advance();//break
        if (!check(TT::NEWLINE) && !check(TT::RBRACE) && !check(TT::EOF_TOKEN))// после инструкции обязателен перенос строки
            return std::unexpected(makeDiag("ожидается перенос строки после 'break'"));
        auto node = std::make_unique<Break>();
        node->pos = pos;
        return node;
    }
 
    if (check(TT::CONTINUE)) {
        auto pos = Position{current().line, current().col};
        advance();
        if (!check(TT::NEWLINE) && !check(TT::RBRACE) && !check(TT::EOF_TOKEN))
            return std::unexpected(makeDiag("ожидается перенос строки после 'continue'"));
        auto node = std::make_unique<Continue>();
        node->pos = pos;
        return node;
    }
 
    if (check(TT::PASS)) {
        auto pos = Position{current().line, current().col};
        advance();
        auto node = std::make_unique<Pass>();
        node->pos = pos;
        return node;
    }
    return parseAssignOrExprStmt();// всё остальное присваивание или вызов функции
}

std::expected<StmtPtr, Diagnostic> Parser::parseVarDecl() {
    auto pos = Position{current().line, current().col};
    advance();//let
 
    bool is_mut = false;
    if (check(TokenType::MUT)) {
        is_mut = true;
        advance();
    }
    if (!check(TokenType::IDENTIFIER))// обязательное имя переменной
        return std::unexpected(makeDiag("ожидается имя переменной"));
    std::string name = current().value;
    advance();

    std::string type_name;//необяз тип
    if (match(TokenType::COLON)) {
        auto type_res = parseType();
        if (!type_res) return std::unexpected(type_res.error());
        type_name = *type_res;
    }

    auto eq = expect(TokenType::ASSIGN, "ожидается '=' после имени переменной");//обязатльное =
    if (!eq) return std::unexpected(eq.error());
 
    auto init_res = parseExpr();// обязательное начальное значение
    if (!init_res) return std::unexpected(init_res.error());

    if (!check(TokenType::NEWLINE) && !check(TokenType::EOF_TOKEN))
        return std::unexpected(makeDiag("ожидается перенос строки после объявления переменной"));
 
    auto node = std::make_unique<VarDecl>();
    node->pos = pos;
    node->is_mut = is_mut;
    node->name = name;
    node->type_name = type_name;
    node->init = std::move(*init_res);
    return node;
}

std::expected<StmtPtr, Diagnostic> Parser::parseAssignOrExprStmt() {
    auto pos = Position{current().line, current().col};
    if (check(TokenType::IDENTIFIER) &&// если след токен = то присваивание
        peek(1).type == TokenType::ASSIGN)
    {
        std::string name = current().value;
        advance();//имя
        advance(); //=

        auto val_res = parseExpr();
        if (!val_res) return std::unexpected(val_res.error());
 
        if (!check(TokenType::NEWLINE) && !check(TokenType::EOF_TOKEN))
            return std::unexpected(makeDiag("ожидается перенос строки после присваивания"));
 
        auto node = std::make_unique<Assign>();
        node->pos = pos;
        node->name = name;
        node->value = std::move(*val_res);
        return node;
    }
    auto expr_res = parseExpr();//иначе выражение как инструкция
    if (!expr_res) return std::unexpected(expr_res.error());
 
    if (!check(TokenType::NEWLINE) && !check(TokenType::EOF_TOKEN))
        return std::unexpected(makeDiag("ожидается перенос строки после выражения"));
 
    auto node = std::make_unique<ExprStmt>();
    node->pos = pos;
    node->expr = std::move(*expr_res);
    return node;
}
 
std::expected<StmtPtr, Diagnostic> Parser::parseReturn() {
    auto pos = Position{current().line, current().col};
    advance(); //return
 
    auto node = std::make_unique<Return>();
    node->pos = pos;
 
    if (!check(TokenType::NEWLINE) && !check(TokenType::EOF_TOKEN)) {// если после return не перенос строки читаем выражение
        auto val_res = parseExpr();
        if (!val_res) return std::unexpected(val_res.error());
        node->value = std::move(*val_res);
    }
    //иначе return без значения void то value остаётся nullptr
    return node;
}

std::expected<StmtPtr, Diagnostic> Parser::parseIf() {
    auto pos = Position{current().line, current().col};
    advance();
 
    auto cond_res = parseExpr();//условие выражение
    if (!cond_res) return std::unexpected(cond_res.error());
 
    skipNewlines(); //перенос строки перед { допускается
 
    auto then_res = parseBlock();//тело if обязательный блок
    if (!then_res) return std::unexpected(then_res.error());
 
    auto node = std::make_unique<If>();
    node->pos = pos;
    node->condition = std::move(*cond_res);
    node->then_block = std::move(*then_res);
 
    skipNewlines();// необязательный else
    if (check(TokenType::ELSE)) {
        advance();
        skipNewlines();
        auto else_res = parseBlock();
        if (!else_res) return std::unexpected(else_res.error());
        node->else_block = std::move(*else_res);
    }
    return node;
}

std::expected<StmtPtr, Diagnostic> Parser::parseWhile() {
    auto pos = Position{current().line, current().col};
    advance(); // съедаем "while"
 
    // условие
    auto cond_res = parseExpr();
    if (!cond_res) return std::unexpected(cond_res.error());
 
    skipNewlines();
 
    // тело цикла
    auto body_res = parseBlock();
    if (!body_res) return std::unexpected(body_res.error());
 
    auto node = std::make_unique<While>();
    node->pos = pos;
    node->condition = std::move(*cond_res);
    node->body = std::move(*body_res);
    return node;
}
 
std::expected<StmtPtr, Diagnostic> Parser::parseFor() {
    auto pos = Position{current().line, current().col};
    advance();
    // переменная итерации
    if (!check(TokenType::IDENTIFIER))
        return std::unexpected(makeDiag("ожидается имя переменной после 'for'"));
    std::string var_name = current().value;
    advance();
    // обязательное "in"
    auto in = expect(TokenType::IN, "ожидается 'in' после имени переменной");
    if (!in) return std::unexpected(in.error());
    // имя массива
    if (!check(TokenType::IDENTIFIER))
        return std::unexpected(makeDiag("ожидается имя массива после 'in'"));
    std::string iter_name = current().value;
    advance();
 
    skipNewlines();
    // тело цикла
    auto body_res = parseBlock();
    if (!body_res) return std::unexpected(body_res.error());
 
    auto node = std::make_unique<For>();
    node->pos = pos;
    node->var_name = var_name;
    node->iter_name = iter_name;
    node->body = std::move(*body_res);
    return node;
}
 
std::expected<StmtPtr, Diagnostic> Parser::parseMatch() {
    auto pos = Position{current().line, current().col};
    advance();
    auto expr_res = parseExpr();// выражение которое сопоставляем
    if (!expr_res) return std::unexpected(expr_res.error());
 
    skipNewlines();
    auto lb = expect(TokenType::LBRACE, "ожидается '{' после выражения match");// обязательная {
    if (!lb) return std::unexpected(lb.error());
 
    auto node = std::make_unique<Match>();
    node->pos = pos;
    node->expr = std::move(*expr_res);
 
    skipNewlines();
    
    while (check(TokenType::CASE)) {// читаем варианты: case pattern -> stmt
        advance();
 
        MatchArm arm;
        arm.is_wildcard = false;
 
        if (check(TokenType::IDENTIFIER) && current().value == "_") {
            arm.is_wildcard = true; // универсальный шаблон
            advance();
        } else {
            auto pat_res = parseExpr();// обычный образец — выражение
            if (!pat_res) return std::unexpected(pat_res.error());
            arm.pattern = std::move(*pat_res);
        }
 
        auto arrow = expect(TokenType::ARROW, "ожидается '->' после образца");// обязательный "->"
        if (!arrow) return std::unexpected(arrow.error());
 
        auto body_res = parseStmt();// тело варианта одна инструкция
        if (!body_res) return std::unexpected(body_res.error());
        arm.body = std::move(*body_res);
 
        node->arms.push_back(std::move(arm));
        skipNewlines();
    }
 
    auto rb = expect(TokenType::RBRACE, "ожидается '}' после вариантов match");// обязательная }
    if (!rb) return std::unexpected(rb.error());
 
    return node;
}
 
}

// Разбор объявлений верхнего уровня
namespace parser {
using lexer::Token;
using lexer::TokenType;
// parseDecl См на текущий токен и выбирает нужный метод
std::expected<DeclPtr, Diagnostic> Parser::parseDecl() {
    using TT = TokenType;
    skipNewlines();
 
    if (check(TT::FUNC))      {
        auto r = parseFuncDef();
        if (!r) return std::unexpected(r.error());
        return r;
    }
    if (check(TT::STRUCT))    {
        auto r = parseStructDecl();
        if (!r) return std::unexpected(r.error());
        return r;
    }
    if (check(TT::ENUM))      {
        auto r = parseEnumDecl();
        if (!r) return std::unexpected(r.error());
        return r;
    }
    if (check(TT::IMPL))      {
        auto r = parseImplDecl();
        if (!r) return std::unexpected(r.error());
        return r;
    }
    if (check(TT::NAMESPACE)) {
        auto r = parseNamespaceDecl();
        if (!r) return std::unexpected(r.error());
        return r;
    }
    if (check(TT::TYPE))      {
        auto r = parseTypeAlias();
        if (!r) return std::unexpected(r.error());
        return r;
    }
 
    return std::unexpected(makeDiag(
        "ожидается объявление (func/struct/enum/impl/namespace/type), получен: '"
        + current().value + "'"));
}

std::expected<std::unique_ptr<FuncDef>, Diagnostic> Parser::parseFuncDef() {
    auto pos = Position{current().line, current().col};
    advance(); // съедаем "func"
 
    // обязательное имя функции
    if (!check(TokenType::IDENTIFIER))
        return std::unexpected(makeDiag("ожидается имя функции"));
    std::string name = current().value;
    advance(); // съедаем имя
 
    // обязательная "("
    auto lp = expect(TokenType::LPAREN, "ожидается '(' после имени функции");
    if (!lp) return std::unexpected(lp.error());
 
    // параметры: name: type, name: type, ...
    std::vector<Param> params;
    if (!check(TokenType::RPAREN)) {
        while (true) {
            // имя параметра
            if (!check(TokenType::IDENTIFIER))
                return std::unexpected(makeDiag("ожидается имя параметра"));
            std::string param_name = current().value;
            advance();
 
            // обязательное ":"
            auto colon = expect(TokenType::COLON, "ожидается ':' после имени параметра");
            if (!colon) return std::unexpected(colon.error());
 
            // тип параметра
            auto type_res = parseType();
            if (!type_res) return std::unexpected(type_res.error());
 
            params.push_back(Param{param_name, *type_res});
 
            // ещё параметры через запятую?
            if (!match(TokenType::COMMA)) break;
        }
    }
 
    // обязательная ")"
    auto rp = expect(TokenType::RPAREN, "ожидается ')' после параметров");
    if (!rp) return std::unexpected(rp.error());
 
    // необязательный тип возврата: -> type
    std::string return_type;
    if (match(TokenType::ARROW)) {
        auto type_res = parseType();
        if (!type_res) return std::unexpected(type_res.error());
        return_type = *type_res;
    }
 
    skipNewlines(); // перенос строки перед "{" допустим
 
    // тело функции — обязательный блок
    auto body_res = parseBlock();
    if (!body_res) return std::unexpected(body_res.error());
 
    auto node = std::make_unique<FuncDef>();
    node->pos = pos;
    node->name = name;
    node->params = std::move(params);
    node->return_type = return_type;
    node->body = std::move(*body_res);
    return node;
}
 
std::expected<std::unique_ptr<StructDecl>, Diagnostic> Parser::parseStructDecl() {
    auto pos = Position{current().line, current().col};
    advance(); // съедаем "struct"
 
    // обязательное имя структуры
    if (!check(TokenType::IDENTIFIER))
        return std::unexpected(makeDiag("ожидается имя структуры"));
    std::string name = current().value;
    advance();
 
    skipNewlines();
 
    // обязательная "{"
    auto lb = expect(TokenType::LBRACE, "ожидается '{' после имени структуры");
    if (!lb) return std::unexpected(lb.error());
 
    auto node = std::make_unique<StructDecl>();
    node->pos = pos;
    node->name = name;
 
    skipNewlines();

    while (!check(TokenType::RBRACE) &&// читаем поля: name: type;
           !check(TokenType::EOF_TOKEN))
    {
        // имя поля
        if (!check(TokenType::IDENTIFIER))
            return std::unexpected(makeDiag("ожидается имя поля"));
        std::string field_name = current().value;
        advance();
 
        // обязательное ":"
        auto colon = expect(TokenType::COLON, "ожидается ':' после имени поля");
        if (!colon) return std::unexpected(colon.error());
 
        // тип поля
        auto type_res = parseType();
        if (!type_res) return std::unexpected(type_res.error());
 
        // обязательная ";" после поля
        auto sc = expect(TokenType::SEMICOLON, "ожидается ';' после поля структуры");
        if (!sc) return std::unexpected(sc.error());
 
        node->fields.push_back(FieldDecl{field_name, *type_res});
        skipNewlines();
    }
 
    // обязательная "}"
    auto rb = expect(TokenType::RBRACE, "ожидается '}' после полей структуры");
    if (!rb) return std::unexpected(rb.error());
 
    return node;
}

std::expected<std::unique_ptr<EnumDecl>, Diagnostic> Parser::parseEnumDecl() {
    auto pos = Position{current().line, current().col};
    advance(); // съедаем "enum"
 
    // обязательное имя
    if (!check(TokenType::IDENTIFIER))
        return std::unexpected(makeDiag("ожидается имя перечисления"));
    std::string name = current().value;
    advance();
 
    skipNewlines();
 
    // обязательная "{"
    auto lb = expect(TokenType::LBRACE, "ожидается '{' после имени enum");
    if (!lb) return std::unexpected(lb.error());
 
    auto node = std::make_unique<EnumDecl>();
    node->pos = pos;
    node->name = name;
 
    skipNewlines();
 
    // читаем варианты: Name;
    while (!check(TokenType::RBRACE) &&
           !check(TokenType::EOF_TOKEN))
    {
        if (!check(TokenType::IDENTIFIER))
            return std::unexpected(makeDiag("ожидается имя варианта"));
        std::string variant_name = current().value;
        advance();
 
        // обязательная ";" после варианта
        auto sc = expect(TokenType::SEMICOLON, "ожидается ';' после варианта enum");
        if (!sc) return std::unexpected(sc.error());
 
        node->variants.push_back(EnumVariant{variant_name});
        skipNewlines();
    }
 
    // обязательная "}"
    auto rb = expect(TokenType::RBRACE, "ожидается '}' после вариантов enum");
    if (!rb) return std::unexpected(rb.error());
 
    return node;
}
 
std::expected<std::unique_ptr<ImplDecl>, Diagnostic> Parser::parseImplDecl() {
    auto pos = Position{current().line, current().col};
    advance(); // съедаем "impl"
 
    // обязательное имя типа
    if (!check(TokenType::IDENTIFIER))
        return std::unexpected(makeDiag("ожидается имя типа после 'impl'"));
    std::string name = current().value;
    advance();
 
    skipNewlines();
 
    // обязательная "{"
    auto lb = expect(TokenType::LBRACE, "ожидается '{' после имени impl");
    if (!lb) return std::unexpected(lb.error());
 
    auto node = std::make_unique<ImplDecl>();
    node->pos = pos;
    node->name = name;
 
    skipNewlines();
 
    // читаем только функции внутри impl
    while (!check(TokenType::RBRACE) &&
           !check(TokenType::EOF_TOKEN))
    {
        if (!check(TokenType::FUNC))
            return std::unexpected(makeDiag("внутри impl разрешены только функции"));
 
        auto func_res = parseFuncDef();
        if (!func_res) return std::unexpected(func_res.error());
        node->methods.push_back(std::move(*func_res));
        skipNewlines();
    }
 
    // обязательная "}"
    auto rb = expect(TokenType::RBRACE, "ожидается '}' после методов impl");
    if (!rb) return std::unexpected(rb.error());
 
    return node;
}
 
std::expected<std::unique_ptr<NamespaceDecl>, Diagnostic> Parser::parseNamespaceDecl() {
    auto pos = Position{current().line, current().col};
    advance(); // съедаем "namespace"
 
    // обязательное имя
    if (!check(TokenType::IDENTIFIER))
        return std::unexpected(makeDiag("ожидается имя namespace"));
    std::string name = current().value;
    advance();
 
    skipNewlines();
 
    // обязательная "{"
    auto lb = expect(TokenType::LBRACE, "ожидается '{' после имени namespace");
    if (!lb) return std::unexpected(lb.error());
 
    auto node = std::make_unique<NamespaceDecl>();
    node->pos = pos;
    node->name = name;
 
    skipNewlines();
 
    // читаем только функции
    while (!check(TokenType::RBRACE) &&
           !check(TokenType::EOF_TOKEN))
    {
        if (!check(TokenType::FUNC))
            return std::unexpected(makeDiag("внутри namespace разрешены только функции"));
 
        auto func_res = parseFuncDef();
        if (!func_res) return std::unexpected(func_res.error());
        node->funcs.push_back(std::move(*func_res));
        skipNewlines();
    }
 
    // обязательная "}"
    auto rb = expect(TokenType::RBRACE, "ожидается '}' после namespace");
    if (!rb) return std::unexpected(rb.error());
 
    return node;
}

std::expected<std::unique_ptr<TypeAlias>, Diagnostic> Parser::parseTypeAlias() {
    auto pos = Position{current().line, current().col};
    advance(); // съедаем "type"
 
    // обязательное новое имя
    if (!check(TokenType::IDENTIFIER))
        return std::unexpected(makeDiag("ожидается имя синонима типа"));
    std::string name = current().value;
    advance();
 
    // обязательное "="
    auto eq = expect(TokenType::ASSIGN, "ожидается '=' после имени типа");
    if (!eq) return std::unexpected(eq.error());
 
    // обязательный существующий тип
    auto type_res = parseType();
    if (!type_res) return std::unexpected(type_res.error());
 
    // перенос строки в конце
    if (!check(TokenType::NEWLINE) && !check(TokenType::EOF_TOKEN))
        return std::unexpected(makeDiag("ожидается перенос строки после объявления типа"));
 
    auto node = std::make_unique<TypeAlias>();
    node->pos = pos;
    node->name = name;
    node->type_name = *type_res;
    return node;
}
 
//изм parse -главный методгде разбираем всю программу
// Программа -список объявлений верхнего уровня до EOF
Parser::ParseResult Parser::parse() {
    Program program;
    skipNewlines();
    while (!check(TokenType::EOF_TOKEN)) {
        auto decl_res = parseDecl();
        if (!decl_res) {
            diagnostics_.push_back(decl_res.error()); //записываем ошибку
            synchronize();//ищем следующий якорь
        } else {
            program.decls.push_back(std::move(*decl_res));
        }
        skipNewlines();
    }
    return ParseResult{ std::move(program), std::move(diagnostics_) };
}

}
