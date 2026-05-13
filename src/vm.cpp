#include "vm.h"

#include <iostream>
#include <limits>
#include <string>
#include <stdexcept>

namespace {

int ensureIntRange(long long value, const std::string& context) {
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
        throw std::runtime_error(context);
    }

    return static_cast<int>(value);
}

int parseIntValue(const std::string& text, const std::string& context) {
    try {
        std::size_t parsedChars = 0;
        long long value = std::stoll(text, &parsedChars);

        if (parsedChars != text.size()) {
            throw std::runtime_error("Invalid integer " + context + ": " + text);
        }

        return ensureIntRange(value, "Integer " + context + " is outside 32-bit int range: " + text);
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("Invalid integer " + context + ": " + text);
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Integer " + context + " is outside 32-bit int range: " + text);
    }
}

int checkedAdd(int left, int right) {
    return ensureIntRange(static_cast<long long>(left) + static_cast<long long>(right),
                          "Integer overflow during addition.");
}

int checkedSubtract(int left, int right) {
    return ensureIntRange(static_cast<long long>(left) - static_cast<long long>(right),
                          "Integer overflow during subtraction.");
}

int checkedMultiply(int left, int right) {
    return ensureIntRange(static_cast<long long>(left) * static_cast<long long>(right),
                          "Integer overflow during multiplication.");
}

int checkedDivide(int left, int right) {
    if (right == 0) {
        throw std::runtime_error("Division by zero.");
    }

    return ensureIntRange(static_cast<long long>(left) / static_cast<long long>(right),
                          "Integer overflow during division.");
}

}  // namespace

void VM::execute(const std::vector<Instruction>& instructions) {
    execute(instructions, std::cin, std::cout);
}

void VM::execute(const std::vector<Instruction>& instructions, std::istream& input, std::ostream& output) {
    stack.clear();
    variables.clear();

    std::size_t ip = 0;

    while (ip < instructions.size()) {
        const Instruction& instruction = instructions[ip];

        switch (instruction.opcode) {
            case OpCode::PushInt:
                push(parseIntValue(instruction.operand, "literal"));
                break;

            case OpCode::Input: {
                std::string line;
                output << "input> ";
                if (!std::getline(input, line)) {
                    throw std::runtime_error("Failed to read input.");
                }
                push(parseIntValue(line, "input"));
                break;
            }

            case OpCode::LoadVar: {
                auto it = variables.find(instruction.operand);
                if (it == variables.end()) {
                    throw std::runtime_error("Undefined variable: " + instruction.operand);
                }
                push(it->second);
                break;
            }

            case OpCode::StoreVar: {
                int value = pop();
                variables[instruction.operand] = value;
                break;
            }

            case OpCode::Add: {
                int right = pop();
                int left = pop();
                push(checkedAdd(left, right));
                break;
            }

            case OpCode::Subtract: {
                int right = pop();
                int left = pop();
                push(checkedSubtract(left, right));
                break;
            }

            case OpCode::Multiply: {
                int right = pop();
                int left = pop();
                push(checkedMultiply(left, right));
                break;
            }

            case OpCode::Divide: {
                int right = pop();
                int left = pop();
                push(checkedDivide(left, right));
                break;
            }

            case OpCode::Equal: {
                int right = pop();
                int left = pop();
                push(left == right ? 1 : 0);
                break;
            }

            case OpCode::Less: {
                int right = pop();
                int left = pop();
                push(left < right ? 1 : 0);
                break;
            }

            case OpCode::Jump:
                ip = static_cast<std::size_t>(std::stoul(instruction.operand));
                continue;

            case OpCode::JumpIfFalse: {
                int condition = pop();
                if (condition == 0) {
                    ip = static_cast<std::size_t>(std::stoul(instruction.operand));
                    continue;
                }
                break;
            }

            case OpCode::Print: {
                int value = pop();
                output << value << '\n';
                break;
            }

            case OpCode::Pop:
                pop();
                break;

            case OpCode::Halt:
                return;
        }

        ip++;
    }
}

void VM::push(int value) {
    stack.push_back(value);
}

int VM::pop() {
    if (stack.empty()) {
        throw std::runtime_error("Stack underflow.");
    }

    int value = stack.back();
    stack.pop_back();
    return value;
}
