#ifndef VM_H
#define VM_H

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "opcode.h"

// ─── Runtime Error ────────────────────────────────────────────────────────────
// Thrown by VM::execute() for arithmetic errors, division by zero, overflow.
// Format:  [Runtime Error] [Line N]:\n<message>
struct RuntimeError : std::runtime_error {
    int         line;
    std::string header;
    std::string detail;
    std::string token;  // specific lexeme to underline; "" = whole line

    RuntimeError(const std::string& detail, int line, const std::string& token = "")
        : std::runtime_error("[Runtime Error] [Line " + std::to_string(line) + "]:\n" + detail),
          line(line),
          header("[Runtime Error] [Line " + std::to_string(line) + "]"),
          detail(detail),
          token(token) {}
};

// ─── Type Mismatch Error ──────────────────────────────────────────────────────
// Thrown for input that is not an integer.
// Format:  [Type Mismatch Error]:\n<message>
struct TypeMismatchError : std::runtime_error {
    int         line;
    std::string header;
    std::string detail;

    TypeMismatchError(const std::string& detail, int line)
        : std::runtime_error("[Type Mismatch Error] [Line " + std::to_string(line) + "]:\n" + detail),
          line(line),
          header("[Type Mismatch Error] [Line " + std::to_string(line) + "]"),
          detail(detail) {}
};

// ─── VM Error ─────────────────────────────────────────────────────────────────
// Thrown for internal VM faults: stack underflow, unknown opcode.
// Format:  [VM Error]:\n<message>
struct VMError : std::runtime_error {
    std::string header;
    std::string detail;

    explicit VMError(const std::string& detail)
        : std::runtime_error("[VM Error]:\n" + detail),
          header("[VM Error]"),
          detail(detail) {}
};

class VM {
public:
    void execute(const std::vector<Instruction>& instructions);
    void execute(const std::vector<Instruction>& instructions,
                 const std::string& source,
                 std::istream& input,
                 std::ostream& output);

private:
    void       push(long long value);
    long long  pop();

    std::vector<long long>                   stack;
    std::unordered_map<std::string, long long> variables;
};

#endif