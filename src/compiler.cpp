#include "compiler.h"

#include <stdexcept>

std::vector<Instruction> Compiler::compile(const std::vector<std::unique_ptr<Stmt>>& statements) {
    instructions.clear();

    for (const auto& statement : statements) {
        if (statement) {
            compileStatement(statement.get());
        }
    }

    instructions.emplace_back(OpCode::Halt);
    return instructions;
}

void Compiler::compileStatement(const Stmt* stmt) {
    if (const auto* letStmt = dynamic_cast<const LetStmt*>(stmt)) {
        compileExpression(letStmt->value.get());
        instructions.emplace_back(OpCode::StoreVar, letStmt->name);
        return;
    }

    if (const auto* printStmt = dynamic_cast<const PrintStmt*>(stmt)) {
        compileExpression(printStmt->value.get());
        instructions.emplace_back(OpCode::Print);
        return;
    }

    if (const auto* exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        compileExpression(exprStmt->expression.get());
        instructions.emplace_back(OpCode::Pop);
        return;
    }

    throw std::runtime_error("Unsupported statement type.");
}

void Compiler::compileExpression(const Expr* expr) {
    if (const auto* numberExpr = dynamic_cast<const NumberExpr*>(expr)) {
        instructions.emplace_back(OpCode::PushInt, numberExpr->value);
        return;
    }

    if (const auto* identifierExpr = dynamic_cast<const IdentifierExpr*>(expr)) {
        instructions.emplace_back(OpCode::LoadVar, identifierExpr->name);
        return;
    }

    if (const auto* binaryExpr = dynamic_cast<const BinaryExpr*>(expr)) {
        compileExpression(binaryExpr->left.get());
        compileExpression(binaryExpr->right.get());

        if (binaryExpr->op == "+") {
            instructions.emplace_back(OpCode::Add);
        } else if (binaryExpr->op == "-") {
            instructions.emplace_back(OpCode::Subtract);
        } else if (binaryExpr->op == "*") {
            instructions.emplace_back(OpCode::Multiply);
        } else if (binaryExpr->op == "/") {
            instructions.emplace_back(OpCode::Divide);
        } else {
            throw std::runtime_error("Unknown binary operator: " + binaryExpr->op);
        }

        return;
    }

    throw std::runtime_error("Unsupported expression type.");
}
