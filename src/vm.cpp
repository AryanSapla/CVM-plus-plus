#include "vm.h"

#include <iostream>
#include <string>
#include <stdexcept>

void VM::execute(const std::vector<Instruction>& instructions) {
    stack.clear();
    variables.clear();

    std::size_t ip = 0;

    while (ip < instructions.size()) {
        const Instruction& instruction = instructions[ip];

        switch (instruction.opcode) {
            case OpCode::PushInt:
                push(std::stoi(instruction.operand));
                break;

            case OpCode::Input: {
                std::string line;
                std::cout << "input> ";
                if (!std::getline(std::cin, line)) {
                    throw std::runtime_error("Failed to read input.");
                }
                push(std::stoi(line));
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
                push(left + right);
                break;
            }

            case OpCode::Subtract: {
                int right = pop();
                int left = pop();
                push(left - right);
                break;
            }

            case OpCode::Multiply: {
                int right = pop();
                int left = pop();
                push(left * right);
                break;
            }

            case OpCode::Divide: {
                int right = pop();
                int left = pop();

                if (right == 0) {
                    throw std::runtime_error("Division by zero.");
                }

                push(left / right);
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
                std::cout << value << '\n';
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
