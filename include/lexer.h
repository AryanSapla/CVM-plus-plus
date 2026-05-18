#ifndef LEXER_H
#define LEXER_H

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "token.h"

// ─── Lexer Error ──────────────────────────────────────────────────────────────
struct LexerError : std::runtime_error {
    int         line;
    std::string header;
    std::string detail;
    std::string token;

    explicit LexerError(const std::string& detail, int line, std::string token = "")
        : std::runtime_error("[Lexer Error] [Line " + std::to_string(line) + "]:\n" + detail),
          line(line),
          header("[Lexer Error] [Line " + std::to_string(line) + "]"),
          detail(detail),
          token(std::move(token)) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    char advance();
    char peek() const;
    char peekNext() const;
    bool isAtEnd() const;
    void skipWhitespace();
    void skipLineComment();
    void skipHashComment();
    void skipBlockComment();

    Token number();       // handles both integers and floats
    Token identifier();
    Token makeToken(TokenType type, const std::string& lexeme) const;

    std::string source;
    std::size_t current;
    int         line;
};

#endif