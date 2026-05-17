#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    Number,
    Identifier,
    Let,
    IntKeyword,
    LongKeyword,
    Print,
    Input,
    If,
    Else,
    While,
    True,
    False,
    Plus,
    Minus,
    Caret,
    Star,
    Slash,
    Equal,
    EqualEqual,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Semicolon,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    EndOfFile,
    Invalid
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
};

inline const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::Number:       return "Number";
        case TokenType::Identifier:   return "Identifier";
        case TokenType::Let:          return "Let";
        case TokenType::IntKeyword:   return "IntKeyword";
        case TokenType::LongKeyword:  return "LongKeyword";
        case TokenType::Print:        return "Print";
        case TokenType::Input:        return "Input";
        case TokenType::If:           return "If";
        case TokenType::Else:         return "Else";
        case TokenType::While:        return "While";
        case TokenType::True:         return "True";
        case TokenType::False:        return "False";
        case TokenType::Plus:         return "Plus";
        case TokenType::Minus:        return "Minus";
        case TokenType::Caret:        return "Caret";
        case TokenType::Star:         return "Star";
        case TokenType::Slash:        return "Slash";
        case TokenType::Equal:        return "Equal";
        case TokenType::EqualEqual:   return "EqualEqual";
        case TokenType::NotEqual:     return "NotEqual";
        case TokenType::Less:         return "Less";
        case TokenType::LessEqual:    return "LessEqual";
        case TokenType::Greater:      return "Greater";
        case TokenType::GreaterEqual: return "GreaterEqual";
        case TokenType::Semicolon:    return "Semicolon";
        case TokenType::LeftParen:    return "LeftParen";
        case TokenType::RightParen:   return "RightParen";
        case TokenType::LeftBrace:    return "LeftBrace";
        case TokenType::RightBrace:   return "RightBrace";
        case TokenType::EndOfFile:    return "EndOfFile";
        case TokenType::Invalid:      return "Invalid";
        default:                      return "Unknown";
    }
}

#endif
