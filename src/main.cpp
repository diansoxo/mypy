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

static void runRepl() {//доп3
    std::cout << "MyLang REPL. Введите 'exit(0)' для выхода.\n";
    
    // накапливаем все объявления между итерациями
    parser::Program accumulated;
    
    while (true) {
        std::cout << ">>> ";
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        
        lexer::Lexer lex(line, "<repl>");
        auto tok_result = lex.tokenize();
        if (!tok_result) {
            tok_result.error().print();
            continue;
        }
        
        parser::Parser p(std::move(*tok_result), "<repl>");
        auto parse_result = p.parse();
        if (!parse_result.ok()) {
            for (auto& e : parse_result.errors) e.print();
            continue;
        }
        
        // добавляем новые объявления к накопленным
        for (auto& d : parse_result.program.decls)
            accumulated.decls.push_back(std::move(d));
        
        semantic::SemanticAnalyzer sa("<repl>");
        auto sem_result = sa.analyze(accumulated);
        if (!sem_result.ok()) {
            // откатываем последнее добавленное
            for (size_t i = 0; i < parse_result.program.decls.size(); ++i)
                accumulated.decls.pop_back();
            for (auto& e : sem_result.errors) e.print();
            continue;
        }
        
        interpreter::Interpreter interp(accumulated);
        interp.run();
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
        runRepl();//доп3
        return 0;
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
