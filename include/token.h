#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    // Literals
    Number,           // integer or float digits
    Identifier,
    // Type keywords
    Let,
    IntKeyword,
    LongKeyword,      // 'long' — combined with 'long' 'int' in parser
    BoolKeyword,
    FloatKeyword,
    // Other keywords
    Print,
    Input,
    If,
    Else,
    While,
    For,
    Break,
    Continue,
    True,
    False,
    // Keyword operators (and / or / not — reserved, cannot be identifiers)
    AndKw,
    OrKw,
    NotKw,
    // Suffix token
    LL,               // LL  suffix on integer literals  e.g. 42LL
    // Arithmetic operators
    Plus,
    Minus,
    PlusPlus,         // ++
    MinusMinus,       // --
    CaretCaret,       // ^^  power
    Caret,            // ^   bitwise XOR
    Star,             // *
    Percent,          // %   modulo
    Slash,            // /
    // Unary
    Bang,             // !
    Tilde,            // ~   bitwise NOT
    // Assignment / comparison
    Equal,            // =
    EqualEqual,       // ==
    NotEqual,         // !=
    // Logical
    AndAnd,           // &&
    OrOr,             // ||
    // Bitwise
    Ampersand,        // &
    Pipe,             // |
    // Shift / comparison (two-char first)
    LessLess,         // <<
    LessEqual,        // <=
    Less,             // <
    GreaterGreater,   // >>
    GreaterEqual,     // >=
    Greater,          // >
    // Punctuation
    Semicolon,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    // Special
    EndOfFile,
    Invalid
};

struct Token {
    TokenType   type;
    std::string lexeme;
    int         line;
};

inline const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::Number:          return "Number";
        case TokenType::Identifier:      return "Identifier";
        case TokenType::Let:             return "Let";
        case TokenType::IntKeyword:      return "IntKeyword";
        case TokenType::LongKeyword:     return "LongKeyword";
        case TokenType::BoolKeyword:     return "BoolKeyword";
        case TokenType::FloatKeyword:    return "FloatKeyword";
        case TokenType::Print:           return "Print";
        case TokenType::Input:           return "Input";
        case TokenType::If:              return "If";
        case TokenType::Else:            return "Else";
        case TokenType::While:           return "While";
        case TokenType::For:             return "For";
        case TokenType::Break:           return "Break";
        case TokenType::Continue:        return "Continue";
        case TokenType::True:            return "True";
        case TokenType::False:           return "False";
        case TokenType::AndKw:           return "AndKw";
        case TokenType::OrKw:            return "OrKw";
        case TokenType::NotKw:           return "NotKw";
        case TokenType::LL:              return "LL";
        case TokenType::Plus:            return "Plus";
        case TokenType::Minus:           return "Minus";
        case TokenType::PlusPlus:        return "PlusPlus";
        case TokenType::MinusMinus:      return "MinusMinus";
        case TokenType::CaretCaret:      return "CaretCaret";
        case TokenType::Caret:           return "Caret";
        case TokenType::Star:            return "Star";
        case TokenType::Percent:         return "Percent";
        case TokenType::Slash:           return "Slash";
        case TokenType::Bang:            return "Bang";
        case TokenType::Tilde:           return "Tilde";
        case TokenType::Equal:           return "Equal";
        case TokenType::EqualEqual:      return "EqualEqual";
        case TokenType::NotEqual:        return "NotEqual";
        case TokenType::AndAnd:          return "AndAnd";
        case TokenType::OrOr:            return "OrOr";
        case TokenType::Ampersand:       return "Ampersand";
        case TokenType::Pipe:            return "Pipe";
        case TokenType::LessLess:        return "LessLess";
        case TokenType::LessEqual:       return "LessEqual";
        case TokenType::Less:            return "Less";
        case TokenType::GreaterGreater:  return "GreaterGreater";
        case TokenType::GreaterEqual:    return "GreaterEqual";
        case TokenType::Greater:         return "Greater";
        case TokenType::Semicolon:       return "Semicolon";
        case TokenType::LeftParen:       return "LeftParen";
        case TokenType::RightParen:      return "RightParen";
        case TokenType::LeftBrace:       return "LeftBrace";
        case TokenType::RightBrace:      return "RightBrace";
        case TokenType::EndOfFile:       return "EndOfFile";
        case TokenType::Invalid:         return "Invalid";
        default:                         return "Unknown";
    }
}

#endif