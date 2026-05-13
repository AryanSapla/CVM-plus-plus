#ifndef OPCODE_H
#define OPCODE_H

#include <string>
#include <utility>

enum class OpCode {
    PushInt,
    Input,
    LoadVar,
    StoreVar,
    Add,
    Subtract,
    Multiply,
    Divide,
    Equal,
    Less,
    Jump,
    JumpIfFalse,
    Print,
    Pop,
    Halt
};

struct Instruction {
    OpCode opcode;
    std::string operand;

    Instruction(OpCode opcode, std::string operand = "")
        : opcode(opcode), operand(std::move(operand)) {}
};

inline const char* opcodeToString(OpCode opcode) {
    switch (opcode) {
        case OpCode::PushInt: return "PushInt";
        case OpCode::Input: return "Input";
        case OpCode::LoadVar: return "LoadVar";
        case OpCode::StoreVar: return "StoreVar";
        case OpCode::Add: return "Add";
        case OpCode::Subtract: return "Subtract";
        case OpCode::Multiply: return "Multiply";
        case OpCode::Divide: return "Divide";
        case OpCode::Equal: return "Equal";
        case OpCode::Less: return "Less";
        case OpCode::Jump: return "Jump";
        case OpCode::JumpIfFalse: return "JumpIfFalse";
        case OpCode::Print: return "Print";
        case OpCode::Pop: return "Pop";
        case OpCode::Halt: return "Halt";
        default: return "Unknown";
    }
}

#endif
