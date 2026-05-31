#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
import tokens;
import lexer;
import parser;
import ast;
import semantic;
import interpreter;

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "error: не удалось открыть файл '" << path << "'\n";
        std::exit(1);
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void dumpTokens(const std::vector<lexer::Token>& tokens) {
    for (auto& t : tokens)
        std::cout << t.toString() << "\n";
}

static void dumpAst(const parser::Program& program) {
    for (auto& d : program.decls) {
        if (auto* fd = dynamic_cast<parser::FuncDef*>(d.get()))
            std::cout << "FuncDef: " << fd->name << "\n";
        else if (auto* sd = dynamic_cast<parser::StructDecl*>(d.get()))
            std::cout << "StructDecl: " << sd->name << "\n";
        else if (auto* ed = dynamic_cast<parser::EnumDecl*>(d.get()))
            std::cout << "EnumDecl: " << ed->name << "\n";
        else if (auto* td = dynamic_cast<parser::TypeAlias*>(d.get()))
            std::cout << "TypeAlias: " << td->name << " = " << td->type_name << "\n";
        else if (auto* nd = dynamic_cast<parser::NamespaceDecl*>(d.get()))
            std::cout << "NamespaceDecl: " << nd->name << "\n";
        else if (auto* id = dynamic_cast<parser::ImplDecl*>(d.get()))
            std::cout << "ImplDecl: " << id->name << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::string source_file;
    bool dump_tokens = false;
    bool dump_ast    = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dump-tokens")
            dump_tokens = true;
        else if (arg == "--dump-ast")
            dump_ast = true;
        else if (arg[0] != '-')
            source_file = arg;
    }

    if (source_file.empty()) {
        std::cerr << "использование: mypy <source file> [--dump-tokens] [--dump-ast]\n";
        return 1;
    }

    std::string source = readFile(source_file);

    //1 лексер
    lexer::Lexer lex(source, source_file);
    auto tok_result = lex.tokenize();
    if (!tok_result) {
        tok_result.error().print();
        return 1;
    }
    auto& tokens = *tok_result;

    if (dump_tokens) {
        dumpTokens(tokens);
        return 0;
    }

    // 2 парсер
    parser::Parser p(std::move(tokens), source_file);
    auto parse_result = p.parse();

    if (!parse_result.ok()) {
        for (auto& e : parse_result.errors)
            e.print();
        return 1;
    }

    if (dump_ast) {
        dumpAst(parse_result.program);
        return 0;
    }

    //3 семантика
    semantic::SemanticAnalyzer sa(source_file);
    auto sem_result = sa.analyze(parse_result.program);

    if (!sem_result.ok()) {
        for (auto& e : sem_result.errors)
            e.print();
        return 1;
    }

    // 4 интерпретатор
    interpreter::Interpreter interp(parse_result.program);
    return interp.run();
}