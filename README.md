# mypy

Интерпретатор языка PyRust, реализованный на C++23.

## Сборка
cmake -B build
cmake --build build

## Запуск
./build/mypy <файл.pyrust>

## Флаги
--dump-tokens  вывести токены
--dump-ast     вывести AST
