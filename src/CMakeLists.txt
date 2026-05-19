cmake_minimum_required(VERSION 3.28)
project(LexerParserTests CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(test_lexer test_lexer.cpp)
target_sources(test_lexer
    PUBLIC FILE_SET CXX_MODULES FILES
    tokens.cppm
    ast.cppm
    lexer.cppm
)


add_executable(test_parser test_parser.cpp)
target_sources(test_parser
    PUBLIC FILE_SET CXX_MODULES FILES
        tokens.cppm
        ast.cppm
        lexer.cppm
        parser.cppm
)

# enable_testing() позволяет запускать через ctest
enable_testing()
add_test(NAME LexerTests  COMMAND test_lexer)
add_test(NAME ParserTests COMMAND test_parser)

add_executable(test_semantic test_semantic.cpp)
target_sources(test_semantic
    PUBLIC FILE_SET CXX_MODULES FILES
        tokens.cppm
        ast.cppm
        lexer.cppm
        parser.cppm
        semantic.cppm
)
add_test(NAME SemanticTests COMMAND test_semantic)
