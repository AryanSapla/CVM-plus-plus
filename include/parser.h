#ifndef PARSER_H
#define PARSER_H

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ast.h"
#include "token.h"

// ─── Parse Error ─────────────────────────────────────────────────────────────
struct ParseError : std::runtime_error {
    int         line;
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

// ─── Semantic Error ───────────────────────────────────────────────────────────
struct SemanticError : std::runtime_error {
    int         line;
    std::string header;
    std::string detail;
    std::string token;

    explicit SemanticError(const std::string& detail, int line, const std::string& token = "")
        : std::runtime_error("[Semantic Error] [Line " + std::to_string(line) + "]:\n" + detail),
          line(line),
          header("[Semantic Error] [Line " + std::to_string(line) + "]"),
          detail(detail),
          token(token) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::vector<std::unique_ptr<Stmt>> parse();
    const std::string& getErrorMessage() const;

private:
    // ── Statement parsers ────────────────────────────────────────────────────
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> declarationStatement(int stmtLine,
                                               ValueType declaredType,
                                               const std::string& keywordText);
    std::unique_ptr<Stmt> blockStatement(int stmtLine);
    std::unique_ptr<Stmt> ifStatement(int stmtLine);
    std::unique_ptr<Stmt> whileStatement(int stmtLine);
    std::unique_ptr<Stmt> forStatement(int stmtLine);
    std::unique_ptr<Stmt> breakStatement(int stmtLine);
    std::unique_ptr<Stmt> continueStatement(int stmtLine);
    std::unique_ptr<Stmt> printStatement(int stmtLine);
    std::unique_ptr<Stmt> assignmentStatement(int stmtLine);
    std::unique_ptr<Stmt> expressionStatement(int stmtLine);

    // ── Expression parsers  (precedence low → high) ──────────────────────────
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> logicalOr();
    std::unique_ptr<Expr> logicalAnd();
    std::unique_ptr<Expr> bitwiseOr();
    std::unique_ptr<Expr> bitwiseXor();
    std::unique_ptr<Expr> bitwiseAnd();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> shift();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> power();
    std::unique_ptr<Expr> primary();

    // ── Semantic analysis ────────────────────────────────────────────────────
    // A scope stack: each element is the set of names declared in that scope.
    // Innermost (current) scope is at the back.
    using ScopeLevel = std::unordered_set<std::string>;
    using ScopeStack = std::vector<ScopeLevel>;

    // Helpers on ScopeStack
    bool        isDeclared(const ScopeStack& scopes, const std::string& name) const;
    bool        isDeclaredInCurrentScope(const ScopeStack& scopes, const std::string& name) const;
    void        declare(ScopeStack& scopes, const std::string& name, int line) const;
    ScopeStack  pushScope(ScopeStack scopes) const;   // returns new stack with empty scope pushed
    ScopeStack  popScope(ScopeStack scopes) const;    // pops innermost scope, returns remainder

    void        runSemanticChecks(const std::vector<std::unique_ptr<Stmt>>& statements) const;
    ScopeStack  checkStatements(const std::vector<std::unique_ptr<Stmt>>& statements,
                                ScopeStack scopes) const;
    ScopeStack  checkStatement(const Stmt* stmt, ScopeStack scopes) const;
    void        checkExpression(const Expr* expr, const ScopeStack& scopes) const;
    // Returns names visible in BOTH branches (for if-without-else / partial declaration)
    ScopeStack  intersectScopes(const ScopeStack& base,
                                const ScopeStack& left,
                                const ScopeStack& right) const;

    // ── Token helpers ────────────────────────────────────────────────────────
    bool         match(TokenType type);
    void         consume(TokenType type, const std::string& message);
    bool         check(TokenType type) const;
    bool         checkNext(TokenType type) const;
    const Token& advance();
    const Token& peek() const;
    const Token& previous() const;
    bool         isAtEnd() const;

    // ── Helper: try to parse a cast type name starting at current position ───
    // Returns {ValueType, display-text} or {Unknown,""} if not a cast.
    struct CastType { ValueType type; std::string text; };
    CastType tryCastType();

    [[noreturn]] void parseError(const std::string& message, int line,
                                 const std::string& token = "") const;
    [[noreturn]] void parseError(const std::string& message) const;
    [[noreturn]] void semanticError(const std::string& message, int line,
                                    const std::string& token = "") const;

    std::string describeToken(const Token& token) const;
    std::string describeCurrentToken() const;

    const std::vector<Token>& tokens;
    std::size_t current;
    std::string errorMessage;
    int loopDepth = 0;  // incremented inside while/for, used to validate break/continue
};

#endif