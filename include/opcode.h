#ifndef OPCODE_H
#define OPCODE_H

#include <string>
#include <utility>

// ─── Value types ─────────────────────────────────────────────────────────────

enum class ValueType {
    Unknown,
    Bool,
    Int,
    LongLong,   // long long int  (64-bit signed)
    Float       // double-precision floating point
};

inline const char* valueTypeToString(ValueType type) {
    switch (type) {
        case ValueType::Bool:     return "bool";
        case ValueType::Int:      return "int";
        case ValueType::LongLong: return "long long int";
        case ValueType::Float:    return "float";
        case ValueType::Unknown:  return "unknown";
        default:                  return "unknown";
    }
}

// ─── Opcodes ─────────────────────────────────────────────────────────────────

enum class OpCode {
    // ── Push literals ────────────────────────────────────────────────────────
    PushBool,       // operand = "0" or "1"
    PushInt,        // operand = decimal integer text  → type Int
    PushLongLong,   // operand = decimal integer text  → type LongLong  (LL suffix)
    PushFloat,      // operand = decimal float text    → type Float
    // ── I/O ──────────────────────────────────────────────────────────────────
    Input,
    // ── Variables ────────────────────────────────────────────────────────────
    LoadVar,
    DeclareBool,
    DeclareInt,
    DeclareLongLong,
    DeclareFloat,
    StoreVar,
    // ── Explicit casts ────────────────────────────────────────────────────────
    CastToInt,
    CastToLongLong,
    CastToFloat,
    CastToBool,
    // ── Arithmetic ───────────────────────────────────────────────────────────
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Power,
    // ── Comparison ───────────────────────────────────────────────────────────
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    // ── Logical (short-circuit compiled away to jumps; kept for dead-code) ───
    And,
    Or,
    Not,
    // ── Bitwise (integer only) ───────────────────────────────────────────────
    BitAnd,       // &
    BitOr,        // |
    BitXor,       // ^
    BitNot,       // ~  (unary)
    ShiftLeft,    // <<
    ShiftRight,   // >>
    // ── Control flow ─────────────────────────────────────────────────────────
    Jump,
    JumpIfFalse,
    // ── Scope management ─────────────────────────────────────────────────────
    PushScope,    // enter a new variable scope
    PopScope,     // exit the current variable scope (drops all its variables)
    // ── Misc ─────────────────────────────────────────────────────────────────
    Print,
    Pop,
    Halt
};

struct Instruction {
    OpCode      opcode;
    std::string operand;
    int         line = 0;

    Instruction(OpCode opcode, std::string operand = "", int line = 0)
        : opcode(opcode), operand(std::move(operand)), line(line) {}
};

inline const char* opcodeToString(OpCode opcode) {
    switch (opcode) {
        case OpCode::PushBool:        return "PushBool";
        case OpCode::PushInt:         return "PushInt";
        case OpCode::PushLongLong:    return "PushLongLong";
        case OpCode::PushFloat:       return "PushFloat";
        case OpCode::Input:           return "Input";
        case OpCode::LoadVar:         return "LoadVar";
        case OpCode::DeclareBool:     return "DeclareBool";
        case OpCode::DeclareInt:      return "DeclareInt";
        case OpCode::DeclareLongLong: return "DeclareLongLong";
        case OpCode::DeclareFloat:    return "DeclareFloat";
        case OpCode::StoreVar:        return "StoreVar";
        case OpCode::CastToInt:       return "CastToInt";
        case OpCode::CastToLongLong:  return "CastToLongLong";
        case OpCode::CastToFloat:     return "CastToFloat";
        case OpCode::CastToBool:      return "CastToBool";
        case OpCode::Add:             return "Add";
        case OpCode::Subtract:        return "Subtract";
        case OpCode::Multiply:        return "Multiply";
        case OpCode::Divide:          return "Divide";
        case OpCode::Modulo:          return "Modulo";
        case OpCode::Power:           return "Power";
        case OpCode::Equal:           return "Equal";
        case OpCode::NotEqual:        return "NotEqual";
        case OpCode::Less:            return "Less";
        case OpCode::LessEqual:       return "LessEqual";
        case OpCode::Greater:         return "Greater";
        case OpCode::GreaterEqual:    return "GreaterEqual";
        case OpCode::And:             return "And";
        case OpCode::Or:              return "Or";
        case OpCode::Not:             return "Not";
        case OpCode::BitAnd:          return "BitAnd";
        case OpCode::BitOr:           return "BitOr";
        case OpCode::BitXor:          return "BitXor";
        case OpCode::BitNot:          return "BitNot";
        case OpCode::ShiftLeft:       return "ShiftLeft";
        case OpCode::ShiftRight:      return "ShiftRight";
        case OpCode::Jump:            return "Jump";
        case OpCode::JumpIfFalse:     return "JumpIfFalse";
        case OpCode::PushScope:       return "PushScope";
        case OpCode::PopScope:        return "PopScope";
        case OpCode::Print:           return "Print";
        case OpCode::Pop:             return "Pop";
        case OpCode::Halt:            return "Halt";
        default:                      return "Unknown";
    }
}

#endif