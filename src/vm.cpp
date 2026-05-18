#include "vm.h"

#include <cmath>
#include <climits>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace {

// ── Range helpers ─────────────────────────────────────────────────────────────

long long intMin()  { return static_cast<long long>(std::numeric_limits<int>::min()); }
long long intMax()  { return static_cast<long long>(std::numeric_limits<int>::max()); }
long long llMin()   { return std::numeric_limits<long long>::min(); }
long long llMax()   { return std::numeric_limits<long long>::max(); }

bool fitsInType(long long v, ValueType t) {
    if (t == ValueType::Bool)     return v == 0 || v == 1;
    if (t == ValueType::Int)      return v >= intMin() && v <= intMax();
    if (t == ValueType::LongLong) return true;   // long long is native storage
    return true;
}

long long normalizeBool(long long v) { return v != 0 ? 1 : 0; }

// Float-op: if either side is Float, the whole expression is float
bool isFloatOp(ValueType a, ValueType b) {
    return a == ValueType::Float || b == ValueType::Float;
}

// Promotion: Float > LongLong > Int > Bool
ValueType numericResultType(ValueType l, ValueType r) {
    if (l == ValueType::Float    || r == ValueType::Float)    return ValueType::Float;
    if (l == ValueType::LongLong || r == ValueType::LongLong) return ValueType::LongLong;
    return ValueType::Int;
}

// ── Range-checked helpers ─────────────────────────────────────────────────────

std::string rangeMsg(long long v, ValueType t) {
    return "Value " + std::to_string(v) + " is outside " +
           std::string(valueTypeToString(t)) + " range";
}

long long ensureRange(long long v, ValueType t, const std::string& msg) {
    if (!fitsInType(v, t)) throw std::runtime_error(msg);
    return v;
}

long long ensureRange128(__int128 v, ValueType t, const std::string& msg) {
    if (t == ValueType::Int) {
        if (v < static_cast<__int128>(intMin()) || v > static_cast<__int128>(intMax()))
            throw std::runtime_error(msg);
    } else if (t == ValueType::LongLong) {
        if (v < static_cast<__int128>(llMin()) || v > static_cast<__int128>(llMax()))
            throw std::runtime_error(msg);
    }
    return static_cast<long long>(v);
}

// ── Integer checked arithmetic ────────────────────────────────────────────────

long long chkAdd(long long l, long long r, ValueType t) {
    return ensureRange128(static_cast<__int128>(l) + r, t, "Integer overflow during addition");
}
long long chkSub(long long l, long long r, ValueType t) {
    return ensureRange128(static_cast<__int128>(l) - r, t, "Integer overflow during subtraction");
}
long long chkMul(long long l, long long r, ValueType t) {
    return ensureRange128(static_cast<__int128>(l) * r, t, "Integer overflow during multiplication");
}
long long chkDiv(long long l, long long r, ValueType t) {
    if (r == 0) throw std::runtime_error("Division by zero");
    return ensureRange128(static_cast<__int128>(l) / r, t, "Integer overflow during division");
}
long long chkMod(long long l, long long r, ValueType) {
    if (r == 0) throw std::runtime_error("Modulo by zero");
    return l % r;
}
long long chkPow(long long base, long long exp, ValueType t) {
    if (exp < 0)  throw std::runtime_error("Negative exponent not supported for integers");
    if (exp > 62) throw std::runtime_error("Exponent too large (max 62 for integers)");
    // Always use LongLong for ALL intermediate and final arithmetic.  The
    // operand types (e.g. Int) must not constrain intermediate products —
    // only the *declared* variable type matters, and that check happens in
    // storeDeclared after the Power opcode returns.  Checking against `t`
    // here would incorrectly reject e.g. `long long int x = 2^^34` because
    // both literals are typed Int, making t==Int, yet the result is valid LL.
    long long result = 1, b = base, e = exp;
    while (e > 0) {
        if (e & 1) result = chkMul(result, b, ValueType::LongLong);
        e >>= 1;
        if (e > 0) b = chkMul(b, b, ValueType::LongLong);
    }
    return ensureRange(result, t, "Integer overflow during power");
}

// ── Number parsing ────────────────────────────────────────────────────────────

struct ParsedNumber {
    long long ival;
    double    fval;
    ValueType type;
};

