#include "parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;

    while (!isAtEnd()) {
        statements.push_back(statement());
    }

    return statements;
}

std::unique_ptr<Stmt> Parser::statement() {
    if (match(TokenType::Let)) {
        return letStatement();
    }

    if (match(TokenType::Print)) {
        return printStatement();
    }

    return expressionStatement();
}

std::unique_ptr<Stmt> Parser::letStatement() {
    Token nameToken = advance();

    if (nameToken.type != TokenType::Identifier) {
        return nullptr;
    }

    if (!match(TokenType::Equal)) {
        return nullptr;
    }

    std::unique_ptr<Expr> value = expression();

    if (!match(TokenType::Semicolon)) {
        return nullptr;
    }

    return std::make_unique<LetStmt>(nameToken.lexeme, std::move(value));
}

std::unique_ptr<Stmt> Parser::printStatement() {
    std::unique_ptr<Expr> value = expression();

    if (!match(TokenType::Semicolon)) {
        return nullptr;
    }

    return std::make_unique<PrintStmt>(std::move(value));
}

std::unique_ptr<Stmt> Parser::expressionStatement() {
    std::unique_ptr<Expr> expr = expression();

    if (!match(TokenType::Semicolon)) {
        return nullptr;
    }

    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::expression() {
    std::unique_ptr<Expr> expr = term();

    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        Token op = previous();
        std::unique_ptr<Expr> right = term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    std::unique_ptr<Expr> expr = factor();

    while (match(TokenType::Star) || match(TokenType::Slash)) {
        Token op = previous();
        std::unique_ptr<Expr> right = factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    if (match(TokenType::Number)) {
        return std::make_unique<NumberExpr>(previous().lexeme);
    }

    if (match(TokenType::Identifier)) {
        return std::make_unique<IdentifierExpr>(previous().lexeme);
    }

    if (match(TokenType::LeftParen)) {
        std::unique_ptr<Expr> expr = expression();

        if (!match(TokenType::RightParen)) {
            return nullptr;
        }

        return expr;
    }

    return nullptr;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }

    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) {
        return type == TokenType::EndOfFile;
    }

    return peek().type == type;
}

const Token& Parser::advance() {
    if (!isAtEnd()) {
        current++;
    }

    return previous();
}

const Token& Parser::peek() const {
    return tokens[current];
}

const Token& Parser::previous() const {
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EndOfFile;
}
