#include "parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    errorMessage.clear();
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

const std::string& Parser::getErrorMessage() const {
    return errorMessage;
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

    if (!consume(TokenType::RightBrace, "Expected '}' after block.")) {
        return nullptr;
    }

    return std::make_unique<BlockStmt>(std::move(statements));
}

std::unique_ptr<Stmt> Parser::ifStatement() {
    if (!consume(TokenType::LeftParen, "Expected '(' after 'if'.")) {
        return nullptr;
    }

    std::unique_ptr<Expr> condition = expression();

    if (!condition || !consume(TokenType::RightParen, "Expected ')' after if condition.")) {
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
    if (!consume(TokenType::LeftParen, "Expected '(' after 'while'.")) {
        return nullptr;
    }

    std::unique_ptr<Expr> condition = expression();

    if (!condition || !consume(TokenType::RightParen, "Expected ')' after while condition.")) {
        return nullptr;
    }

    std::unique_ptr<Stmt> body = statement();
    if (!body) {
        return nullptr;
    }

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::letStatement() {
    if (!consume(TokenType::Identifier, "Expected variable name after 'let'.")) {
        return nullptr;
    }
    Token nameToken = previous();

    if (!consume(TokenType::Equal, "Expected '=' after variable name.")) {
        return nullptr;
    }

    std::unique_ptr<Expr> value = expression();

    if (!value || !consume(TokenType::Semicolon, "Expected ';' after variable declaration.")) {
        return nullptr;
    }

    return std::make_unique<LetStmt>(nameToken.lexeme, std::move(value));
}

std::unique_ptr<Stmt> Parser::printStatement() {
    std::unique_ptr<Expr> value = expression();

    if (!value || !consume(TokenType::Semicolon, "Expected ';' after print value.")) {
        return nullptr;
    }

    return std::make_unique<PrintStmt>(std::move(value));
}

std::unique_ptr<Stmt> Parser::assignmentStatement() {
    if (!consume(TokenType::Identifier, "Expected variable name in assignment.")) {
        return nullptr;
    }
    Token nameToken = previous();

    if (!consume(TokenType::Equal, "Expected '=' in assignment.")) {
        return nullptr;
    }

    std::unique_ptr<Expr> value = expression();

    if (!value || !consume(TokenType::Semicolon, "Expected ';' after assignment.")) {
        return nullptr;
    }

    return std::make_unique<AssignStmt>(nameToken.lexeme, std::move(value));
}

std::unique_ptr<Stmt> Parser::expressionStatement() {
    std::unique_ptr<Expr> expr = expression();

    if (!expr || !consume(TokenType::Semicolon, "Expected ';' after expression.")) {
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

        if (!consume(TokenType::RightParen, "Expected ')' after expression.")) {
            return nullptr;
        }

        return expr;
    }

    setError("Unexpected token " + describeCurrentToken() + " in expression.");
    return nullptr;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }

    return false;
}

bool Parser::consume(TokenType type, const std::string& message) {
    if (match(type)) {
        return true;
    }

    setError(message + " Found " + describeCurrentToken() + ".");
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

void Parser::setError(const std::string& message) {
    if (errorMessage.empty()) {
        errorMessage = message;
    }
}

std::string Parser::describeToken(const Token& token) const {
    if (token.type == TokenType::EndOfFile) {
        return "end of file";
    }

    if (token.lexeme.empty()) {
        return std::string("token ") + tokenTypeToString(token.type);
    }

    return "'" + token.lexeme + "'";
}

std::string Parser::describeCurrentToken() const {
    return describeToken(peek());
}
