cmake_minimum_required(VERSION 3.28)
project(mypy CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-stdlib=libc++ -I/usr/lib/llvm-18/include/c++/v1)
add_link_options(-stdlib=libc++)


add_executable(test_lexer src/test_lexer.cpp)
target_sources(test_lexer
    PUBLIC FILE_SET CXX_MODULES FILES
    src/tokens.cppm
    src/ast.cppm
    src/lexer.cppm
)


add_executable(test_parser src/test_parser.cpp)
target_sources(test_parser
    PUBLIC FILE_SET CXX_MODULES FILES
        src/tokens.cppm
        src/ast.cppm
        src/lexer.cppm
        src/parser.cppm
)

# enable_testing() позволяет запускать через ctest
enable_testing()
add_test(NAME LexerTests COMMAND test_lexer)
add_test(NAME ParserTests COMMAND test_parser)

add_executable(test_semantic src/test_semantic.cpp)
target_sources(test_semantic
    PUBLIC FILE_SET CXX_MODULES FILES
        src/tokens.cppm
        src/ast.cppm
        src/lexer.cppm
        src/parser.cppm
        src/semantic.cppm
)
add_test(NAME SemanticTests COMMAND test_semantic)

add_executable(mypy src/main.cpp)
target_sources(mypy
    PUBLIC FILE_SET CXX_MODULES FILES
        src/tokens.cppm
        src/ast.cppm
        src/lexer.cppm
        src/parser.cppm
        src/semantic.cppm
        src/interpreter.cppm
)
