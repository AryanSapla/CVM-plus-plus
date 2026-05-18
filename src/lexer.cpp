#include "lexer.h"
#include <cctype>

Lexer::Lexer(const std::string& source) : source(source), current(0), line(1) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;

        char c = peek();

        // ── Comments ──────────────────────────────────────────────────────────
        if (c == '/' && peekNext() == '/') { skipLineComment();  continue; }
        if (c == '#')                      { skipHashComment();   continue; }
        if (c == '/' && peekNext() == '*') { skipBlockComment(); continue; }

        // ── Numbers (integer or float) ────────────────────────────────────────
        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(number());
            continue;
        }

        // ── Identifiers / keywords ────────────────────────────────────────────
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(identifier());
            continue;
        }

        c = advance();

        switch (c) {
            case '+':
                if (!isAtEnd() && peek() == '+') {
                    advance();
                    tokens.push_back(makeToken(TokenType::PlusPlus,   "++"));
                } else {
                    tokens.push_back(makeToken(TokenType::Plus,       "+"));
                }
                break;
            case '-':
                if (!isAtEnd() && peek() == '-') {
                    advance();
                    tokens.push_back(makeToken(TokenType::MinusMinus,  "--"));
                } else {
                    tokens.push_back(makeToken(TokenType::Minus,       "-"));
                }
                break;
            case '*': tokens.push_back(makeToken(TokenType::Star,      "*")); break;
            case '%': tokens.push_back(makeToken(TokenType::Percent,   "%")); break;
            case ';': tokens.push_back(makeToken(TokenType::Semicolon, ";")); break;
            case '(': tokens.push_back(makeToken(TokenType::LeftParen, "(")); break;
            case ')': tokens.push_back(makeToken(TokenType::RightParen,")")); break;
            case '{': tokens.push_back(makeToken(TokenType::LeftBrace, "{")); break;
            case '}': tokens.push_back(makeToken(TokenType::RightBrace,"}")); break;
            case '~': tokens.push_back(makeToken(TokenType::Tilde,     "~")); break;

            case '^':
                if (!isAtEnd() && peek() == '^') {
                    advance();
                    tokens.push_back(makeToken(TokenType::CaretCaret, "^^"));
                } else {
                    tokens.push_back(makeToken(TokenType::Caret, "^"));
                }
                break;

            case '/':
                tokens.push_back(makeToken(TokenType::Slash, "/"));
                break;

            case '<':
                if (!isAtEnd() && peek() == '<') {
                    advance();
                    tokens.push_back(makeToken(TokenType::LessLess, "<<"));
                } else if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.push_back(makeToken(TokenType::LessEqual, "<="));
                } else {
                    tokens.push_back(makeToken(TokenType::Less, "<"));
                }
                break;

            case '>':
                if (!isAtEnd() && peek() == '>') {
                    advance();
                    tokens.push_back(makeToken(TokenType::GreaterGreater, ">>"));
                } else if (!isAtEnd() && peek() == '=') {
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
                    tokens.push_back(makeToken(TokenType::Bang, "!"));
                }
                break;

            case '&':
                if (!isAtEnd() && peek() == '&') {
                    advance();
                    tokens.push_back(makeToken(TokenType::AndAnd, "&&"));
                } else {
                    tokens.push_back(makeToken(TokenType::Ampersand, "&"));
                }
                break;

            case '|':
                if (!isAtEnd() && peek() == '|') {
                    advance();
                    tokens.push_back(makeToken(TokenType::OrOr, "||"));
                } else {
                    tokens.push_back(makeToken(TokenType::Pipe, "|"));
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

char Lexer::advance() { return source[current++]; }

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::peekNext() const {
    if (current + 1 >= source.size()) return '\0';
    return source[current + 1];
}

bool Lexer::isAtEnd() const { return current >= source.length(); }

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if      (c == '\n')                        { line++; advance(); }
        else if (c == ' ' || c == '\t' || c == '\r') { advance(); }
        else break;
    }
}

void Lexer::skipLineComment() {
    advance(); advance();
    while (!isAtEnd() && peek() != '\n') advance();
}

