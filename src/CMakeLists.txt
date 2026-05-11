cmake_minimum_required(VERSION 3.20)
project(LexerParserTests CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# заголовки лежат в ../inc
set(INC_DIR ${CMAKE_SOURCE_DIR}/../inc)

# общие исходники нужные обоим тестам
set(COMMON_SOURCES
    lexer.cpp
    tokens.cpp
)

# -------------------------------------------------------
# Тест лексера — только lexer.cpp + tokens.cpp
# -------------------------------------------------------
add_executable(test_lexer
    test_lexer.cpp
    ${COMMON_SOURCES}
)
target_include_directories(test_lexer PRIVATE ${INC_DIR})

# -------------------------------------------------------
# Тест парсера — lexer.cpp + parser.cpp + tokens.cpp
# -------------------------------------------------------
add_executable(test_parser
    test_parser.cpp
    ${COMMON_SOURCES}
    parser.cpp
)
target_include_directories(test_parser PRIVATE ${INC_DIR})

# -------------------------------------------------------
# enable_testing() позволяет запускать через ctest
# -------------------------------------------------------
enable_testing()
add_test(NAME LexerTests  COMMAND test_lexer)
add_test(NAME ParserTests COMMAND test_parser)