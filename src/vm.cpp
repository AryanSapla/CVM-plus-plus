#include "vm.h"

#include <climits>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace {

long long intMinValue() {
    return static_cast<long long>(std::numeric_limits<int>::min());
}

long long intMaxValue() {
    return static_cast<long long>(std::numeric_limits<int>::max());
}

long long longMinValue() {
    return static_cast<long long>(std::numeric_limits<long>::min());
}

long long longMaxValue() {
    return static_cast<long long>(std::numeric_limits<long>::max());
}

bool fitsInType(long long value, ValueType type) {
    if (type == ValueType::Bool) {
        return value == 0 || value == 1;
    }

    if (type == ValueType::Int) {
        return value >= intMinValue() && value <= intMaxValue();
    }

    if (type == ValueType::Long) {
        return value >= longMinValue() && value <= longMaxValue();
    }

    return true;
}

long long normalizeBool(long long value) {
    return value == 0 ? 0 : 1;
}

std::pair<long long, ValueType> parseNumericText(const std::string& text, bool isInput) {
    try {
        std::size_t parsedChars = 0;
        long long value = std::stoll(text, &parsedChars);

        if (parsedChars != text.size()) {
            throw std::runtime_error(isInput ? "Expected integer input"
                                             : "Invalid integer literal: " + text);
        }

        if (fitsInType(value, ValueType::Int)) {
            return {value, ValueType::Int};
        }

        if (fitsInType(value, ValueType::Long)) {
            return {value, ValueType::Long};
        }

        throw std::runtime_error(isInput ? "Integer input is outside long range: " + text
                                         : "Integer literal is outside long range: " + text);
    } catch (const std::invalid_argument&) {
        throw std::runtime_error(isInput ? "Expected integer input"
                                         : "Invalid integer literal: " + text);
    } catch (const std::out_of_range&) {
        throw std::runtime_error(isInput ? "Integer input is outside long range: " + text
                                         : "Integer literal is outside long range: " + text);
    }
}

long long ensureRange(long long value, ValueType targetType, const std::string& message) {
    if (!fitsInType(value, targetType)) {
        throw std::runtime_error(message);
    }

    return value;
}

long long ensureRange128(__int128 value, ValueType targetType, const std::string& message) {
    if (targetType == ValueType::Int) {
        if (value < static_cast<__int128>(intMinValue()) ||
            value > static_cast<__int128>(intMaxValue())) {
            throw std::runtime_error(message);
        }
        return static_cast<long long>(value);
    }

    if (targetType == ValueType::Long) {
        if (value < static_cast<__int128>(longMinValue()) ||
            value > static_cast<__int128>(longMaxValue())) {
            throw std::runtime_error(message);
        }
        return static_cast<long long>(value);
    }

    return static_cast<long long>(value);
}

ValueType numericResultType(ValueType left, ValueType right) {
    if (left == ValueType::Long || right == ValueType::Long) {
        return ValueType::Long;
    }

    return ValueType::Int;
}

long long checkedAdd(long long left, long long right, ValueType resultType) {
    __int128 result = static_cast<__int128>(left) + static_cast<__int128>(right);
    return ensureRange128(result, resultType, "Integer overflow during addition");
}

long long checkedSubtract(long long left, long long right, ValueType resultType) {
    __int128 result = static_cast<__int128>(left) - static_cast<__int128>(right);
    return ensureRange128(result, resultType, "Integer overflow during subtraction");
}

long long checkedMultiply(long long left, long long right, ValueType resultType) {
    __int128 result = static_cast<__int128>(left) * static_cast<__int128>(right);
    return ensureRange128(result, resultType, "Integer overflow during multiplication");
}

long long checkedPower(long long base, long long exponent, ValueType resultType) {
    if (exponent < 0) {
        throw std::runtime_error("Negative exponent is not supported");
    }

    long long result = 1;
    long long currentBase = base;
    long long currentExponent = exponent;

    while (currentExponent > 0) {
        if (currentExponent % 2 == 1) {
            result = checkedMultiply(result, currentBase, resultType);
        }

        currentExponent /= 2;
        if (currentExponent > 0) {
            currentBase = checkedMultiply(currentBase, currentBase, resultType);
        }
    }

    return ensureRange(result, resultType, "Integer overflow during power");
}

