#ifndef OPCODE_H
#define OPCODE_H

#include <stdexcept>
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

inline OpCode opcodeFromString(const std::string& opcode) {
    if (opcode == "PushBool")        return OpCode::PushBool;
    if (opcode == "PushInt")         return OpCode::PushInt;
    if (opcode == "PushLongLong")    return OpCode::PushLongLong;
    if (opcode == "PushFloat")       return OpCode::PushFloat;
    if (opcode == "Input")           return OpCode::Input;
    if (opcode == "LoadVar")         return OpCode::LoadVar;
    if (opcode == "DeclareBool")     return OpCode::DeclareBool;
    if (opcode == "DeclareInt")      return OpCode::DeclareInt;
    if (opcode == "DeclareLongLong") return OpCode::DeclareLongLong;
    if (opcode == "DeclareFloat")    return OpCode::DeclareFloat;
    if (opcode == "StoreVar")        return OpCode::StoreVar;
    if (opcode == "CastToInt")       return OpCode::CastToInt;
    if (opcode == "CastToLongLong")  return OpCode::CastToLongLong;
    if (opcode == "CastToFloat")     return OpCode::CastToFloat;
    if (opcode == "CastToBool")      return OpCode::CastToBool;
    if (opcode == "Add")             return OpCode::Add;
    if (opcode == "Subtract")        return OpCode::Subtract;
    if (opcode == "Multiply")        return OpCode::Multiply;
    if (opcode == "Divide")          return OpCode::Divide;
    if (opcode == "Modulo")          return OpCode::Modulo;
    if (opcode == "Power")           return OpCode::Power;
    if (opcode == "Equal")           return OpCode::Equal;
    if (opcode == "NotEqual")        return OpCode::NotEqual;
    if (opcode == "Less")            return OpCode::Less;
    if (opcode == "LessEqual")       return OpCode::LessEqual;
    if (opcode == "Greater")         return OpCode::Greater;
    if (opcode == "GreaterEqual")    return OpCode::GreaterEqual;
    if (opcode == "And")             return OpCode::And;
    if (opcode == "Or")              return OpCode::Or;
    if (opcode == "Not")             return OpCode::Not;
    if (opcode == "BitAnd")          return OpCode::BitAnd;
    if (opcode == "BitOr")           return OpCode::BitOr;
    if (opcode == "BitXor")          return OpCode::BitXor;
    if (opcode == "BitNot")          return OpCode::BitNot;
    if (opcode == "ShiftLeft")       return OpCode::ShiftLeft;
    if (opcode == "ShiftRight")      return OpCode::ShiftRight;
    if (opcode == "Jump")            return OpCode::Jump;
    if (opcode == "JumpIfFalse")     return OpCode::JumpIfFalse;
    if (opcode == "PushScope")       return OpCode::PushScope;
    if (opcode == "PopScope")        return OpCode::PopScope;
    if (opcode == "Print")           return OpCode::Print;
    if (opcode == "Pop")             return OpCode::Pop;
    if (opcode == "Halt")            return OpCode::Halt;
    throw std::runtime_error("Unknown opcode text: " + opcode);
}

#endif