ParsedNumber parseIntText(const std::string& text, bool isInput) {
    try {
        std::size_t n = 0;
        long long v = std::stoll(text, &n);
        if (n != text.size())
            throw std::runtime_error(isInput ? "Expected integer input"
                                             : "Invalid integer literal: " + text);
        ValueType t = fitsInType(v, ValueType::Int) ? ValueType::Int : ValueType::LongLong;
        return {v, static_cast<double>(v), t};
    } catch (const std::invalid_argument&) {
        throw std::runtime_error(isInput ? "Expected integer input"
                                         : "Invalid integer literal: " + text);
    } catch (const std::out_of_range&) {
        throw std::runtime_error(isInput ? "Input outside long long range: " + text
                                         : "Integer literal outside long long range: " + text);
    }
}

ParsedNumber parseFloatText(const std::string& text) {
    try {
        std::size_t n = 0;
        double v = std::stod(text, &n);
        if (n != text.size())
            throw std::runtime_error("Invalid float literal: " + text);
        return {static_cast<long long>(v), v, ValueType::Float};
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("Invalid float literal: " + text);
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Float literal out of range: " + text);
    }
}

ParsedNumber parseInputText(const std::string& text) {
    if (text.find('.') != std::string::npos ||
        text.find('e') != std::string::npos ||
        text.find('E') != std::string::npos) {
        try { return parseFloatText(text); } catch (...) {}
    }
    return parseIntText(text, true);
}

// ── Float printing  (match C++ default stream behaviour) ─────────────────────
// Prints up to 6 significant digits, strips trailing zeros, keeps the decimal
// point only when needed (mirrors %g format).

std::string formatFloat(double v) {
    std::ostringstream oss;
    oss << v;          // default: 6 sig-digits, strips zeros like %g
    return oss.str();
}

} // namespace

// ─── VM ───────────────────────────────────────────────────────────────────────

void VM::execute(const std::vector<Instruction>& instructions) {
    execute(instructions, "", std::cin, std::cout);
}