long long checkedDivide(long long left, long long right, ValueType resultType) {
    if (right == 0) {
        throw std::runtime_error("Division by zero");
    }

    __int128 result = static_cast<__int128>(left) / static_cast<__int128>(right);
    return ensureRange128(result, resultType, "Integer overflow during division");
}

long long checkedModulo(long long left, long long right, ValueType resultType) {
    if (right == 0) {
        throw std::runtime_error("Modulo by zero");
    }

    long long result = left % right;
    return ensureRange(result, resultType, "Integer overflow during modulo");
}

std::string rangeErrorMessage(long long value, ValueType targetType) {
    return "Value " + std::to_string(value) + " is outside " +
           std::string(valueTypeToString(targetType)) + " range";
}

}  // namespace

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

    auto runtimeErr = [&](const std::string& msg, const std::string& tok = "") -> RuntimeError {
        return RuntimeError(msg, instructions[ip].line, tok);
    };

    auto storeDeclaredValue = [&](const std::string& name, ValueType targetType) {
        TypedValue value = pop();
        long long castValue = targetType == ValueType::Bool
            ? normalizeBool(value.value)
            : ensureRange(value.value, targetType, rangeErrorMessage(value.value, targetType));
        variables[name] = VariableValue{castValue, targetType};
    };

    auto assignExistingValue = [&](const std::string& name) {
        TypedValue value = pop();
        auto it = variables.find(name);
        if (it == variables.end()) {
            throw runtimeErr("Undefined variable '" + name + "'", name);
        }

        long long castValue = it->second.type == ValueType::Bool
            ? normalizeBool(value.value)
            : ensureRange(value.value, it->second.type, rangeErrorMessage(value.value, it->second.type));
        it->second.value = castValue;
    };

    auto arithmeticToken = [](long long left, const std::string& op, long long right) {
        return std::to_string(left) + op + std::to_string(right);
    };

    while (ip < instructions.size()) {
        const Instruction& instr = instructions[ip];

        switch (instr.opcode) {
            case OpCode::PushBool:
                if (instr.operand == "0") {
                    push(0, ValueType::Bool);
                } else {
                    push(1, ValueType::Bool);
                }
                break;

            case OpCode::PushInt: {
                try {
                    auto [value, type] = parseNumericText(instr.operand, false);
                    push(value, type);
                } catch (const std::exception& error) {
                    throw runtimeErr(error.what(), instr.operand);
                }
                break;
            }

            case OpCode::Input: {
                std::string line;
                output << "input> ";
                if (!std::getline(input, line)) {
                    throw runtimeErr("Failed to read input");
                }

                while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
                    line.pop_back();
                }

                try {
                    auto [value, type] = parseNumericText(line, true);
                    push(value, type);
                } catch (const std::exception& error) {
                    throw runtimeErr(error.what(), "input");
                }
                break;
            }

            case OpCode::LoadVar: {
                auto it = variables.find(instr.operand);
                if (it == variables.end()) {
                    throw runtimeErr("Undefined variable '" + instr.operand + "'", instr.operand);
                }

                push(it->second.value, it->second.type);
                break;
            }

            case OpCode::DeclareInt:
                try {
                    storeDeclaredValue(instr.operand, ValueType::Int);
                } catch (const std::exception& error) {
                    throw runtimeErr(error.what(), instr.operand);
                }
                break;

            case OpCode::DeclareBool:
                try {
                    storeDeclaredValue(instr.operand, ValueType::Bool);
                } catch (const std::exception& error) {
                    throw runtimeErr(error.what(), instr.operand);
                }
                break;

            case OpCode::DeclareLong:
                try {
                    storeDeclaredValue(instr.operand, ValueType::Long);
                } catch (const std::exception& error) {
                    throw runtimeErr(error.what(), instr.operand);
                }
                break;

            case OpCode::StoreVar:
                try {
                    assignExistingValue(instr.operand);
                } catch (const RuntimeError&) {
                    throw;
                } catch (const std::exception& error) {
                    throw runtimeErr(error.what(), instr.operand);
                }
                break;

            case OpCode::Add: {
                TypedValue right = pop();
                TypedValue left = pop();
                ValueType resultType = numericResultType(left.type, right.type);
                try {
                    push(checkedAdd(left.value, right.value, resultType), resultType);
                } catch (const std::exception& error) {
                    throw runtimeErr(error.what(), arithmeticToken(left.value, "+", right.value));
                }
                break;
            }

            case OpCode::Subtract: {
                TypedValue right = pop();
                TypedValue left = pop();
                ValueType resultType = numericResultType(left.type, right.type);
                try {
                    push(checkedSubtract(left.value, right.value, resultType), resultType);
                } catch (const std::exception& error) {
                    throw runtimeErr(error.what(), arithmeticToken(left.value, "-", right.value));
                }
                break;
            }

            case OpCode::Power: {
                TypedValue right = pop();
                TypedValue left = pop();
                ValueType resultType = numericResultType(left.type, right.type);
                try {
                    push(checkedPower(left.value, right.value, resultType), resultType);
                } catch (const std::exception& error) {
                    throw runtimeErr(error.what(), arithmeticToken(left.value, "^", right.value));
                }
                break;
            }

            case OpCode::Multiply: {
                TypedValue right = pop();
                TypedValue left = pop();
                ValueType resultType = numericResultType(left.type, right.type);
                try {
                    push(checkedMultiply(left.value, right.value, resultType), resultType);
                } catch (const std::exception& error) {
                    throw runtimeErr(error.what(), arithmeticToken(left.value, "*", right.value));
                }
                break;
            }

            case OpCode::Divide: {
                TypedValue right = pop();
                TypedValue left = pop();
                ValueType resultType = numericResultType(left.type, right.type);
                try {
                    push(checkedDivide(left.value, right.value, resultType), resultType);
                } catch (const std::exception& error) {
                    throw runtimeErr(error.what(), arithmeticToken(left.value, "/", right.value));
                }
                break;
            }

            case OpCode::Modulo: {
                TypedValue right = pop();
                TypedValue left = pop();
                ValueType resultType = numericResultType(left.type, right.type);
                try {
                    push(checkedModulo(left.value, right.value, resultType), resultType);
                } catch (const std::exception& error) {
                    throw runtimeErr(error.what(), arithmeticToken(left.value, "%", right.value));
                }
                break;
            }

            case OpCode::Equal: {
                TypedValue right = pop();
                TypedValue left = pop();
                push(left.value == right.value ? 1 : 0, ValueType::Bool);
                break;
            }

            case OpCode::NotEqual: {
                TypedValue right = pop();
                TypedValue left = pop();
                push(left.value != right.value ? 1 : 0, ValueType::Bool);
                break;
            }

            case OpCode::Less: {
                TypedValue right = pop();
                TypedValue left = pop();
                push(left.value < right.value ? 1 : 0, ValueType::Bool);
                break;
            }

            case OpCode::LessEqual: {
                TypedValue right = pop();
                TypedValue left = pop();
                push(left.value <= right.value ? 1 : 0, ValueType::Bool);
                break;
            }

            case OpCode::Greater: {
                TypedValue right = pop();
                TypedValue left = pop();
                push(left.value > right.value ? 1 : 0, ValueType::Bool);
                break;
            }

            case OpCode::GreaterEqual: {
                TypedValue right = pop();
                TypedValue left = pop();
                push(left.value >= right.value ? 1 : 0, ValueType::Bool);
                break;
            }

            case OpCode::And: {
                TypedValue right = pop();
                TypedValue left = pop();
                push((left.value != 0 && right.value != 0) ? 1 : 0, ValueType::Bool);
                break;
            }

            case OpCode::Or: {
                TypedValue right = pop();
                TypedValue left = pop();
                push((left.value != 0 || right.value != 0) ? 1 : 0, ValueType::Bool);
                break;
            }

            case OpCode::Not: {
                TypedValue value = pop();
                push(value.value == 0 ? 1 : 0, ValueType::Bool);
                break;
            }

            case OpCode::BitAnd: {
                TypedValue right = pop();
                TypedValue left = pop();
                if (left.type == ValueType::Bool || right.type == ValueType::Bool) {
                    throw runtimeErr("Bitwise '&' is not supported on bool values");
                }
                ValueType resultType = numericResultType(left.type, right.type);
                push(left.value & right.value, resultType);
                break;
            }

            case OpCode::BitOr: {
                TypedValue right = pop();
                TypedValue left = pop();
                if (left.type == ValueType::Bool || right.type == ValueType::Bool) {
                    throw runtimeErr("Bitwise '|' is not supported on bool values");
                }
                ValueType resultType = numericResultType(left.type, right.type);
                push(left.value | right.value, resultType);
                break;
            }

            case OpCode::BitXor: {
                TypedValue right = pop();
                TypedValue left = pop();
                if (left.type == ValueType::Bool || right.type == ValueType::Bool) {
                    throw runtimeErr("Bitwise '^' is not supported on bool values");
                }
                ValueType resultType = numericResultType(left.type, right.type);
                push(left.value ^ right.value, resultType);
                break;
            }

            case OpCode::ShiftLeft: {
                TypedValue right = pop();
                TypedValue left = pop();
                if (left.type == ValueType::Bool || right.type == ValueType::Bool) {
                    throw runtimeErr("Shift '<<' is not supported on bool values");
                }
                if (right.value < 0) {
                    throw runtimeErr("Shift amount cannot be negative: " + std::to_string(right.value));
                }
                if (right.value >= 64) {
                    throw runtimeErr("Shift amount too large: " + std::to_string(right.value));
                }
                ValueType resultType = numericResultType(left.type, right.type);
                push(left.value << right.value, resultType);
                break;
            }

            case OpCode::ShiftRight: {
                TypedValue right = pop();
                TypedValue left = pop();
                if (left.type == ValueType::Bool || right.type == ValueType::Bool) {
                    throw runtimeErr("Shift '>>' is not supported on bool values");
                }
                if (right.value < 0) {
                    throw runtimeErr("Shift amount cannot be negative: " + std::to_string(right.value));
                }
                if (right.value >= 64) {
                    throw runtimeErr("Shift amount too large: " + std::to_string(right.value));
                }
                ValueType resultType = numericResultType(left.type, right.type);
                push(left.value >> right.value, resultType);
                break;
            }

            case OpCode::BitNot: {
                TypedValue value = pop();
                if (value.type == ValueType::Bool) {
                    throw runtimeErr("Bitwise '~' is not supported on bool values");
                }
                push(~value.value, value.type);
                break;
            }

            case OpCode::Jump:
                ip = static_cast<std::size_t>(std::stoull(instr.operand));
                continue;

            case OpCode::JumpIfFalse: {
                TypedValue condition = pop();
                if (condition.value == 0) {
                    ip = static_cast<std::size_t>(std::stoull(instr.operand));
                    continue;
                }
                break;
            }

            case OpCode::Print: {
                TypedValue value = pop();
                output << value.value << '\n';
                break;
            }

            case OpCode::Pop:
                pop();
                break;

            case OpCode::Halt:
                return;

            default:
                throw VMError("Unknown opcode encountered: " +
                              std::to_string(static_cast<int>(instr.opcode)));
        }

        ip++;
    }
}

void VM::push(long long value, ValueType type) {
    stack.push_back(TypedValue{value, type});
}

VM::TypedValue VM::pop() {
    if (stack.empty()) {
        throw VMError("Stack underflow during operation");
    }

    TypedValue value = stack.back();
    stack.pop_back();
    return value;
}