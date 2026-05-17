#ifndef OPCODE_H
#define OPCODE_H

#include <string>
#include <utility>

enum class ValueType {
    Unknown,
    Bool,
    Int,
    Long
};

inline const char* valueTypeToString(ValueType type) {
    switch (type) {
        case ValueType::Bool: return "bool";
        case ValueType::Int: return "int";
        case ValueType::Long: return "long";
        case ValueType::Unknown: return "unknown";
        default: return "unknown";
    }
}

enum class OpCode {
    PushBool,     // operand = "0" or "1"
    PushInt,      // operand = decimal integer literal text
    Input,
    LoadVar,
    DeclareBool,
    DeclareInt,
    DeclareLong,
    StoreVar,
    Add,
    Subtract,
    Power,
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
        case OpCode::PushBool:     return "PushBool";
        case OpCode::PushInt:      return "PushInt";
        case OpCode::Input:        return "Input";
        case OpCode::LoadVar:      return "LoadVar";
        case OpCode::DeclareBool:  return "DeclareBool";
        case OpCode::DeclareInt:   return "DeclareInt";
        case OpCode::DeclareLong:  return "DeclareLong";
        case OpCode::StoreVar:     return "StoreVar";
        case OpCode::Add:          return "Add";
        case OpCode::Subtract:     return "Subtract";
        case OpCode::Power:        return "Power";
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
