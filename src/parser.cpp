#include "parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}

// ─── Public ──────────────────────────────────────────────────────────────────

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    errorMessage.clear();
    std::vector<std::unique_ptr<Stmt>> statements;

    while (!isAtEnd()) {
        statements.push_back(statement());
    }

    // Semantic analysis pass — after full parse succeeds
    semanticCheck(statements);

    return statements;
}

const std::string& Parser::getErrorMessage() const {
    return errorMessage;
}

// ─── Statements ──────────────────────────────────────────────────────────────

std::unique_ptr<Stmt> Parser::statement() {
    int stmtLine = peek().line;

    if (match(TokenType::If))        return ifStatement(stmtLine);
    if (match(TokenType::While))     return whileStatement(stmtLine);
    if (match(TokenType::LeftBrace)) return blockStatement(stmtLine);
    if (match(TokenType::Let))       return letStatement(stmtLine);
    if (match(TokenType::Print))     return printStatement(stmtLine);

    if (check(TokenType::Identifier) && checkNext(TokenType::Equal))
        return assignmentStatement(stmtLine);

    // Bare identifier not followed by '=' — unknown keyword/identifier used as statement.
    // Raise as SemanticError directly (better category than ParseError).
    if (check(TokenType::Identifier)) {
        std::string name = peek().lexeme;
        advance(); // consume it so we can report properly
        throw SemanticError(
            "Unknown keyword or identifier '" + name + "'",
            stmtLine, name);
    }

    return expressionStatement(stmtLine);
}

