#include "lexer.h"

#include <cctype>

Lexer::Lexer(const std::string& source) : source(source), current(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespace();

        if (isAtEnd()) {
            break;
        }

        char c = peek();

        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(number());
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(identifier());
            continue;
        }

        c = advance();

        switch (c) {
            case '+':
                tokens.push_back(makeToken(TokenType::Plus, "+"));
                break;
            case '-':
                tokens.push_back(makeToken(TokenType::Minus, "-"));
                break;
            case '*':
                tokens.push_back(makeToken(TokenType::Star, "*"));
                break;
            case '/':
                tokens.push_back(makeToken(TokenType::Slash, "/"));
                break;
            case '=':
                if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.push_back(makeToken(TokenType::EqualEqual, "=="));
                } else {
                    tokens.push_back(makeToken(TokenType::Equal, "="));
                }
                break;
            case '<':
                tokens.push_back(makeToken(TokenType::Less, "<"));
                break;
            case ';':
                tokens.push_back(makeToken(TokenType::Semicolon, ";"));
                break;
            case '(':
                tokens.push_back(makeToken(TokenType::LeftParen, "("));
                break;
            case ')':
                tokens.push_back(makeToken(TokenType::RightParen, ")"));
                break;
            default:
                tokens.push_back(makeToken(TokenType::Invalid, std::string(1, c)));
                break;
        }
    }

    tokens.push_back(makeToken(TokenType::EndOfFile, ""));
    return tokens;
}

char Lexer::advance() {
    return source[current++];
}

char Lexer::peek() const {
    if (isAtEnd()) {
        return '\0';
    }
    return source[current];
}

bool Lexer::isAtEnd() const {
    return current >= source.length();
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();

        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance();
        } else {
            break;
        }
    }
}

Token Lexer::number() {
    std::size_t start = current;

    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    return makeToken(TokenType::Number, source.substr(start, current - start));
}

Token Lexer::identifier() {
    std::size_t start = current;

    while (!isAtEnd()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            advance();
        } else {
            break;
        }
    }

    std::string text = source.substr(start, current - start);

    if (text == "let") {
        return makeToken(TokenType::Let, text);
    }

    if (text == "print") {
        return makeToken(TokenType::Print, text);
    }

    if (text == "true") {
        return makeToken(TokenType::True, text);
    }

    if (text == "false") {
        return makeToken(TokenType::False, text);
    }

    return makeToken(TokenType::Identifier, text);
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) const {
    return Token{type, lexeme};
}
