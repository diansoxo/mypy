#include <cassert>
#include <iostream>
#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"

static parser::Parser::ParseResult parse(const std::string& src) {
    lexer::Lexer lex(src, "<test>");
    auto tok_res = lex.tokenize();
    assert(tok_res && "токенизация упала");
    parser::Parser p(std::move(*tok_res), "<test>");
    return p.parse();
}

// Тест 1: простая функция без параметров
void test_func_no_params() {
    auto result = parse("func foo() {\n}\n");
    assert(result.ok());
    assert(result.program.decls.size() == 1);
    auto* fd = dynamic_cast<parser::FuncDef*>(result.program.decls[0].get());
    assert(fd != nullptr);
    assert(fd->name == "foo");
    assert(fd->params.empty());
    std::cout << "[PARSER] test_func_no_params PASSED\n";
}

// Тест 2: объявление переменной с типом и значением
void test_var_decl() {
    auto result = parse("func main() {\nlet x: int32 = 42\n}\n");
    assert(result.ok());
    auto* fd = dynamic_cast<parser::FuncDef*>(result.program.decls[0].get());
    assert(fd != nullptr);
    assert(fd->body->stmts.size() == 1);
    auto* vd = dynamic_cast<parser::VarDecl*>(fd->body->stmts[0].get());
    assert(vd != nullptr);
    assert(vd->name == "x");
    assert(vd->type_name == "int32");
    assert(!vd->is_mut);
    auto* lit = dynamic_cast<parser::IntLiteral*>(vd->init.get());
    assert(lit != nullptr);
    assert(lit->value == 42);
    std::cout << "[PARSER] test_var_decl PASSED\n";
}

// Тест 3: error recovery
void test_error_recovery() {
    // используем \n между функциями явно
    auto result = parse("func () {\n}\n\nfunc bar() {\n}\n");
    assert(!result.errors.empty());
    bool found_bar = false;
    for (auto& d : result.program.decls) {
        if (auto* fd = dynamic_cast<parser::FuncDef*>(d.get())) {
            if (fd->name == "bar") found_bar = true;
        }
    }
    assert(found_bar);
    std::cout << "[PARSER] test_error_recovery PASSED\n";
}

int main() {
    test_func_no_params();
    test_var_decl();
    test_error_recovery();
    std::cout << "\n[PARSER] Все тесты прошли.\n";
    return 0;
}