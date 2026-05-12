#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    Number,
    Identifier,
    Let,
    Print,
    Plus,
    Minus,
    Star,
    Slash,
    Equal,
    Semicolon,
    LeftParen,
    RightParen,
    EndOfFile,
    Invalid
};

struct Token {
    TokenType type;
    std::string lexeme;
};

inline const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::Number: return "Number";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Let: return "Let";
        case TokenType::Print: return "Print";
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::Slash: return "Slash";
        case TokenType::Equal: return "Equal";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::LeftParen: return "LeftParen";
        case TokenType::RightParen: return "RightParen";
        case TokenType::EndOfFile: return "EndOfFile";
        case TokenType::Invalid: return "Invalid";
        default: return "Unknown";
    }
}

#endif
