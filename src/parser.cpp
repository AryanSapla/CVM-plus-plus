#include "parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    errorMessage.clear();
    std::vector<std::unique_ptr<Stmt>> statements;

    while (!isAtEnd()) {
        statements.push_back(statement());
    }

    return statements;
}

const std::string& Parser::getErrorMessage() const {
    return errorMessage;
}

std::unique_ptr<Stmt> Parser::statement() {
    int stmtLine = peek().line;

    if (match(TokenType::If)) {
        return ifStatement(stmtLine);
    }

    if (match(TokenType::While)) {
        return whileStatement(stmtLine);
    }

    if (match(TokenType::LeftBrace)) {
        return blockStatement(stmtLine);
    }

    if (match(TokenType::Let)) {
        return declarationStatement(stmtLine, ValueType::Int, "let");
    }

    if (match(TokenType::IntKeyword)) {
        return declarationStatement(stmtLine, ValueType::Int, "int");
    }

    if (match(TokenType::LongKeyword)) {
        std::string keywordText = "long";
        if (match(TokenType::IntKeyword)) {
            keywordText = "long int";
        }
        return declarationStatement(stmtLine, ValueType::Long, keywordText);
    }

    if (match(TokenType::Print)) {
        return printStatement(stmtLine);
    }

    if (check(TokenType::Identifier) && checkNext(TokenType::Equal)) {
        return assignmentStatement(stmtLine);
    }

    if (check(TokenType::Identifier)) {
        parseError("Unexpected identifier '" + peek().lexeme + "'", peek().line, peek().lexeme);
    }

    return expressionStatement(stmtLine);
}

