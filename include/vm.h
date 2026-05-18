#ifndef VM_H
#define VM_H

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "opcode.h"

// ─── Runtime Error ────────────────────────────────────────────────────────────
struct RuntimeError : std::runtime_error {
    int         line;
    std::string header;
    std::string detail;
    std::string token;

    RuntimeError(const std::string& detail, int line, const std::string& token = "")
        : std::runtime_error("[Runtime Error] [Line " + std::to_string(line) + "]:\n" + detail),
          line(line),
          header("[Runtime Error] [Line " + std::to_string(line) + "]"),
          detail(detail),
          token(token) {}
};

// ─── VM Error ─────────────────────────────────────────────────────────────────
struct VMError : std::runtime_error {
    std::string header;
    std::string detail;

    explicit VMError(const std::string& detail)
        : std::runtime_error("[VM Error]:\n" + detail),
          header("[VM Error]"),
          detail(detail) {}
};

struct ExecutionResult {
    bool        hasValue = false;
    bool        printedValue = false;
    std::string value;
    ValueType   type = ValueType::Unknown;
};

class VM {
public:
    ExecutionResult execute(const std::vector<Instruction>& instructions);
    ExecutionResult execute(const std::vector<Instruction>& instructions,
                            const std::string& source,
                            std::istream& input,
                            std::ostream& output);

private:
    // A stack value carries both representations so mixed-type ops need no extra conversion.
    struct TypedValue {
        long long ival = 0;
        double    fval = 0.0;
        ValueType type = ValueType::Unknown;

        TypedValue() = default;
        TypedValue(long long v, ValueType t)
            : ival(v), fval(static_cast<double>(v)), type(t) {}
        TypedValue(double v, ValueType t)
            : ival(static_cast<long long>(v)), fval(v), type(t) {}
    };

    struct VariableValue {
        long long ival = 0;
        double    fval = 0.0;
        ValueType type = ValueType::Unknown;
    };

    void       push(long long value, ValueType type);
    void       pushFloat(double value);
    TypedValue pop();

    std::vector<TypedValue>                         stack;
    // Variable storage as a scope stack; innermost scope is at the back.
    // Each Declare opcode pushes into the current (back) scope.
    // LoadVar / StoreVar search from back to front.
    using VarMap = std::unordered_map<std::string, VariableValue>;
    std::vector<VarMap> scopes;
};

#endif
