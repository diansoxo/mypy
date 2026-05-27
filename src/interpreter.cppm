module;
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <optional>
#include <iostream>
#include <cstdlib>
export module interpreter;
import ast;
import parser;

export namespace interpreter {

struct Value {
    using Array  = std::vector<Value>;
    using Struct = std::unordered_map<std::string, Value>;

    std::variant
        std::monostate,
        int64_t,
        double,
        bool,
        char,
        std::string,
        Array,
        Struct
    > data;

    Value() : data(std::monostate{}) {}
    Value(int64_t v): data(v) {}
    Value(double v): data(v) {}
    Value(bool v): data(v) {}
    Value(char v): data(v) {}
    Value(std::string v) : data(std::move(v)) {}
    Value(Array v): data(std::move(v)) {}
    Value(Struct v): data(std::move(v)) {}

    bool isVoid()const { return std::holds_alternative<std::monostate>(data); }
    bool isInt()const { return std::holds_alternative<int64_t>(data); }
    bool isFloat()const { return std::holds_alternative<double>(data); }
    bool isBool()const { return std::holds_alternative<bool>(data); }
    bool isChar()const { return std::holds_alternative<char>(data); }
    bool isString()const { return std::holds_alternative<std::string>(data); }
    bool isArray()const { return std::holds_alternative<Array>(data); }
    bool isStruct()const { return std::holds_alternative<Struct>(data); }

    int64_t asInt() const { return std::get<int64_t>(data); }
    double asFloat() const { return std::get<double>(data); }
    bool asBool() const { return std::get<bool>(data); }
    char asChar() const { return std::get<char>(data); }
    const std::string& asString() const { return std::get<std::string>(data); }
    Array& asArray() { return std::get<Array>(data); }
    const Array& asArray() const { return std::get<Array>(data); }
    Struct& asStruct() { return std::get<Struct>(data); }
    const Struct& asStruct() const { return std::get<Struct>(data); }

    std::string toString() const {
        if (isVoid()) return "void";
        if (isInt()) return std::to_string(asInt());
        if (isFloat()) return std::to_string(asFloat());
        if (isBool()) return asBool() ? "true" : "false";
        if (isChar()) return std::string(1, asChar());
        if (isString()) return asString();
        if (isArray()) {
            std::string s = "[";
            for (size_t i = 0; i < asArray().size(); ++i) {
                if (i) s += ", ";
                s += asArray()[i].toString();
            }
            return s + "]";
        }
        if (isStruct()) {
            std::string s = "{";
            bool first = true;
            for (auto& [k, v] : asStruct()) {
                if (!first) s += ", ";
                s += k + " = " + v.toString();
                first = false;
            }
            return s + "}";
        }
        return "<unknown>";
    }

    bool operator==(const Value& o) const { return data == o.data; }
    bool operator!=(const Value& o) const { return !(*this == o); }
};

struct ReturnSignal { Value value; };
struct BreakSignal {};
struct ContinueSignal {};
using Signal = std::variant<ReturnSignal, BreakSignal, ContinueSignal>;

[[noreturn]] void runtimeError(const std::string& msg, int line) {
    std::cerr << "runtime error: " << msg << " at line " << line << "\n";
    std::exit(1);
}

class Interpreter {
public:
    explicit Interpreter(const parser::Program& program);
    int run();

private:
    const parser::Program& program_;
    std::vector<std::unordered_map<std::string, Value>> scopes_;
    std::unordered_map<std::string, const parser::FuncDef*> functions_;

    void pushScope();
    void popScope();
    void declareVar(const std::string& name, Value val);
    void setVar(const std::string& name, Value val, int line);
    Value& getVar(const std::string& name, int line);

