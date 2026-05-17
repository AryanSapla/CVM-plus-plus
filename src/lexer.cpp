#include "lexer.h"
#include <cctype>

Lexer::Lexer(const std::string& source) : source(source), current(0), line(1) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;

        char c = peek();

        if (c == '/' && peekNext() == '/') {
            skipLineComment();
            continue;
        }

        if (c == '#') {
            skipHashComment();
            continue;
        }

        if (c == '/' && peekNext() == '*') {
            skipBlockComment();
            continue;
        }

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
            case '+': tokens.push_back(makeToken(TokenType::Plus,      "+")); break;
            case '-': tokens.push_back(makeToken(TokenType::Minus,     "-")); break;
            case '^': tokens.push_back(makeToken(TokenType::Caret,     "^")); break;
            case '*': tokens.push_back(makeToken(TokenType::Star,      "*")); break;
            case '%': tokens.push_back(makeToken(TokenType::Star,      "%")); break; // reuse Star slot — handled in parser
            case ';': tokens.push_back(makeToken(TokenType::Semicolon, ";")); break;
            case '(': tokens.push_back(makeToken(TokenType::LeftParen, "(")); break;
            case ')': tokens.push_back(makeToken(TokenType::RightParen,")")); break;
            case '{': tokens.push_back(makeToken(TokenType::LeftBrace, "{")); break;
            case '}': tokens.push_back(makeToken(TokenType::RightBrace,"}")); break;

            case '/':
                tokens.push_back(makeToken(TokenType::Slash, "/"));
                break;

            case '<':
                if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.push_back(makeToken(TokenType::LessEqual, "<="));
                } else {
                    tokens.push_back(makeToken(TokenType::Less, "<"));
                }
                break;

            case '>':
                if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.push_back(makeToken(TokenType::GreaterEqual, ">="));
                } else {
                    tokens.push_back(makeToken(TokenType::Greater, ">"));
                }
                break;

            case '!':
                if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.push_back(makeToken(TokenType::NotEqual, "!="));
                } else {
                    throw LexerError("Unexpected character '!'. Did you mean '!='?", line, "!");
                }
                break;

            case '=':
                if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.push_back(makeToken(TokenType::EqualEqual, "=="));
                } else {
                    tokens.push_back(makeToken(TokenType::Equal, "="));
                }
                break;

            default: {
                std::string msg = "Unexpected character '" + std::string(1, c) + "'";
                throw LexerError(msg, line, std::string(1, c));
            }
        }
    }

    tokens.push_back(makeToken(TokenType::EndOfFile, ""));
    return tokens;
}

char Lexer::advance() {
    return source[current++];
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::peekNext() const {
    if (current + 1 >= source.size()) return '\0';
    return source[current + 1];
}

bool Lexer::isAtEnd() const {
    return current >= source.length();
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == '\n') {
            line++;
            advance();
        } else if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipLineComment() {
    advance();
    advance();
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}

void Lexer::skipHashComment() {
    advance();
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}

void Lexer::skipBlockComment() {
    int startLine = line;

    advance();
    advance();

    while (!isAtEnd()) {
        if (peek() == '\n') {
            line++;
            advance();
            continue;
        }

        if (peek() == '*' && peekNext() == '/') {
            advance();
            advance();
            return;
        }

        advance();
    }

    throw LexerError("Unterminated block comment", startLine, "/*");
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
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') advance();
        else break;
    }

    std::string text = source.substr(start, current - start);

    if (text == "let")   return makeToken(TokenType::Let,         text);
    if (text == "int")   return makeToken(TokenType::IntKeyword,  text);
    if (text == "long")  return makeToken(TokenType::LongKeyword, text);
    if (text == "print") return makeToken(TokenType::Print,       text);
    if (text == "input") return makeToken(TokenType::Input,       text);
    if (text == "if")    return makeToken(TokenType::If,          text);
    if (text == "else")  return makeToken(TokenType::Else,        text);
    if (text == "while") return makeToken(TokenType::While,       text);
    if (text == "true")  return makeToken(TokenType::True,        text);
    if (text == "false") return makeToken(TokenType::False,       text);
    if (text == "and")   return makeToken(TokenType::Identifier, text); // reserved but handled in parser
    if (text == "or")    return makeToken(TokenType::Identifier, text);
    if (text == "not")   return makeToken(TokenType::Identifier, text);

    return makeToken(TokenType::Identifier, text);
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) const {
    return Token{type, lexeme, line};
}
