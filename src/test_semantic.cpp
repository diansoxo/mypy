#include <cassert>
#include <iostream>
import lexer;
import parser;
import ast;
import semantic;

static semantic::AnalysisResult analyze(const std::string& src) {
    lexer::Lexer lex(src, "<test>");
    auto tok_res = lex.tokenize();
    assert(tok_res && "токенизация упала");
    parser::Parser p(std::move(*tok_res), "<test>");
    auto parse_result = p.parse();
    semantic::SemanticAnalyzer sa("<test>");
    return sa.analyze(parse_result.program);
}

// Тест 1: переменная объявлена и используется
void test_var_declared() {
    auto r = analyze("func main() {\nlet x: int32 = 42\nlet y: int32 = x\n}\n");
    assert(r.ok());
    std::cout << "[SEMANTIC] test_var_declared PASSED\n";
}

// Тест 2: переменная не объявлена
void test_var_undeclared() {
    auto r = analyze("func main() {\nlet y: int32 = x\n}\n");
    assert(!r.ok());
    std::cout << "[SEMANTIC] test_var_undeclared PASSED\n";
}

// Тест 3: присваивание неизменяемой переменной
void test_assign_immutable() {
    auto r = analyze("func main() {\nlet x: int32 = 1\nx = 2\n}\n");
    assert(!r.ok());
    std::cout << "[SEMANTIC] test_assign_immutable PASSED\n";
}

// Тест 4: присваивание mut переменной
void test_assign_mutable() {
    auto r = analyze("func main() {\nlet mut x: int32 = 1\nx = 2\n}\n");
    assert(r.ok());
    std::cout << "[SEMANTIC] test_assign_mutable PASSED\n";
}

// Тест 5: условие if должно быть bool
void test_if_condition_bool() {
    auto r = analyze("func main() {\nlet x: int32 = 1\nif x {\n}\n}\n");
    std::cerr << "errors: " << r.errors.size() << "\n";
    for (auto& e : r.errors)
        std::cerr << "  " << e.format() << "\n";
    assert(!r.ok());
    std::cout << "[SEMANTIC] test_if_condition_bool PASSED\n";
}

// Тест 6: корректное условие if
void test_if_condition_ok() {
    auto r = analyze("func main() {\nlet x: bool = true\nif x {\n}\n}\n");
    assert(r.ok());
    std::cout << "[SEMANTIC] test_if_condition_ok PASSED\n";
}

// Тест 7: функция с неверным типом аргумента
void test_call_wrong_arg_type() {
    auto r = analyze("func add(a: int32, b: int32) -> int32 {\nreturn a\n}\nfunc main() {\nadd(1, true)\n}\n");
    assert(!r.ok());
    std::cout << "[SEMANTIC] test_call_wrong_arg_type PASSED\n";
}

// Тест 8: функция с верными аргументами
void test_call_correct() {
    auto r = analyze("func add(a: int32, b: int32) -> int32 {\nreturn a\n}\nfunc main() {\nadd(1, 2)\n}\n");
    assert(r.ok());
    std::cout << "[SEMANTIC] test_call_correct PASSED\n";
}

// Тест 9: неверный тип возврата
void test_return_wrong_type() {
    auto r = analyze("func foo() -> int32 {\nreturn true\n}\n");
    assert(!r.ok());
    std::cout << "[SEMANTIC] test_return_wrong_type PASSED\n";
}

// Тест 10: break вне цикла
void test_break_outside_loop() {
    auto r = analyze("func main() {\nbreak\n}\n");
    assert(!r.ok());
    std::cout << "[SEMANTIC] test_break_outside_loop PASSED\n";
}

// Тест 11: повторное объявление переменной
void test_var_redeclared() {
    auto r = analyze("func main() {\nlet x: int32 = 1\nlet x: int32 = 2\n}\n");
    assert(!r.ok());
    std::cout << "[SEMANTIC] test_var_redeclared PASSED\n";
}

// Тест 12: несовместимые типы при объявлении
void test_var_type_mismatch() {
    auto r = analyze("func main() {\nlet x: int32 = true\n}\n");
    assert(!r.ok());
    std::cout << "[SEMANTIC] test_var_type_mismatch PASSED\n";
}

int main() {
    test_var_declared();
    test_var_undeclared();
    test_assign_immutable();
    test_assign_mutable();
    test_if_condition_bool();
    test_if_condition_ok();
    test_call_wrong_arg_type();
    test_call_correct();
    test_return_wrong_type();
    test_break_outside_loop();
    test_var_redeclared();
    test_var_type_mismatch();
    std::cout << "\n[SEMANTIC] Все тесты прошли.\n";
    return 0;
}
