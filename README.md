# MyPy — интерпретатор языка программирования PyRust

MyPy — учебный интерпретатор собственного языка программирования **PyRust**, реализованный на **C++23**.

Язык сочетает Python-подобный синтаксис с явной статической типизацией в стиле Rust и предназначен для изучения принципов построения компиляторов и интерпретаторов.

## Возможности языка

### Типы данных
Встроенные типы:

* `int8`, `int16`, `int32`, `int64`
* `uint8`, `uint16`, `uint32`, `uint64`
* `float32`, `float64`
* `bool`
* `char`
* `string`
* `void`

Поддерживаются:
* десятичные целочисленные литералы
* шестнадцатеричные литералы (`0x2A`)
* двоичные литералы (`0b101010`)
* вещественные литералы
* строковые и символьные литералы
* булевы литералы (`true`, `false`)


### Пользовательские типы

#### Структуры

```pyrust
struct Point {
    x: int32
    y: int32
}
```

#### Перечисления

```pyrust
enum Color {
    Red
    Green
    Blue
}
```

#### Псевдонимы типов

```pyrust
type Distance = int32
```

### Функции
Поддерживаются:

* именованные функции
* рекурсия
* перегрузка функций
* параметры по умолчанию
* функции как значения
* лямбда-выражения

Пример:

```pyrust
func add(a: int32, b: int32 = 0) -> int32 {
    return a + b
}
```

Лямбда:

```pyrust
let inc = fn(x: int32) -> int32 {
    return x + 1
}
```

### Методы структур
Методы определяются через блоки `impl`.

```pyrust
struct Point {
    x: int32
    y: int32
}

impl Point {
    func length() -> float64 {
        return 0.0
    }
}
```

### Управление потоком

Поддерживаются:
* блоки
* `if / else`
* `while`
* `for ... in`
* `match / case`
* `break`
* `continue`
* `return`
* `pass`

Пример:

```pyrust
while i < 10 {
    println(i)
    i = i + 1
}
```

### Сопоставление с образцом

```pyrust
match value {
    case 0 {
        println("zero")
    }

    case 1 {
        println("one")
    }

    case _ {
        println("other")
    }
}
```

### Пространства имён

```pyrust
namespace Math {
    func square(x: int32) -> int32 {
        return x * x
    }
}
```

### Встроенные функции
Ввод/вывод:

* `print`
* `println`
* `input`

Диагностика и завершение программы:

* `assert`
* `panic`
* `exit`

Служебные функции:

* `len`


### Комментарии
Однострочные:
```pyrust
// комментарий
```

Вложенные блочные:
```pyrust
/*
    внешний комментарий

    /*
        вложенный комментарий
    */
*/
```


## Архитектура интерпретатора

Интерпретатор реализован как классический pipeline.

Исходный код (.pyrust)
        │
     Lexer
        │
    Tokens
        │
     Parser
        │
       AST
        │
Semantic Analyzer
        │
  Annotated AST
        │
 Interpreter
 

## Структура проекта
mypy/
├── src/
│   ├── tokens.cppm
│   ├── lexer.cppm
│   ├── ast.cppm
│   ├── parser.cppm
│   ├── semantic.cppm
│   ├── interpreter.cppm
│   └── main.cpp
│
├── specs/
│   ├── grammar.md
│   ├── semantics.md
│   ├── types.md
│   └── codegen.md
│
├── examples/
├── report.md
├── README.md
├── CMakeLists.txt
└── .gitignore

## Сборка
* CMake ≥ 3.28
* Компилятор с поддержкой C++23

Проверенные компиляторы:

* clang++ 17+
* g++ 13+

Сборка:

```bash
cmake -B build
cmake --build build
```

## Запуск

Запуск программы:

```bash
./build/mypy program.pyrust
```

Интерактивный режим:

```bash
./build/mypy
```

## Отладочные режимы

Вывод потока токенов:
```bash
./build/mypy program.pyrust --dump-tokens
```

Вывод AST:
```bash
./build/mypy program.pyrust --dump-ast
```

## Пример программы

```pyrust
func fibonacci(n: int32) -> int32 {
    if n <= 1 {
        return n
    }

    return fibonacci(n - 1) + fibonacci(n - 2)
}

func main() -> int32 {
    let mut i: int32 = 0

    while i < 10 {
        println(fibonacci(i))
        i = i + 1
    }

    return 0
}
```


## Диагностика ошибок

Все диагностические сообщения выводятся в формате:

```text
<file>:<line>:<column>: error: <message>
```

Пример:

```text
test.pyrust:10:15: error: undefined variable 'x'
```

Ошибки обнаруживаются на этапах:

* лексического анализа
* синтаксического анализа
* семантического анализа
* выполнения программы


## дополнительные возможности

* A.1.1 — числовые типы различных размеров
* A.1.5 — перечисления (enum)
* A.1.15 — вложенные блочные комментарии
* A.2.7 — функции как значения и лямбда-выражения
* A.2.8 — перегрузка функций
* A.2.9 — параметры по умолчанию
* A.2.12 — методы структур
