#ifndef LEXER_H
#define LEXER_H

#include <cstddef>
#include <string>
#include <vector>

#include "token.h"

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    char advance();
    char peek() const;
    bool isAtEnd() const;
    void skipWhitespace();

    Token number();
    Token identifier();
    Token makeToken(TokenType type, const std::string& lexeme) const;

    std::string source;
    std::size_t current;
};

#endif
