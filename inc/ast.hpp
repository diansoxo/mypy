#pragma once
// Абстрактное синтаксическое дерево (AST)
// Каждый узел — отдельный класс, наследуется от базового Expr или Stmt
// Узлы хранят дочерние узлы через unique_ptr (владение памятью)

#include <string>
#include <vector>
#include <memory>

namespace parser {
struct Position {// Каждый узел хранит позицию в исходном коде (для диагностики)
    int line;
    int col;
};

// Выражение — это то что вычисляется в значение
struct Expr {
    Position pos;
    virtual ~Expr() = default;
};

// Инструкция — это то что выполняется
struct Stmt {
    Position pos;
    virtual ~Stmt() = default;
};

// Удобный псевдоним владеющий указатель на выражение
// unique_ptr означает память освобождается автоматически
using ExprPtr = std::unique_ptr<Expr>;//псевдонимы
using StmtPtr = std::unique_ptr<Stmt>;

// Узлы выражений

struct IntLiteral : Expr {
    int64_t value;
};

struct FloatLiteral : Expr {
    double value;
};

struct StringLiteral : Expr {
    std::string value;  // содержимое строки без кавычек
};

struct BoolLiteral : Expr {
    bool value;
};

struct Identifier : Expr {
    std::string name;
};

struct BinaryOp : Expr {
    std::string op;// "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=", "and", "or"
    ExprPtr left;
    ExprPtr right;
};

struct UnaryOp : Expr {
    std::string op; // "-" или "not"
    ExprPtr operand;
};

struct Call : Expr {
    std::string name;
    std::vector<ExprPtr> args;
};

struct ArrayAccess : Expr {
    std::string name;
    ExprPtr index;
};

struct ArrayLiteral : Expr {
    std::vector<ExprPtr> elements;//элементы массива
};

struct FieldAccess : Expr {
    ExprPtr object; // объект
    std::string field;// имя поля
};

struct Cast : Expr {
    ExprPtr expr;// выражение которое приводим
    std::string type;// имя целевого типа
};

struct FieldInit {
    std::string name; // имя поля
    ExprPtr value; // значение
};

struct StructLiteral : Expr {
    std::string name;// имя структуры
    std::vector<FieldInit> fields;// инициализаторы полей
};

struct EnumLiteral : Expr {
    std::string enum_name;
    std::string variant_name;
};

struct TupleLiteral : Expr {
    std::vector<ExprPtr> elements;
};

// Узлы инструкций

// Блок инструкций: { stmt1 stmt2 ... }
// Это тело функции, тело if, тело while и т.д.
struct Block : Stmt {
    std::vector<StmtPtr> stmts;  // список инструкций внутри блока
};
 
// Объявление переменной: let [mut] name [: type] = expr
struct VarDecl : Stmt {
    bool is_mut;
    std::string name;
    std::string type_name;
    ExprPtr init;
};
 
struct Assign : Stmt {
    std::string name;// имя переменной
    ExprPtr value;// новое значение
};
 
struct Return : Stmt {
    ExprPtr value;  // nullptr если return без значения
};
 
struct Break : Stmt {};
 
struct Continue : Stmt {};

// Используется когда нужно пустое тело блока
struct Pass : Stmt {};
 
// Инструкция-выражение: expr
// например print("hello")
struct ExprStmt : Stmt {
    ExprPtr expr; //выражение (обычно вызов функции)
};
 
struct If : Stmt {
    ExprPtr condition;// условие
    std::unique_ptr<Block> then_block;// if
    std::unique_ptr<Block> else_block;// тело else, nullptr если нет else
};
 
struct While : Stmt {
    ExprPtr condition;// условие продолжения цикла
    std::unique_ptr<Block> body;// тело цикла
};
 
struct For : Stmt {
    std::string var_name;// переменная итерации (x)
    std::string iter_name;// имя массива (arr)
    std::unique_ptr<Block> body;// тело цикла
};
 
struct MatchArm {
    ExprPtr pattern;// образец: литерал, идентификатор, или nullptr если "_"
    bool is_wildcard;// true если образец "_"
    StmtPtr body;// инструкция которую выполняем
};
 
struct Match : Stmt {
    ExprPtr expr;// выражение которое сопоставляем
    std::vector<MatchArm> arms;// варианты
};
 
// Узлы объявлений

struct Decl {// Базовый класс для всех объявлений
    Position pos;
    virtual ~Decl() = default;
};

using DeclPtr = std::unique_ptr<Decl>;
 
// Параметр функции: name : type
struct Param {
    std::string name;
    std::string type_name;
};
 
struct FuncDef : Decl {
    std::string name;
    std::vector<Param> params;
    std::string return_type;
    std::unique_ptr<Block> body;
};
 
struct FieldDecl {
    std::string name;
    std::string type_name;
};

struct StructDecl: Decl {
    std::string name;
    std::vector<FieldDecl> fields;
};
 
// Вариант перечисления: Name;
struct EnumVariant {
    std::string name;   // имя варианта
};
 
struct EnumDecl : Decl {
    std::string name;                       // имя перечисления
    std::vector<EnumVariant> variants;      // варианты
};
 
// Реализация методов: impl Name { func ... func ... }
// Пример:
//   impl Point {
//       func distance(p: Point) -> float64 { ... }
//   }
struct ImplDecl : Decl {
    std::string name;
    std::vector<std::unique_ptr<FuncDef>> methods;
};
 
struct NamespaceDecl : Decl {
    std::string name; 
    std::vector<std::unique_ptr<FuncDef>> funcs;
};
 
struct TypeAlias : Decl {
    std::string name;
    std::string type_name; 
};
 
// Корень дерева вся программа
// Программа список объявлений верхнего уровня
// Инструкции и выражения допускаются только внутри функций
struct Program {//список всех объявлений в файле
    std::vector<DeclPtr> decls;
};
}