std::unique_ptr<Stmt> Parser::declarationStatement(int stmtLine,
                                                   ValueType declaredType,
                                                   const std::string& keywordText) {
    consume(TokenType::Identifier, "Expected variable name after '" + keywordText + "'");
    Token nameToken = previous();

    consume(TokenType::Equal, "Expected '=' after variable name");

    std::unique_ptr<Expr> value = expression();

    if (!check(TokenType::Semicolon)) {
        parseError("Expected ';' after variable declaration. Found " + describeCurrentToken(),
                   peek().line,
                   peek().lexeme);
    }
    consume(TokenType::Semicolon, "Expected ';' after variable declaration");

    auto node = std::make_unique<LetStmt>(declaredType, keywordText, nameToken.lexeme, std::move(value));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::blockStatement(int stmtLine) {
    std::vector<std::unique_ptr<Stmt>> statements;

    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        statements.push_back(statement());
    }

    if (isAtEnd()) {
        parseError("Expected '}'", previous().line, previous().lexeme);
    }

    consume(TokenType::RightBrace, "Expected '}' after block");

    auto node = std::make_unique<BlockStmt>(std::move(statements));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::ifStatement(int stmtLine) {
    consume(TokenType::LeftParen, "Expected '(' after 'if'");

    if (check(TokenType::RightParen)) {
        parseError("Expected expression inside condition", peek().line, peek().lexeme);
    }

    std::unique_ptr<Expr> condition = expression();
    consume(TokenType::RightParen, "Expected ')' after if condition");

    std::unique_ptr<Stmt> thenBranch = statement();

    std::unique_ptr<Stmt> elseBranch;
    if (match(TokenType::Else)) {
        elseBranch = statement();
    }

    auto node = std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::whileStatement(int stmtLine) {
    consume(TokenType::LeftParen, "Expected '(' after 'while'");

    if (check(TokenType::RightParen)) {
        parseError("Expected expression inside condition", peek().line, peek().lexeme);
    }

    std::unique_ptr<Expr> condition = expression();
    consume(TokenType::RightParen, "Expected ')' after while condition");

    std::unique_ptr<Stmt> body = statement();

    auto node = std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::printStatement(int stmtLine) {
    if (check(TokenType::Semicolon) || isAtEnd()) {
        parseError("Expected expression after 'print'", peek().line, peek().lexeme);
    }

    if (check(TokenType::Identifier) && checkNext(TokenType::Equal)) {
        parseError("Invalid assignment inside print statement", peek().line, peek().lexeme);
    }

    std::unique_ptr<Expr> value = expression();

    if (check(TokenType::Equal)) {
        std::string underline = value ? value->toString() : peek().lexeme;
        parseError("Invalid assignment inside print statement", peek().line, underline);
    }

    consume(TokenType::Semicolon, "Expected ';' after print value");

    auto node = std::make_unique<PrintStmt>(std::move(value));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::assignmentStatement(int stmtLine) {
    consume(TokenType::Identifier, "Expected variable name in assignment");
    Token nameToken = previous();

    consume(TokenType::Equal, "Expected '=' in assignment");

    std::unique_ptr<Expr> value = expression();
    consume(TokenType::Semicolon, "Expected ';' after assignment");

    auto node = std::make_unique<AssignStmt>(nameToken.lexeme, std::move(value));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::expressionStatement(int stmtLine) {
    std::unique_ptr<Expr> expr = expression();

    if (check(TokenType::Equal)) {
        std::string underline = expr ? expr->toString() : previous().lexeme;
        parseError("Invalid assignment target", peek().line, underline);
    }

    consume(TokenType::Semicolon, "Expected ';' after expression");

    auto node = std::make_unique<ExprStmt>(std::move(expr));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Expr> Parser::expression() {
    return logicalOr();
}

std::unique_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();

    while (check(TokenType::Identifier) && peek().lexeme == "or") {
        advance();
        Token op = previous();
        auto right = logicalAnd();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), "or", std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::logicalAnd() {
    auto expr = equality();

    while (check(TokenType::Identifier) && peek().lexeme == "and") {
        advance();
        Token op = previous();
        auto right = equality();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), "and", std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    auto expr = comparison();

    while (match(TokenType::EqualEqual) || match(TokenType::NotEqual)) {
        Token op = previous();
        auto right = comparison();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    auto expr = term();

    while (match(TokenType::Less) || match(TokenType::LessEqual) ||
           match(TokenType::Greater) || match(TokenType::GreaterEqual)) {
        Token op = previous();
        auto right = term();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    auto expr = factor();

    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        Token op = previous();
        auto right = factor();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    auto expr = unary();

    while (match(TokenType::Star) || match(TokenType::Slash)) {
        Token op = previous();
        auto right = unary();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match(TokenType::Minus)) {
        Token op = previous();
        auto right = unary();
        auto node = std::make_unique<UnaryExpr>(op.lexeme, std::move(right));
        node->line = op.line;
        return node;
    }

    if (check(TokenType::Identifier) && peek().lexeme == "not") {
        Token op = advance();
        auto right = unary();
        auto node = std::make_unique<UnaryExpr>("not", std::move(right));
        node->line = op.line;
        return node;
    }

    return primary();
}

std::unique_ptr<Expr> Parser::primary() {
    if (match(TokenType::Number)) {
        auto node = std::make_unique<NumberExpr>(previous().lexeme);
        node->line = previous().line;
        return node;
    }

    if (match(TokenType::True)) {
        auto node = std::make_unique<BoolExpr>(true);
        node->line = previous().line;
        return node;
    }

    if (match(TokenType::False)) {
        auto node = std::make_unique<BoolExpr>(false);
        node->line = previous().line;
        return node;
    }

    if (match(TokenType::Input)) {
        auto node = std::make_unique<InputExpr>();
        node->line = previous().line;
        return node;
    }

    if (match(TokenType::Identifier)) {
        auto node = std::make_unique<IdentifierExpr>(previous().lexeme);
        node->line = previous().line;
        return node;
    }

    if (match(TokenType::LeftParen)) {
        auto expr = expression();
        consume(TokenType::RightParen, "Expected ')' after expression");
        return expr;
    }

    if (check(TokenType::Equal)) {
        parseError("Invalid assignment target", peek().line, peek().lexeme);
    }

    parseError("Unexpected token " + describeCurrentToken() + " in expression", peek().line, peek().lexeme);
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }

    return false;
}

void Parser::consume(TokenType type, const std::string& message) {
    if (match(type)) {
        return;
    }

    int errorLine = isAtEnd() ? previous().line : peek().line;
    std::string token = isAtEnd() ? previous().lexeme : peek().lexeme;
    parseError(message + ". Found " + describeCurrentToken(), errorLine, token);
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

void Parser::parseError(const std::string& message, int line, const std::string& token) const {
    const_cast<Parser*>(this)->errorMessage =
        "[Parse Error] [Line " + std::to_string(line) + "]:\n" + message;
    throw ParseError(message, line, token);
}

void Parser::parseError(const std::string& message) const {
    int errorLine = isAtEnd() ? previous().line : peek().line;
    std::string token = isAtEnd() ? previous().lexeme : peek().lexeme;
    parseError(message, errorLine, token);
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
