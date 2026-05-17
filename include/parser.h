#ifndef PARSER_H
#define PARSER_H

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "ast.h"
#include "token.h"

// ─── Parse Error ──────────────────────────────────────────────────────────────
// Thrown by Parser::parse() on any syntax error.
// Format:  [Parse Error] [Line N]:\n<message>
struct ParseError : std::runtime_error {
    int line;
    std::string header;
    std::string detail;
    std::string token;

    explicit ParseError(const std::string& detail, int line, const std::string& token = "")
        : std::runtime_error("[Parse Error] [Line " + std::to_string(line) + "]:\n" + detail),
          line(line),
          header("[Parse Error] [Line " + std::to_string(line) + "]"),
          detail(detail),
          token(token) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    // Returns parsed statements; throws ParseError or SemanticError on failure.
    std::vector<std::unique_ptr<Stmt>> parse();

    const std::string& getErrorMessage() const;

private:
    // ── Statement parsers ────────────────────────────────────────────────────
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> declarationStatement(int stmtLine, ValueType declaredType, const std::string& keywordText);
    std::unique_ptr<Stmt> blockStatement(int stmtLine);
    std::unique_ptr<Stmt> ifStatement(int stmtLine);
    std::unique_ptr<Stmt> whileStatement(int stmtLine);
    std::unique_ptr<Stmt> printStatement(int stmtLine);
    std::unique_ptr<Stmt> assignmentStatement(int stmtLine);
    std::unique_ptr<Stmt> expressionStatement(int stmtLine);

    // ── Expression parsers ───────────────────────────────────────────────────
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> logicalOr();
    std::unique_ptr<Expr> logicalAnd();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> primary();

    // ── Token helpers ────────────────────────────────────────────────────────
    bool         match(TokenType type);
    void         consume(TokenType type, const std::string& message);
    bool         check(TokenType type) const;
    bool         checkNext(TokenType type) const;
    const Token& advance();
    const Token& peek() const;
    const Token& previous() const;
    bool         isAtEnd() const;

    [[noreturn]] void parseError(const std::string& message, int line, const std::string& token = "") const;
    [[noreturn]] void parseError(const std::string& message) const;

    std::string describeToken(const Token& token) const;
    std::string describeCurrentToken() const;

    const std::vector<Token>& tokens;
    std::size_t current;
    std::string errorMessage;
};

#endif
