#include "vm.h"

#include <climits>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <stdexcept>

// ─── Internal arithmetic helpers ─────────────────────────────────────────────

namespace {

// long long range: -(2^63) to 2^63-1
// We use __int128 for overflow detection where available, otherwise check manually.

long long parseLongValue(const std::string& text, bool isInput) {
    try {
        std::size_t parsedChars = 0;
        long long value = std::stoll(text, &parsedChars);
        if (parsedChars != text.size()) {
            throw std::runtime_error(isInput ? "Expected integer input"
                                             : "Invalid integer literal: " + text);
        }
        return value;
    } catch (const std::invalid_argument&) {
        throw std::runtime_error(isInput ? "Expected integer input, received non-integer"
                                         : "Invalid integer literal: " + text);
    } catch (const std::out_of_range&) {
        throw std::runtime_error(
            isInput ? "Integer input is outside 64-bit range: " + text
                    : "Integer literal is outside 64-bit range: " + text);
    }
}

// Use __int128 to detect overflow safely
long long checkedAdd(long long a, long long b) {
    __int128 r = (__int128)a + (__int128)b;
    if (r > std::numeric_limits<long long>::max() ||
        r < std::numeric_limits<long long>::min())
        throw std::runtime_error("Integer overflow during addition");
    return (long long)r;
}

long long checkedSubtract(long long a, long long b) {
    __int128 r = (__int128)a - (__int128)b;
    if (r > std::numeric_limits<long long>::max() ||
        r < std::numeric_limits<long long>::min())
        throw std::runtime_error("Integer overflow during subtraction");
    return (long long)r;
}

long long checkedMultiply(long long a, long long b) {
    __int128 r = (__int128)a * (__int128)b;
    if (r > std::numeric_limits<long long>::max() ||
        r < std::numeric_limits<long long>::min())
        throw std::runtime_error("Integer overflow during multiplication");
    return (long long)r;
}

long long checkedDivide(long long a, long long b) {
    if (b == 0) throw std::runtime_error("Division by zero");
    // Only overflow case: LLONG_MIN / -1
    if (a == std::numeric_limits<long long>::min() && b == -1)
        throw std::runtime_error("Integer overflow during division");
    return a / b;
}

long long checkedModulo(long long a, long long b) {
    if (b == 0) throw std::runtime_error("Modulo by zero");
    return a % b;
}

}  // namespace

// ─── VM ──────────────────────────────────────────────────────────────────────

void VM::execute(const std::vector<Instruction>& instructions) {
    execute(instructions, "", std::cin, std::cout);
}

