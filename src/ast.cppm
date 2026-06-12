module;

#include <string>
#include <vector>
#include <memory>
export module ast;

export namespace parser {
struct Position {// Каждый узел хранит позицию в исходном коде (для диагностики)
    int line;
    int col;
};

struct Node {//единый корневой узел
    Position pos;//изм
    virtual ~Node() = default;
};

using NodePtr = std::unique_ptr<Node>;

struct Expr : Node {};//изм
struct Stmt : Node {};
struct Decl : Node {};
// Удобный псевдоним владеющий указатель на выражение
// unique_ptr означает память освобождается автоматически
using ExprPtr = std::unique_ptr<Expr>;//псевдонимы
using StmtPtr = std::unique_ptr<Stmt>;
using DeclPtr = std::unique_ptr<Decl>;//изм

struct VarDecl : Stmt {//изм
    bool is_mut;
    std::string name;
    std::string type_name;
    ExprPtr init;

    VarDecl(Position p, bool is_mut, std::string name, std::string type_name, ExprPtr init)
        : is_mut(is_mut)
        , name(std::move(name))
        , type_name(std::move(type_name))
        , init(std::move(init))
    {
        pos = p;
    }
};

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

struct CharLiteral : Expr {
    char value;
};

struct Identifier : Expr {
    std::string name;
};

enum class BinOp {//изм
    Add, Sub, Mul, Div, Mod,
    Eq, Ne, Lt, Gt, Le, Ge,
    And, Or
};
struct BinaryOp : Expr {//изм
    BinOp op;
    ExprPtr left;
    ExprPtr right;
};

enum class UnOp {//изм
    Neg, // -
    Not // not
};

struct UnaryOp : Expr {//изм
    UnOp op;
    ExprPtr operand;
};

struct Call : Expr {//изм
    ExprPtr callee;//любое выражение
    std::vector<ExprPtr> args;
};

struct ArrayAccess: Expr {//изм
    ExprPtr base;// любое выражение
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
struct Block : Stmt {
    std::vector<StmtPtr> stmts;  // список инструкций внутри блока
};
 
struct Assign : Stmt {
    ExprPtr target;
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
// Параметр функции: name : type
struct Param {
    std::string name;
    std::string type_name;
    ExprPtr default_value; //доп2 nullptr если нет дефолта
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
    std::string name; // имя варианта
};
 
struct EnumDecl : Decl {
    std::string name;
    std::vector<EnumVariant> variants; // варианты
};
 
// Реализация методов: impl Name { func ... func ... }
struct ImplDecl : Decl {
    std::string name;
    std::vector<DeclPtr> methods;//изм
};
 
struct NamespaceDecl : Decl {
    std::string name; 
    std::vector<DeclPtr> decls;//изм
};

struct Lambda : Expr {// лямбда-выражение: fn(x: int32) -> int32 { return x + 1 } доп5
    std::vector<Param> params;
    std::string return_type;
    std::unique_ptr<Block> body;
};
 
struct TypeAlias : Decl {
    std::string name;
    std::string type_name; 
};
 
struct Program {//список всех объявлений в файле
    std::vector<DeclPtr> decls;
};
}
