#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

struct Expr {
    virtual ~Expr() = default;
    virtual std::string toString() const = 0;
};

struct NumberExpr : Expr {
    std::string value;

    explicit NumberExpr(std::string value) : value(std::move(value)) {}

    std::string toString() const override {
        return value;
    }
};

struct BoolExpr : Expr {
    bool value;

    explicit BoolExpr(bool value) : value(value) {}

    std::string toString() const override {
        return value ? "true" : "false";
    }
};

struct IdentifierExpr : Expr {
    std::string name;

    explicit IdentifierExpr(std::string name) : name(std::move(name)) {}

    std::string toString() const override {
        return name;
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

struct Stmt {
    virtual ~Stmt() = default;
    virtual std::string toString() const = 0;
};

struct LetStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> value;

    LetStmt(std::string name, std::unique_ptr<Expr> value)
        : name(std::move(name)), value(std::move(value)) {}

    std::string toString() const override {
        return "let " + name + " = " + value->toString();
    }
};

struct PrintStmt : Stmt {
    std::unique_ptr<Expr> value;

    explicit PrintStmt(std::unique_ptr<Expr> value)
        : value(std::move(value)) {}

    std::string toString() const override {
        return "print " + value->toString();
    }
};

struct AssignStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> value;

    AssignStmt(std::string name, std::unique_ptr<Expr> value)
        : name(std::move(name)), value(std::move(value)) {}

    std::string toString() const override {
        return name + " = " + value->toString();
    }
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expression;

    explicit ExprStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}

    std::string toString() const override {
        return expression->toString();
    }
};

#endif