void Lexer::skipHashComment() {
    advance();
    while (!isAtEnd() && peek() != '\n') advance();
}

void Lexer::skipBlockComment() {
    int startLine = line;
    advance(); advance();  // consume /*

    while (!isAtEnd()) {
        if (peek() == '\n') { line++; advance(); continue; }
        if (peek() == '*' && peekNext() == '/') { advance(); advance(); return; }
        advance();
    }
    throw LexerError("Unterminated block comment", startLine, "/*");
}

// ─── Number: integer (with optional LL suffix) or float ──────────────────────
//
// Recognised forms:
//   42          → Number  (type determined at VM runtime: Int or LongLong)
//   42LL        → Number token, immediately followed by LL token
//   3.14        → Number  (float — contains a dot)
//   3.14e2      → Number  (scientific float)
//   .5          → NOT supported (must start with digit)

Token Lexer::number() {
    std::size_t start = current;
    bool isFloat = false;

    // Integer part
    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) advance();

    // Optional fractional part
    if (!isAtEnd() && peek() == '.') {
        // Make sure it's not '..' or '.identifier' — just consume if next is digit
        if (current + 1 < source.size() &&
            std::isdigit(static_cast<unsigned char>(source[current + 1]))) {
            isFloat = true;
            advance(); // consume '.'
            while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
        }
    }

    // Optional exponent  (e/E followed by optional sign and digits)
    if (!isFloat && !isAtEnd() && (peek() == 'e' || peek() == 'E')) {
        // Only treat as float exponent if next char after e/E is digit or sign+digit
        std::size_t epos = current;
        advance(); // consume e/E
        if (!isAtEnd() && (peek() == '+' || peek() == '-')) advance();
        if (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
            isFloat = true;
            while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
        } else {
            // Not a valid exponent; rewind — 'e'/'E' will be picked up as identifier
            current = epos;
        }
    } else if (isFloat && !isAtEnd() && (peek() == 'e' || peek() == 'E')) {
        advance();
        if (!isAtEnd() && (peek() == '+' || peek() == '-')) advance();
        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
    }

    std::string text = source.substr(start, current - start);
    TokenType type = isFloat ? TokenType::Number : TokenType::Number;
    // We always use Number; the parser/compiler distinguishes float vs int via '.' in lexeme

    return makeToken(type, text);
}

// ─── Identifier / keyword ────────────────────────────────────────────────────
Token Lexer::identifier() {
    std::size_t start = current;
    while (!isAtEnd()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') advance();
        else break;
    }

    std::string text = source.substr(start, current - start);

    // Keywords
    if (text == "let")   return makeToken(TokenType::Let,          text);
    if (text == "int")   return makeToken(TokenType::IntKeyword,   text);
    if (text == "long")  return makeToken(TokenType::LongKeyword,  text);
    if (text == "bool")  return makeToken(TokenType::BoolKeyword,  text);
    if (text == "float") return makeToken(TokenType::FloatKeyword, text);
    if (text == "print") return makeToken(TokenType::Print,        text);
    if (text == "input") return makeToken(TokenType::Input,        text);
    if (text == "if")    return makeToken(TokenType::If,           text);
    if (text == "else")  return makeToken(TokenType::Else,         text);
    if (text == "while") return makeToken(TokenType::While,        text);
    if (text == "for")   return makeToken(TokenType::For,          text);
    if (text == "break") return makeToken(TokenType::Break,        text);
    if (text == "continue") return makeToken(TokenType::Continue,  text);
    if (text == "true")  return makeToken(TokenType::True,         text);
    if (text == "false") return makeToken(TokenType::False,        text);
    // and / or / not are now proper keyword tokens
    if (text == "and")   return makeToken(TokenType::AndKw,        text);
    if (text == "or")    return makeToken(TokenType::OrKw,         text);
    if (text == "not")   return makeToken(TokenType::NotKw,        text);
    // LL suffix — accept both upper and lower case: 42LL and 42ll
    if (text == "LL" || text == "ll") return makeToken(TokenType::LL, text);

    return makeToken(TokenType::Identifier, text);
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) const {
    return Token{type, lexeme, line};
}