    Value evalExpr(const parser::Expr& expr);
    std::optional<Signal> execStmt(const parser::Stmt& stmt);
    std::optional<Signal> execBlock(const parser::Block& block);
    Value callFunc(const parser::FuncDef& fd, std::vector<Value> args, int line);
    Value callBuiltin(const std::string& name,
                      const std::vector<parser::ExprPtr>& arg_nodes,
                      int line);
};

// 1)области видимости
Interpreter::Interpreter(const parser::Program& program)
    : program_(program)
{}

void Interpreter::pushScope() {
    scopes_.emplace_back(); // новый пустой словарь сверху
}

void Interpreter::popScope() {
    if (!scopes_.empty())
        scopes_.pop_back();
}

void Interpreter::declareVar(const std::string& name, Value val) {
    scopes_.back()[name] = std::move(val);
}

void Interpreter::setVar(const std::string& name, Value val, int line) {
    for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {
        auto it = scopes_[i].find(name);
        if (it != scopes_[i].end()) {
            it->second = std::move(val);
            return;
        }
    }
    runtimeError("переменная '" + name + "' не найдена", line);
}

Value& Interpreter::getVar(const std::string& name, int line) {
    for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {
        auto it = scopes_[i].find(name);
        if (it != scopes_[i].end())
            return it->second;
    }
    runtimeError("переменная '" + name + "' не найдена", line);
}

// 2) конструктор и run

int Interpreter::run() {
    for (auto& d : program_.decls) {
        if (auto* fd = dynamic_cast<const parser::FuncDef*>(d.get()))
            functions_[fd->name] = fd;
    }
    auto it = functions_.find("main");
    if (it == functions_.end()) {
        std::cerr << "runtime error: функция 'main' не найдена\n";
        return 1;
    }

    Value result = callFunc(*it->second, {}, 0);

    if (result.isInt())
        return static_cast<int>(result.asInt());
    return 0;
}

//3 evalExpr
Value Interpreter::evalExpr(const parser::Expr& expr) {

    if (auto* n = dynamic_cast<const parser::IntLiteral*>(&expr))// литералы — просто возвращаем значение
        return Value(static_cast<int64_t>(n->value));

    if (auto* n = dynamic_cast<const parser::FloatLiteral*>(&expr))
        return Value(n->value);

    if (auto* n = dynamic_cast<const parser::BoolLiteral*>(&expr))
        return Value(n->value);

    if (auto* n = dynamic_cast<const parser::CharLiteral*>(&expr))
        return Value(n->value);

    if (auto* n = dynamic_cast<const parser::StringLiteral*>(&expr))
        return Value(n->value);

    if (auto* n = dynamic_cast<const parser::Identifier*>(&expr))// переменная — ищем в scopes_
        return getVar(n->name, n->pos.line);

    if (auto* n = dynamic_cast<const parser::ArrayLiteral*>(&expr)) {// массив
        Value::Array arr;
        for (auto& el : n->elements)
            arr.push_back(evalExpr(*el));
        return Value(std::move(arr));
    }

    if (auto* n = dynamic_cast<const parser::StructLiteral*>(&expr)) {// структура
        Value::Struct s;
        for (auto& f : n->fields)
            s[f.name] = evalExpr(*f.value);
        return Value(std::move(s));
    }

    if (auto* n = dynamic_cast<const parser::ArrayAccess*>(&expr)) {// индексирование
        Value base  = evalExpr(*n->base);
        Value index = evalExpr(*n->index);
        if (!base.isArray())
            runtimeError("индексирование не-массива", n->pos.line);
        if (!index.isInt())
            runtimeError("индекс должен быть целым", n->pos.line);
        int64_t i = index.asInt();
        auto& arr = base.asArray();
        if (i < 0 || i >= static_cast<int64_t>(arr.size()))
            runtimeError("выход за границы массива", n->pos.line);
        return arr[i];
    }

    if (auto* n = dynamic_cast<const parser::FieldAccess*>(&expr)) {//// доступ к полю
        Value obj = evalExpr(*n->object);
        if (!obj.isStruct())
            runtimeError("доступ к полю не-структуры", n->pos.line);
        auto& s = obj.asStruct();
        auto it = s.find(n->field);
        if (it == s.end())
            runtimeError("поле '" + n->field + "' не найдено", n->pos.line);
        return it->second;
    }

    if (auto* n = dynamic_cast<const parser::Cast*>(&expr)) {//приведение типов
        Value v = evalExpr(*n->expr);
        const std::string& t = n->type;
        // int → float
        if ((t=="float32"||t=="float64") && v.isInt())
            return Value(static_cast<double>(v.asInt()));
        // float → int
        if ((t=="int8"||t=="int16"||t=="int32"||t=="int64"||
             t=="uint8"||t=="uint16"||t=="uint32"||t=="uint64") && v.isFloat())
            return Value(static_cast<int64_t>(v.asFloat()));
        // char → int
        if ((t=="int8"||t=="int16"||t=="int32"||t=="int64") && v.isChar())
            return Value(static_cast<int64_t>(v.asChar()));
        // int → char
        if (t=="char" && v.isInt())
            return Value(static_cast<char>(v.asInt()));
        return v; // тот же тип
    }

    if (auto* n = dynamic_cast<const parser::UnaryOp*>(&expr)) {//унарные операторы
        Value v = evalExpr(*n->operand);
        if (n->op == parser::UnOp::Neg) {
            if (v.isInt())   return Value(-v.asInt());
            if (v.isFloat()) return Value(-v.asFloat());
            runtimeError("унарный минус: не число", n->pos.line);
        }
        if (n->op == parser::UnOp::Not) {
            if (v.isBool()) return Value(!v.asBool());
            runtimeError("not: не bool", n->pos.line);
        }
    }

    if (auto* n = dynamic_cast<const parser::BinaryOp*>(&expr)) {//бинарные
        Value l = evalExpr(*n->left);
        Value r = evalExpr(*n->right);
        using B = parser::BinOp;

        switch (n->op) {
            case B::Add:// арифметика
                if (l.isInt() && r.isInt()) return Value(l.asInt() + r.asInt());
                if (l.isFloat() && r.isFloat()) return Value(l.asFloat() + r.asFloat());
                if (l.isString()&& r.isString())return Value(l.asString()+ r.asString());
                runtimeError("'+': несовместимые типы", n->pos.line);
            case B::Sub:
                if (l.isInt() && r.isInt()) return Value(l.asInt() - r.asInt());
                if (l.isFloat() && r.isFloat()) return Value(l.asFloat() - r.asFloat());
                runtimeError("'-': несовместимые типы", n->pos.line);
            case B::Mul:
                if (l.isInt() && r.isInt()) return Value(l.asInt() * r.asInt());
                if (l.isFloat() && r.isFloat()) return Value(l.asFloat() * r.asFloat());
                runtimeError("'*': несовместимые типы", n->pos.line);
            case B::Div:
                if (l.isInt() && r.isInt()) {
                    if (r.asInt() == 0)
                        runtimeError("деление на ноль", n->pos.line);
                    return Value(l.asInt() / r.asInt());
                }
                if (l.isFloat() && r.isFloat()) return Value(l.asFloat() / r.asFloat());
                runtimeError("'/': несовместимые типы", n->pos.line);
            case B::Mod:
                if (l.isInt() && r.isInt()) {
                    if (r.asInt() == 0)
                        runtimeError("остаток от деления на ноль", n->pos.line);
                    return Value(l.asInt() % r.asInt());
                }
                runtimeError("'%': только целые", n->pos.line);

            case B::Eq: return Value(l == r);// сравнения
            case B::Ne: return Value(l != r);
            case B::Lt:
                if (l.isInt() && r.isInt()) return Value(l.asInt() < r.asInt());
                if (l.isFloat() && r.isFloat()) return Value(l.asFloat() < r.asFloat());
                if (l.isChar() && r.isChar())  return Value(l.asChar() < r.asChar());
                runtimeError("'<': несовместимые типы", n->pos.line);
            case B::Gt:
                if (l.isInt() && r.isInt()) return Value(l.asInt() > r.asInt());
                if (l.isFloat() && r.isFloat()) return Value(l.asFloat() > r.asFloat());
                if (l.isChar() && r.isChar()) return Value(l.asChar() > r.asChar());
                runtimeError("'>': несовместимые типы", n->pos.line);
            case B::Le:
                if (l.isInt() && r.isInt()) return Value(l.asInt() <= r.asInt());
                if (l.isFloat() && r.isFloat()) return Value(l.asFloat() <= r.asFloat());
                if (l.isChar() && r.isChar()) return Value(l.asChar() <= r.asChar());
                runtimeError("'<=': несовместимые типы", n->pos.line);
            case B::Ge:
                if (l.isInt() && r.isInt()) return Value(l.asInt() >= r.asInt());
                if (l.isFloat() && r.isFloat()) return Value(l.asFloat() >= r.asFloat());
                if (l.isChar() && r.isChar()) return Value(l.asChar() >= r.asChar());
                runtimeError("'>=': несовместимые типы", n->pos.line);

            case B::And:// логика
                if (l.isBool() && r.isBool()) return Value(l.asBool() && r.asBool());
                runtimeError("'and': не bool", n->pos.line);
            case B::Or:
                if (l.isBool() && r.isBool()) return Value(l.asBool() || r.asBool());
                runtimeError("'or': не bool", n->pos.line);
        }
    }

    if (auto* n = dynamic_cast<const parser::Call*>(&expr)) {// вызов функции
        auto* callee = dynamic_cast<const parser::Identifier*>(n->callee.get());
        if (!callee)
            runtimeError("вызов только по имени", n->pos.line);

        Value builtin = callBuiltin(callee->name, n->args, n->pos.line);// сначала проверяем встроенные
        if (!builtin.isVoid() || callee->name == "print" ||
            callee->name == "println" || callee->name == "exit" ||
            callee->name == "panic" || callee->name == "assert" ||
            callee->name == "input")
            return builtin;

        auto it = functions_.find(callee->name);// пользовательская функция
        if (it == functions_.end())
            runtimeError("функция '" + callee->name + "' не найдена", n->pos.line);

        std::vector<Value> args;
        for (auto& a : n->args)
            args.push_back(evalExpr(*a));
        return callFunc(*it->second, std::move(args), n->pos.line);
    }

    if (auto* n = dynamic_cast<const parser::EnumLiteral*>(&expr))// enum литерал
        return Value(n->variant_name);

    if (auto* n = dynamic_cast<const parser::TupleLiteral*>(&expr)) {// кортеж
        Value::Array arr;
        for (auto& el : n->elements)
            arr.push_back(evalExpr(*el));
        return Value(std::move(arr));
    }

    runtimeError("неизвестное выражение", 0);
}

}