std::unique_ptr<Stmt> Parser::blockStatement(int stmtLine) {
    std::vector<std::unique_ptr<Stmt>> statements;

    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        statements.push_back(statement());
    }

    if (isAtEnd()) {
        parseError("Expected '}' before end of file", stmtLine);
    }

    consume(TokenType::RightBrace, "Expected '}' after block");

    auto node = std::make_unique<BlockStmt>(std::move(statements));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::ifStatement(int stmtLine) {
    consume(TokenType::LeftParen, "Expected '(' after 'if'");

    if (check(TokenType::RightParen))
        parseError("Expected expression inside condition", stmtLine);

    std::unique_ptr<Expr> condition = expression();
    consume(TokenType::RightParen, "Expected ')' after if condition");

    std::unique_ptr<Stmt> thenBranch = statement();

    std::unique_ptr<Stmt> elseBranch;
    if (match(TokenType::Else))
        elseBranch = statement();

    auto node = std::make_unique<IfStmt>(
        std::move(condition), std::move(thenBranch), std::move(elseBranch));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::whileStatement(int stmtLine) {
    consume(TokenType::LeftParen, "Expected '(' after 'while'");

    if (check(TokenType::RightParen))
        parseError("Expected expression inside condition", stmtLine);

    std::unique_ptr<Expr> condition = expression();
    consume(TokenType::RightParen, "Expected ')' after while condition");

    std::unique_ptr<Stmt> body = statement();

    auto node = std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::letStatement(int stmtLine) {
    consume(TokenType::Identifier, "Expected variable name after 'let'");
    Token nameToken = previous();

    consume(TokenType::Equal, "Expected '=' after variable name");

    std::unique_ptr<Expr> value = expression();

    if (!check(TokenType::Semicolon)) {
        std::string found = "Found " + describeCurrentToken();
        parseError("Expected ';' after variable declaration. " + found, stmtLine);
    }
    consume(TokenType::Semicolon, "Expected ';' after variable declaration");

    auto node = std::make_unique<LetStmt>(nameToken.lexeme, std::move(value));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::printStatement(int stmtLine) {
    if (check(TokenType::Semicolon) || isAtEnd())
        parseError("Expected expression after 'print'", stmtLine);

    if (check(TokenType::Identifier) && checkNext(TokenType::Equal))
        parseError("Invalid assignment inside print statement", stmtLine);

    std::unique_ptr<Expr> value = expression();
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
    consume(TokenType::Semicolon, "Expected ';' after expression");

    auto node = std::make_unique<ExprStmt>(std::move(expr));
    node->line = stmtLine;
    return node;
}

// ─── Expressions ─────────────────────────────────────────────────────────────

std::unique_ptr<Expr> Parser::expression() {
    return logicalOr();
}

std::unique_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();
    // 'or' is represented as an identifier token in the lexer
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
    // 'not' keyword
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
        parseError("Invalid assignment target. Left side of '=' must be a variable name");
    }

    parseError("Unexpected token " + describeCurrentToken() + " in expression");
}

// ─── Semantic Analysis ────────────────────────────────────────────────────────

void Parser::semanticCheck(const std::vector<std::unique_ptr<Stmt>>& stmts) {
    std::unordered_set<std::string> declared;
    std::vector<std::string> declOrder;
    for (const auto& stmt : stmts)
        checkStmt(stmt.get(), declared, declOrder);
}

void Parser::checkStmt(const Stmt* stmt,
                       std::unordered_set<std::string>& declared,
                       std::vector<std::string>& declOrder) {
    if (!stmt) return;

    if (const auto* let = dynamic_cast<const LetStmt*>(stmt)) {
        checkExpr(let->value.get(), declared);
        if (declared.count(let->name)) {
            throw SemanticError(
                "Variable '" + let->name + "' already declared",
                let->line, let->name);
        }
        declared.insert(let->name);
        declOrder.push_back(let->name);
        return;
    }

    if (const auto* assign = dynamic_cast<const AssignStmt*>(stmt)) {
        checkExpr(assign->value.get(), declared);
        // Assignment to undeclared variable is a semantic error
        if (!declared.count(assign->name)) {
            throw SemanticError(
                "Undefined variable '" + assign->name + "'",
                assign->line, assign->name);
        }
        return;
    }

    if (const auto* print = dynamic_cast<const PrintStmt*>(stmt)) {
        checkExpr(print->value.get(), declared);
        return;
    }

    if (const auto* exprS = dynamic_cast<const ExprStmt*>(stmt)) {
        checkExpr(exprS->expression.get(), declared);
        return;
    }

    if (const auto* block = dynamic_cast<const BlockStmt*>(stmt)) {
        // Blocks share outer scope (no new scope for simplicity)
        for (const auto& s : block->statements)
            checkStmt(s.get(), declared, declOrder);
        return;
    }

    if (const auto* ifS = dynamic_cast<const IfStmt*>(stmt)) {
        checkExpr(ifS->condition.get(), declared);
        checkStmt(ifS->thenBranch.get(), declared, declOrder);
        if (ifS->elseBranch)
            checkStmt(ifS->elseBranch.get(), declared, declOrder);
        return;
    }

    if (const auto* whileS = dynamic_cast<const WhileStmt*>(stmt)) {
        checkExpr(whileS->condition.get(), declared);
        checkStmt(whileS->body.get(), declared, declOrder);
        return;
    }
}

void Parser::checkExpr(const Expr* expr,
                       const std::unordered_set<std::string>& declared) {
    if (!expr) return;

    if (const auto* id = dynamic_cast<const IdentifierExpr*>(expr)) {
        if (!declared.count(id->name)) {
            throw SemanticError(
                "Undefined variable '" + id->name + "'",
                id->line, id->name);
        }
        return;
    }

    if (const auto* bin = dynamic_cast<const BinaryExpr*>(expr)) {
        checkExpr(bin->left.get(), declared);
        checkExpr(bin->right.get(), declared);
        return;
    }

    if (const auto* un = dynamic_cast<const UnaryExpr*>(expr)) {
        checkExpr(un->right.get(), declared);
        return;
    }

    // NumberExpr, BoolExpr, InputExpr — no checks needed
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

void Parser::consume(TokenType type, const std::string& message) {
    if (match(type)) return;
    std::string found = "Found " + describeCurrentToken();
    parseError(message + ". " + found);
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return type == TokenType::EndOfFile;
    return peek().type == type;
}

bool Parser::checkNext(TokenType type) const {
    if (current + 1 >= tokens.size()) return false;
    return tokens[current + 1].type == type;
}

const Token& Parser::advance() {
    if (!isAtEnd()) current++;
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

void Parser::parseError(const std::string& message, int line) const {
    const_cast<Parser*>(this)->errorMessage =
        "[Parse Error] [Line " + std::to_string(line) + "]:\n" + message;
    throw ParseError(message, line);
}

void Parser::parseError(const std::string& message) const {
    parseError(message, peek().line);
}

std::string Parser::describeToken(const Token& token) const {
    if (token.type == TokenType::EndOfFile) return "end of file";
    if (token.lexeme.empty()) return std::string("token ") + tokenTypeToString(token.type);
    return "'" + token.lexeme + "'";
}

std::string Parser::describeCurrentToken() const {
    return describeToken(peek());
}