void VM::execute(const std::vector<Instruction>& instructions,
                 const std::string& /*source*/,
                 std::istream& input,
                 std::ostream& output) {

    stack.clear();
    scopes.clear();
    scopes.push_back(VarMap{});  // global scope

    std::size_t ip = 0;

    // ── Helper lambdas ────────────────────────────────────────────────────────

    auto runtimeErr = [&](const std::string& msg,
                          const std::string& tok = "") -> RuntimeError {
        return RuntimeError(msg, instructions[ip].line, tok);
    };

    // Search scope stack from inner to outer, return pointer or nullptr.
    auto findVar = [&](const std::string& name) -> VariableValue* {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return &found->second;
        }
        return nullptr;
    };

    // Store a freshly-declared variable in the CURRENT (innermost) scope.
    auto storeDeclared = [&](const std::string& name, ValueType targetType) {
        TypedValue v = pop();
        VariableValue var;
        var.type = targetType;
        if (targetType == ValueType::Float) {
            var.fval = v.fval;
            var.ival = static_cast<long long>(v.fval);
        } else if (targetType == ValueType::Bool) {
            long long raw = (v.type == ValueType::Float) ? (v.fval != 0.0 ? 1LL : 0LL) : v.ival;
            var.ival = normalizeBool(raw);
            var.fval = static_cast<double>(var.ival);
        } else {
            // Int or LongLong
            long long raw = (v.type == ValueType::Float)
                            ? static_cast<long long>(v.fval) : v.ival;
            var.ival = ensureRange(raw, targetType, rangeMsg(raw, targetType));
            var.fval = static_cast<double>(var.ival);
        }
        scopes.back()[name] = var;
    };

    // Assign to an existing variable (search all scopes, update in place).
    auto assignVar = [&](const std::string& name) {
        TypedValue v = pop();
        VariableValue* var = findVar(name);
        if (!var)
            throw runtimeErr("Undefined variable '" + name + "'", name);
        ValueType targetType = var->type;
        if (targetType == ValueType::Float) {
            var->fval = v.fval;
            var->ival = static_cast<long long>(v.fval);
        } else if (targetType == ValueType::Bool) {
            long long raw = (v.type == ValueType::Float) ? (v.fval != 0.0 ? 1LL : 0LL) : v.ival;
            var->ival = normalizeBool(raw);
            var->fval = static_cast<double>(var->ival);
        } else {
            long long raw = (v.type == ValueType::Float)
                            ? static_cast<long long>(v.fval) : v.ival;
            var->ival = ensureRange(raw, targetType, rangeMsg(raw, targetType));
            var->fval = static_cast<double>(var->ival);
        }
    };

    auto arithmeticTok = [](long long l, const std::string& op, long long r) {
        return std::to_string(l) + op + std::to_string(r);
    };

    // ── Dispatch loop ─────────────────────────────────────────────────────────

    while (ip < instructions.size()) {
        const Instruction& instr = instructions[ip];

        switch (instr.opcode) {

            // ── Push literals ─────────────────────────────────────────────
            case OpCode::PushBool:
                push(instr.operand == "1" ? 1LL : 0LL, ValueType::Bool);
                break;

            case OpCode::PushInt: {
                try {
                    auto p = parseIntText(instr.operand, false);
                    push(p.ival, p.type);
                } catch (const std::exception& e) { throw runtimeErr(e.what(), instr.operand); }
                break;
            }

            case OpCode::PushLongLong: {
                try {
                    auto p = parseIntText(instr.operand, false);
                    push(p.ival, ValueType::LongLong);
                } catch (const std::exception& e) { throw runtimeErr(e.what(), instr.operand); }
                break;
            }

            case OpCode::PushFloat: {
                try {
                    auto p = parseFloatText(instr.operand);
                    pushFloat(p.fval);
                } catch (const std::exception& e) { throw runtimeErr(e.what(), instr.operand); }
                break;
            }

            // ── Input ─────────────────────────────────────────────────────
            case OpCode::Input: {
                std::string line;
                output << "input> ";
                if (!std::getline(input, line))
                    throw runtimeErr("Failed to read input");
                // Strip trailing whitespace / CR
                while (!line.empty() &&
                       (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
                    line.pop_back();
                try {
                    auto p = parseInputText(line);
                    if (p.type == ValueType::Float) pushFloat(p.fval);
                    else push(p.ival, p.type);
                } catch (const std::exception& e) { throw runtimeErr(e.what(), "input"); }
                break;
            }

            // ── Variables ─────────────────────────────────────────────────
            case OpCode::LoadVar: {
                VariableValue* var = findVar(instr.operand);
                if (!var)
                    throw runtimeErr("Undefined variable '" + instr.operand + "'", instr.operand);
                if (var->type == ValueType::Float) pushFloat(var->fval);
                else push(var->ival, var->type);
                break;
            }

            case OpCode::DeclareBool:
                try { storeDeclared(instr.operand, ValueType::Bool); }
                catch (const std::exception& e) { throw runtimeErr(e.what(), instr.operand); }
                break;

            case OpCode::DeclareInt:
                try { storeDeclared(instr.operand, ValueType::Int); }
                catch (const std::exception& e) { throw runtimeErr(e.what(), instr.operand); }
                break;

            case OpCode::DeclareLongLong:
                try { storeDeclared(instr.operand, ValueType::LongLong); }
                catch (const std::exception& e) { throw runtimeErr(e.what(), instr.operand); }
                break;

            case OpCode::DeclareFloat:
                try { storeDeclared(instr.operand, ValueType::Float); }
                catch (const std::exception& e) { throw runtimeErr(e.what(), instr.operand); }
                break;

            case OpCode::StoreVar:
                try { assignVar(instr.operand); }
                catch (const RuntimeError&) { throw; }
                catch (const std::exception& e) { throw runtimeErr(e.what(), instr.operand); }
                break;

            // ── Explicit casts ────────────────────────────────────────────
            case OpCode::CastToInt: {
                TypedValue v = pop();
                long long r = (v.type == ValueType::Float)
                              ? static_cast<long long>(v.fval) : v.ival;
                if (!fitsInType(r, ValueType::Int))
                    throw runtimeErr(rangeMsg(r, ValueType::Int));
                push(r, ValueType::Int);
                break;
            }

            case OpCode::CastToLongLong: {
                TypedValue v = pop();
                long long r = (v.type == ValueType::Float)
                              ? static_cast<long long>(v.fval) : v.ival;
                push(r, ValueType::LongLong);
                break;
            }

            case OpCode::CastToFloat: {
                TypedValue v = pop();
                pushFloat(v.fval);
                break;
            }

            case OpCode::CastToBool: {
                TypedValue v = pop();
                long long r = (v.type == ValueType::Float)
                              ? (v.fval != 0.0 ? 1LL : 0LL)
                              : normalizeBool(v.ival);
                push(r, ValueType::Bool);
                break;
            }

            // ── Arithmetic ────────────────────────────────────────────────
            case OpCode::Add: {
                TypedValue r = pop(), l = pop();
                ValueType rt = numericResultType(l.type, r.type);
                if (isFloatOp(l.type, r.type)) { pushFloat(l.fval + r.fval); break; }
                try { push(chkAdd(l.ival, r.ival, rt), rt); }
                catch (const std::exception& e) {
                    throw runtimeErr(e.what(), arithmeticTok(l.ival, "+", r.ival));
                }
                break;
            }

            case OpCode::Subtract: {
                TypedValue r = pop(), l = pop();
                ValueType rt = numericResultType(l.type, r.type);
                if (isFloatOp(l.type, r.type)) { pushFloat(l.fval - r.fval); break; }
                try { push(chkSub(l.ival, r.ival, rt), rt); }
                catch (const std::exception& e) {
                    throw runtimeErr(e.what(), arithmeticTok(l.ival, "-", r.ival));
                }
                break;
            }

            case OpCode::Multiply: {
                TypedValue r = pop(), l = pop();
                ValueType rt = numericResultType(l.type, r.type);
                if (isFloatOp(l.type, r.type)) { pushFloat(l.fval * r.fval); break; }
                try { push(chkMul(l.ival, r.ival, rt), rt); }
                catch (const std::exception& e) {
                    throw runtimeErr(e.what(), arithmeticTok(l.ival, "*", r.ival));
                }
                break;
            }

            case OpCode::Divide: {
                TypedValue r = pop(), l = pop();
                ValueType rt = numericResultType(l.type, r.type);
                if (isFloatOp(l.type, r.type)) {
                    if (r.fval == 0.0) throw runtimeErr("Division by zero");
                    pushFloat(l.fval / r.fval); break;
                }
                try { push(chkDiv(l.ival, r.ival, rt), rt); }
                catch (const std::exception& e) {
                    throw runtimeErr(e.what(), arithmeticTok(l.ival, "/", r.ival));
                }
                break;
            }

            case OpCode::Modulo: {
                TypedValue r = pop(), l = pop();
                if (l.type == ValueType::Float || r.type == ValueType::Float)
                    throw runtimeErr("Modulo '%' is not supported on float values");
                ValueType rt = numericResultType(l.type, r.type);
                try { push(chkMod(l.ival, r.ival, rt), rt); }
                catch (const std::exception& e) {
                    throw runtimeErr(e.what(), arithmeticTok(l.ival, "%", r.ival));
                }
                break;
            }

            case OpCode::Power: {
                TypedValue r = pop(), l = pop();
                ValueType rt = numericResultType(l.type, r.type);
                if (isFloatOp(l.type, r.type)) { pushFloat(std::pow(l.fval, r.fval)); break; }
                // Always compute integer power as LongLong so bare Int literals
                // (e.g. 2^^34) don't overflow before reaching storeDeclared,
                // which applies the correct declared-type range check.
                try { push(chkPow(l.ival, r.ival, ValueType::LongLong), ValueType::LongLong); }
                catch (const std::exception& e) {
                    throw runtimeErr(e.what(), arithmeticTok(l.ival, "^^", r.ival));
                }
                break;
            }

            // ── Comparison ────────────────────────────────────────────────
            case OpCode::Equal: {
                TypedValue r = pop(), l = pop();
                bool res = isFloatOp(l.type, r.type) ? l.fval == r.fval : l.ival == r.ival;
                push(res ? 1 : 0, ValueType::Bool); break;
            }
            case OpCode::NotEqual: {
                TypedValue r = pop(), l = pop();
                bool res = isFloatOp(l.type, r.type) ? l.fval != r.fval : l.ival != r.ival;
                push(res ? 1 : 0, ValueType::Bool); break;
            }
            case OpCode::Less: {
                TypedValue r = pop(), l = pop();
                bool res = isFloatOp(l.type, r.type) ? l.fval < r.fval : l.ival < r.ival;
                push(res ? 1 : 0, ValueType::Bool); break;
            }
            case OpCode::LessEqual: {
                TypedValue r = pop(), l = pop();
                bool res = isFloatOp(l.type, r.type) ? l.fval <= r.fval : l.ival <= r.ival;
                push(res ? 1 : 0, ValueType::Bool); break;
            }
            case OpCode::Greater: {
                TypedValue r = pop(), l = pop();
                bool res = isFloatOp(l.type, r.type) ? l.fval > r.fval : l.ival > r.ival;
                push(res ? 1 : 0, ValueType::Bool); break;
            }
            case OpCode::GreaterEqual: {
                TypedValue r = pop(), l = pop();
                bool res = isFloatOp(l.type, r.type) ? l.fval >= r.fval : l.ival >= r.ival;
                push(res ? 1 : 0, ValueType::Bool); break;
            }

            // ── Logical ───────────────────────────────────────────────────
            // Note: and/or are short-circuit compiled to JumpIfFalse chains.
            // These opcodes exist only for completeness; not emitted in practice.
            case OpCode::And: {
                TypedValue r = pop(), l = pop();
                push((l.ival != 0 && r.ival != 0) ? 1 : 0, ValueType::Bool); break;
            }
            case OpCode::Or: {
                TypedValue r = pop(), l = pop();
                push((l.ival != 0 || r.ival != 0) ? 1 : 0, ValueType::Bool); break;
            }
            case OpCode::Not: {
                TypedValue v = pop();
                bool zero = (v.type == ValueType::Float) ? v.fval == 0.0 : v.ival == 0;
                push(zero ? 1 : 0, ValueType::Bool); break;
            }

            // ── Bitwise (integers only) ───────────────────────────────────
            case OpCode::BitAnd: {
                TypedValue r = pop(), l = pop();
                if (l.type == ValueType::Bool || r.type == ValueType::Bool ||
                    l.type == ValueType::Float || r.type == ValueType::Float)
                    throw runtimeErr("Bitwise '&' requires integer operands");
                push(l.ival & r.ival, numericResultType(l.type, r.type)); break;
            }
            case OpCode::BitOr: {
                TypedValue r = pop(), l = pop();
                if (l.type == ValueType::Bool || r.type == ValueType::Bool ||
                    l.type == ValueType::Float || r.type == ValueType::Float)
                    throw runtimeErr("Bitwise '|' requires integer operands");
                push(l.ival | r.ival, numericResultType(l.type, r.type)); break;
            }
            case OpCode::BitXor: {
                TypedValue r = pop(), l = pop();
                if (l.type == ValueType::Bool || r.type == ValueType::Bool ||
                    l.type == ValueType::Float || r.type == ValueType::Float)
                    throw runtimeErr("Bitwise '^' requires integer operands");
                push(l.ival ^ r.ival, numericResultType(l.type, r.type)); break;
            }
            case OpCode::BitNot: {
                TypedValue v = pop();
                if (v.type == ValueType::Bool || v.type == ValueType::Float)
                    throw runtimeErr("Bitwise '~' requires an integer operand");
                push(~v.ival, v.type); break;
            }
            case OpCode::ShiftLeft: {
                TypedValue r = pop(), l = pop();
                if (l.type == ValueType::Bool || l.type == ValueType::Float ||
                    r.type == ValueType::Float)
                    throw runtimeErr("Shift '<<' requires integer operands");
                if (r.ival < 0)   throw runtimeErr("Shift amount cannot be negative: " + std::to_string(r.ival));
                if (r.ival >= 64) throw runtimeErr("Shift amount too large: " + std::to_string(r.ival));
                push(l.ival << r.ival, numericResultType(l.type, r.type)); break;
            }
            case OpCode::ShiftRight: {
                TypedValue r = pop(), l = pop();
                if (l.type == ValueType::Bool || l.type == ValueType::Float ||
                    r.type == ValueType::Float)
                    throw runtimeErr("Shift '>>' requires integer operands");
                if (r.ival < 0)   throw runtimeErr("Shift amount cannot be negative: " + std::to_string(r.ival));
                if (r.ival >= 64) throw runtimeErr("Shift amount too large: " + std::to_string(r.ival));
                push(l.ival >> r.ival, numericResultType(l.type, r.type)); break;
            }

            // ── Control flow ──────────────────────────────────────────────
            case OpCode::Jump:
                ip = static_cast<std::size_t>(std::stoull(instr.operand));
                continue;

            case OpCode::JumpIfFalse: {
                TypedValue cond = pop();
                bool isFalse = (cond.type == ValueType::Float)
                               ? cond.fval == 0.0 : cond.ival == 0;
                if (isFalse) {
                    ip = static_cast<std::size_t>(std::stoull(instr.operand));
                    continue;
                }
                break;
            }

            // ── I/O ───────────────────────────────────────────────────────
            case OpCode::Print: {
                TypedValue v = pop();
                if (v.type == ValueType::Float) {
                    output << formatFloat(v.fval) << '\n';
                } else {
                    output << v.ival << '\n';
                }
                break;
            }

            // ── Scope management ─────────────────────────────────────────────
            case OpCode::PushScope:
                scopes.push_back(VarMap{});
                break;

            case OpCode::PopScope:
                if (scopes.size() > 1) scopes.pop_back();
                break;

            case OpCode::Pop:  pop(); break;
            case OpCode::Halt: return;

            default:
                throw VMError("Unknown opcode: " +
                              std::to_string(static_cast<int>(instr.opcode)));
        }

        ip++;
    }
}

void VM::push(long long value, ValueType type) {
    stack.emplace_back(value, type);
}

void VM::pushFloat(double value) {
    stack.emplace_back(value, ValueType::Float);
}

VM::TypedValue VM::pop() {
    if (stack.empty()) throw VMError("Stack underflow");
    TypedValue v = stack.back();
    stack.pop_back();
    return v;
}