void VM::execute(const std::vector<Instruction>& instructions,
                 const std::string& /*source*/,
                 std::istream& input,
                 std::ostream& output) {
    stack.clear();
    variables.clear();

    std::size_t ip = 0;

    // Helper: build a RuntimeError with line info
    auto runtimeErr = [&](const std::string& msg, const std::string& tok = "") -> RuntimeError {
        int ln = instructions[ip].line;
        return RuntimeError(msg, ln, tok);
    };

    while (ip < instructions.size()) {
        const Instruction& instr = instructions[ip];

        switch (instr.opcode) {

            // ── Push a literal ────────────────────────────────────────────────
            case OpCode::PushInt: {
                try {
                    push(parseLongValue(instr.operand, false));
                } catch (const std::exception& e) {
                    int ln = instr.line;
                    throw RuntimeError(e.what(), ln, instr.operand);
                }
                break;
            }

            // ── Read from stdin ───────────────────────────────────────────────
            case OpCode::Input: {
                std::string line;
                output << "input> ";
                if (!std::getline(input, line)) {
                    throw runtimeErr("Failed to read input");
                }
                // Trim trailing whitespace/CR
                while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
                    line.pop_back();

                try {
                    push(parseLongValue(line, true));
                } catch (const std::exception& e) {
                    int ln = instr.line;
                    // Check if the input is non-integer (letters etc.)
                    bool hasAlpha = false;
                    for (char c : line)
                        if (std::isalpha((unsigned char)c)) { hasAlpha = true; break; }
                    if (hasAlpha) {
                        throw TypeMismatchError(
                            std::string("Expected integer input, received string '") + line + "'",
                            ln);
                    }
                    throw runtimeErr(e.what());
                }
                break;
            }

            // ── Variable load ─────────────────────────────────────────────────
            case OpCode::LoadVar: {
                auto it = variables.find(instr.operand);
                if (it == variables.end()) {
                    // Should have been caught by semantic analysis; keep as safety net
                    throw runtimeErr("Undefined variable '" + instr.operand + "'", instr.operand);
                }
                push(it->second);
                break;
            }

            // ── Variable store ────────────────────────────────────────────────
            case OpCode::StoreVar: {
                long long value = pop();
                variables[instr.operand] = value;
                break;
            }

            // ── Arithmetic ────────────────────────────────────────────────────
            case OpCode::Add: {
                long long right = pop(), left = pop();
                try { push(checkedAdd(left, right)); }
                catch (const std::exception& e) {
                    std::string tok = std::to_string(left) + "+" + std::to_string(right);
                    throw runtimeErr(e.what(), tok);
                }
                break;
            }

            case OpCode::Subtract: {
                long long right = pop(), left = pop();
                try { push(checkedSubtract(left, right)); }
                catch (const std::exception& e) {
                    std::string tok = std::to_string(left) + "-" + std::to_string(right);
                    throw runtimeErr(e.what(), tok);
                }
                break;
            }

            case OpCode::Multiply: {
                long long right = pop(), left = pop();
                try { push(checkedMultiply(left, right)); }
                catch (const std::exception& e) {
                    std::string tok = std::to_string(left) + "*" + std::to_string(right);
                    throw runtimeErr(e.what(), tok);
                }
                break;
            }

            case OpCode::Divide: {
                long long right = pop(), left = pop();
                try { push(checkedDivide(left, right)); }
                catch (const std::exception& e) {
                    std::string tok = std::to_string(left) + "/" + std::to_string(right);
                    throw runtimeErr(e.what(), tok);
                }
                break;
            }

            case OpCode::Modulo: {
                long long right = pop(), left = pop();
                try { push(checkedModulo(left, right)); }
                catch (const std::exception& e) {
                    std::string tok = std::to_string(left) + "%" + std::to_string(right);
                    throw runtimeErr(e.what(), tok);
                }
                break;
            }

            // ── Comparison ────────────────────────────────────────────────────
            case OpCode::Equal: {
                long long right = pop(), left = pop();
                push(left == right ? 1 : 0);
                break;
            }

            case OpCode::NotEqual: {
                long long right = pop(), left = pop();
                push(left != right ? 1 : 0);
                break;
            }

            case OpCode::Less: {
                long long right = pop(), left = pop();
                push(left < right ? 1 : 0);
                break;
            }

            case OpCode::LessEqual: {
                long long right = pop(), left = pop();
                push(left <= right ? 1 : 0);
                break;
            }

            case OpCode::Greater: {
                long long right = pop(), left = pop();
                push(left > right ? 1 : 0);
                break;
            }

            case OpCode::GreaterEqual: {
                long long right = pop(), left = pop();
                push(left >= right ? 1 : 0);
                break;
            }

            // ── Logical ───────────────────────────────────────────────────────
            case OpCode::And: {
                long long right = pop(), left = pop();
                push((left != 0 && right != 0) ? 1 : 0);
                break;
            }

            case OpCode::Or: {
                long long right = pop(), left = pop();
                push((left != 0 || right != 0) ? 1 : 0);
                break;
            }

            case OpCode::Not: {
                long long val = pop();
                push(val == 0 ? 1 : 0);
                break;
            }

            // ── Control flow ──────────────────────────────────────────────────
            case OpCode::Jump:
                ip = static_cast<std::size_t>(std::stoull(instr.operand));
                continue;

            case OpCode::JumpIfFalse: {
                long long condition = pop();
                if (condition == 0) {
                    ip = static_cast<std::size_t>(std::stoull(instr.operand));
                    continue;
                }
                break;
            }

            // ── I/O ───────────────────────────────────────────────────────────
            case OpCode::Print: {
                long long value = pop();
                output << value << '\n';
                break;
            }

            case OpCode::Pop:
                pop();
                break;

            case OpCode::Halt:
                return;

            // ── Unknown opcode ────────────────────────────────────────────────
            default: {
                throw VMError("Unknown opcode encountered: " +
                              std::to_string(static_cast<int>(instr.opcode)));
            }
        }

        ip++;
    }
}

void VM::push(long long value) {
    stack.push_back(value);
}

long long VM::pop() {
    if (stack.empty())
        throw VMError("Stack underflow during operation");
    long long value = stack.back();
    stack.pop_back();
    return value;
}