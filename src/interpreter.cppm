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

}
