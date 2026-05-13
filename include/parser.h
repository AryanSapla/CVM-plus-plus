#ifndef PARSER_H
#define PARSER_H

#include <cstddef>
#include <memory>
#include <vector>

#include "ast.h"
#include "token.h"

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::vector<std::unique_ptr<Stmt>> parse();

private:
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> letStatement();
    std::unique_ptr<Stmt> printStatement();
    std::unique_ptr<Stmt> expressionStatement();

    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();

    bool match(TokenType type);
    bool check(TokenType type) const;
    const Token& advance();
    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;

    const std::vector<Token>& tokens;
    std::size_t current;
};

#endif
