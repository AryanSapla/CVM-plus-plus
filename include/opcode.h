#ifndef OPCODE_H
#define OPCODE_H

#include <string>
#include <utility>

enum class OpCode {
    PushInt,      // operand = decimal integer literal (may be long long)
    Input,
    LoadVar,
    StoreVar,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    And,
    Or,
    Not,
    Jump,
    JumpIfFalse,
    Print,
    Pop,
    Halt
};

struct Instruction {
    OpCode opcode;
    std::string operand;
    int line = 0;

    Instruction(OpCode opcode, std::string operand = "", int line = 0)
        : opcode(opcode), operand(std::move(operand)), line(line) {}
};

inline const char* opcodeToString(OpCode opcode) {
    switch (opcode) {
        case OpCode::PushInt:      return "PushInt";
        case OpCode::Input:        return "Input";
        case OpCode::LoadVar:      return "LoadVar";
        case OpCode::StoreVar:     return "StoreVar";
        case OpCode::Add:          return "Add";
        case OpCode::Subtract:     return "Subtract";
        case OpCode::Multiply:     return "Multiply";
        case OpCode::Divide:       return "Divide";
        case OpCode::Modulo:       return "Modulo";
        case OpCode::Equal:        return "Equal";
        case OpCode::NotEqual:     return "NotEqual";
        case OpCode::Less:         return "Less";
        case OpCode::LessEqual:    return "LessEqual";
        case OpCode::Greater:      return "Greater";
        case OpCode::GreaterEqual: return "GreaterEqual";
        case OpCode::And:          return "And";
        case OpCode::Or:           return "Or";
        case OpCode::Not:          return "Not";
        case OpCode::Jump:         return "Jump";
        case OpCode::JumpIfFalse:  return "JumpIfFalse";
        case OpCode::Print:        return "Print";
        case OpCode::Pop:          return "Pop";
        case OpCode::Halt:         return "Halt";
        default:                   return "Unknown";
    }
}

#endif