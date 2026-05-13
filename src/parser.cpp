#include "parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;

    while (!isAtEnd()) {
        std::unique_ptr<Stmt> stmt = statement();
        if (!stmt) {
            statements.push_back(nullptr);
            break;
        }
        statements.push_back(std::move(stmt));
    }

    return statements;
}

std::unique_ptr<Stmt> Parser::statement() {
    if (match(TokenType::If)) {
        return ifStatement();
    }

    if (match(TokenType::While)) {
        return whileStatement();
    }

    if (match(TokenType::LeftBrace)) {
        return blockStatement();
    }

    if (match(TokenType::Let)) {
        return letStatement();
    }

    if (match(TokenType::Print)) {
        return printStatement();
    }

    if (check(TokenType::Identifier) && checkNext(TokenType::Equal)) {
        return assignmentStatement();
    }

    return expressionStatement();
}

std::unique_ptr<Stmt> Parser::blockStatement() {
    std::vector<std::unique_ptr<Stmt>> statements;

    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        std::unique_ptr<Stmt> stmt = statement();
        if (!stmt) {
            return nullptr;
        }
        statements.push_back(std::move(stmt));
    }

    if (!match(TokenType::RightBrace)) {
        return nullptr;
    }

    return std::make_unique<BlockStmt>(std::move(statements));
}

std::unique_ptr<Stmt> Parser::ifStatement() {
    if (!match(TokenType::LeftParen)) {
        return nullptr;
    }

    std::unique_ptr<Expr> condition = expression();

    if (!condition || !match(TokenType::RightParen)) {
        return nullptr;
    }

    std::unique_ptr<Stmt> thenBranch = statement();
    if (!thenBranch) {
        return nullptr;
    }

    std::unique_ptr<Stmt> elseBranch;
    if (match(TokenType::Else)) {
        elseBranch = statement();
        if (!elseBranch) {
            return nullptr;
        }
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::whileStatement() {
    if (!match(TokenType::LeftParen)) {
        return nullptr;
    }

    std::unique_ptr<Expr> condition = expression();

    if (!condition || !match(TokenType::RightParen)) {
        return nullptr;
    }

    std::unique_ptr<Stmt> body = statement();
    if (!body) {
        return nullptr;
    }

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
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

    if (!value || !match(TokenType::Semicolon)) {
        return nullptr;
    }

    return std::make_unique<LetStmt>(nameToken.lexeme, std::move(value));
}

std::unique_ptr<Stmt> Parser::printStatement() {
    std::unique_ptr<Expr> value = expression();

    if (!value || !match(TokenType::Semicolon)) {
        return nullptr;
    }

    return std::make_unique<PrintStmt>(std::move(value));
}

std::unique_ptr<Stmt> Parser::assignmentStatement() {
    Token nameToken = advance();

    if (!match(TokenType::Equal)) {
        return nullptr;
    }

    std::unique_ptr<Expr> value = expression();

    if (!value || !match(TokenType::Semicolon)) {
        return nullptr;
    }

    return std::make_unique<AssignStmt>(nameToken.lexeme, std::move(value));
}

std::unique_ptr<Stmt> Parser::expressionStatement() {
    std::unique_ptr<Expr> expr = expression();

    if (!expr || !match(TokenType::Semicolon)) {
        return nullptr;
    }

    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::expression() {
    return equality();
}

std::unique_ptr<Expr> Parser::equality() {
    std::unique_ptr<Expr> expr = comparison();

    while (match(TokenType::EqualEqual)) {
        Token op = previous();
        std::unique_ptr<Expr> right = comparison();
        if (!expr || !right) {
            return nullptr;
        }
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    std::unique_ptr<Expr> expr = term();

    while (match(TokenType::Less)) {
        Token op = previous();
        std::unique_ptr<Expr> right = term();
        if (!expr || !right) {
            return nullptr;
        }
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    std::unique_ptr<Expr> expr = factor();

    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        Token op = previous();
        std::unique_ptr<Expr> right = factor();
        if (!expr || !right) {
            return nullptr;
        }
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    std::unique_ptr<Expr> expr = unary();

    while (match(TokenType::Star) || match(TokenType::Slash)) {
        Token op = previous();
        std::unique_ptr<Expr> right = unary();
        if (!expr || !right) {
            return nullptr;
        }
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match(TokenType::Minus)) {
        Token op = previous();
        std::unique_ptr<Expr> right = unary();
        if (!right) {
            return nullptr;
        }
        return std::make_unique<UnaryExpr>(op.lexeme, std::move(right));
    }

    return primary();
}

std::unique_ptr<Expr> Parser::primary() {
    if (match(TokenType::Number)) {
        return std::make_unique<NumberExpr>(previous().lexeme);
    }

    if (match(TokenType::True)) {
        return std::make_unique<BoolExpr>(true);
    }

    if (match(TokenType::False)) {
        return std::make_unique<BoolExpr>(false);
    }

    if (match(TokenType::Input)) {
        return std::make_unique<InputExpr>();
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

bool Parser::checkNext(TokenType type) const {
    if (current + 1 >= tokens.size()) {
        return false;
    }

    return tokens[current + 1].type == type;
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
