#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "opcode.h"

// ─── Expressions ─────────────────────────────────────────────────────────────

struct Expr {
    int line = 0;
    virtual ~Expr() = default;
    virtual std::string toString() const = 0;
};

// Integer literal  (no suffix)
struct NumberExpr : Expr {
    std::string value;  // exact source text

    explicit NumberExpr(std::string value) : value(std::move(value)) {}

    std::string toString() const override { return value; }
};

// Integer literal with LL suffix  →  long long int
struct LongLongExpr : Expr {
    std::string value;  // digits only, no suffix

    explicit LongLongExpr(std::string value) : value(std::move(value)) {}

    std::string toString() const override { return value + "LL"; }
};

// Float literal
struct FloatExpr : Expr {
    std::string value;  // exact source text including '.'

    explicit FloatExpr(std::string value) : value(std::move(value)) {}

    std::string toString() const override { return value; }
};

struct BoolExpr : Expr {
    bool value;

    explicit BoolExpr(bool value) : value(value) {}

    std::string toString() const override { return value ? "true" : "false"; }
};

struct InputExpr : Expr {
    std::string toString() const override { return "input"; }
};

// size(type)  →  bit-width of the named type as an Int
struct SizeOfExpr : Expr {
    ValueType   targetType;
    std::string typeName;   // e.g. "int", "float", "bool", "long long int"
    int         bits;       // 1 / 32 / 64

    SizeOfExpr(ValueType targetType, std::string typeName, int bits)
        : targetType(targetType), typeName(std::move(typeName)), bits(bits) {}

    std::string toString() const override {
        return "size(" + typeName + ")";
    }
};

struct IdentifierExpr : Expr {
    std::string name;

    explicit IdentifierExpr(std::string name) : name(std::move(name)) {}

    std::string toString() const override { return name; }
};

// Explicit C-style cast: (int)expr  /  (float)expr  /  (long long int)expr  /  (bool)expr
struct CastExpr : Expr {
    ValueType   targetType;
    std::string castText;   // e.g. "int", "float", "long long int", "bool"
    std::unique_ptr<Expr> operand;

    CastExpr(ValueType targetType, std::string castText, std::unique_ptr<Expr> operand)
        : targetType(targetType), castText(std::move(castText)), operand(std::move(operand)) {}

    std::string toString() const override {
        return "(" + castText + ")" + operand->toString();
    }
};

struct UnaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> right;

    UnaryExpr(std::string op, std::unique_ptr<Expr> right)
        : op(std::move(op)), right(std::move(right)) {}

    std::string toString() const override {
        return "(" + op + right->toString() + ")";
    }
};

struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left;
    std::string op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> left, std::string op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    std::string toString() const override {
        return "(" + left->toString() + " " + op + " " + right->toString() + ")";
    }
};

// ─── Statements ──────────────────────────────────────────────────────────────

struct Stmt {
    int line = 0;
    virtual ~Stmt() = default;
    virtual std::string toString() const = 0;
};

struct LetStmt : Stmt {
    ValueType   declaredType;
    std::string keywordText;
    std::string name;
    std::unique_ptr<Expr> value;

    LetStmt(ValueType declaredType,
            std::string keywordText,
            std::string name,
            std::unique_ptr<Expr> value)
        : declaredType(declaredType),
          keywordText(std::move(keywordText)),
          name(std::move(name)),
          value(std::move(value)) {}

    std::string toString() const override {
        return keywordText + " " + name + " = " + value->toString();
    }
};

struct PrintStmt : Stmt {
    std::unique_ptr<Expr> value;

    explicit PrintStmt(std::unique_ptr<Expr> value) : value(std::move(value)) {}

    std::string toString() const override { return "print " + value->toString(); }
};

struct AssignStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> value;

    AssignStmt(std::string name, std::unique_ptr<Expr> value)
        : name(std::move(name)), value(std::move(value)) {}

    std::string toString() const override { return name + " = " + value->toString(); }
};

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements)
        : statements(std::move(statements)) {}

    std::string toString() const override { return "{ block }"; }
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;

    IfStmt(std::unique_ptr<Expr> condition,
           std::unique_ptr<Stmt> thenBranch,
           std::unique_ptr<Stmt> elseBranch)
        : condition(std::move(condition)),
          thenBranch(std::move(thenBranch)),
          elseBranch(std::move(elseBranch)) {}

    std::string toString() const override {
        return elseBranch
            ? "if (" + condition->toString() + ") ... else ..."
            : "if (" + condition->toString() + ") ...";
    }
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
        : condition(std::move(condition)), body(std::move(body)) {}

    std::string toString() const override {
        return "while (" + condition->toString() + ") ...";
    }
};

// for (init; condition; update) body
// init  : LetStmt (declaration) or AssignStmt, or nullptr (omitted)
// update: AssignStmt, or nullptr (omitted)
struct ForStmt : Stmt {
    std::unique_ptr<Stmt> init;       // optional
    std::unique_ptr<Expr> condition;  // optional (omitted = infinite)
    std::unique_ptr<Stmt> update;     // optional (AssignStmt without trailing ';' in source)
    std::unique_ptr<Stmt> body;

    ForStmt(std::unique_ptr<Stmt> init,
            std::unique_ptr<Expr> condition,
            std::unique_ptr<Stmt> update,
            std::unique_ptr<Stmt> body)
        : init(std::move(init)),
          condition(std::move(condition)),
          update(std::move(update)),
          body(std::move(body)) {}

    std::string toString() const override { return "for (...) ..."; }
};

struct BreakStmt : Stmt {
    std::string toString() const override { return "break"; }
};

struct ContinueStmt : Stmt {
    std::string toString() const override { return "continue"; }
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expression;

    explicit ExprStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}

    std::string toString() const override { return expression->toString(); }
};

#endif