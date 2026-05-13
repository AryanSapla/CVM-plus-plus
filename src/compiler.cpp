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
    if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(stmt)) {
        for (const auto& statement : blockStmt->statements) {
            if (!statement) {
                throw std::runtime_error("Invalid statement inside block.");
            }
            compileStatement(statement.get());
        }
        return;
    }

    if (const auto* ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        compileExpression(ifStmt->condition.get());
        std::size_t jumpIfFalseIndex = emit(OpCode::JumpIfFalse);

        compileStatement(ifStmt->thenBranch.get());

        if (ifStmt->elseBranch) {
            std::size_t jumpToEndIndex = emit(OpCode::Jump);
            patchOperand(jumpIfFalseIndex, instructions.size());
            compileStatement(ifStmt->elseBranch.get());
            patchOperand(jumpToEndIndex, instructions.size());
        } else {
            patchOperand(jumpIfFalseIndex, instructions.size());
        }
        return;
    }

    if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        std::size_t loopStart = instructions.size();
        compileExpression(whileStmt->condition.get());
        std::size_t jumpIfFalseIndex = emit(OpCode::JumpIfFalse);
        compileStatement(whileStmt->body.get());
        emit(OpCode::Jump, std::to_string(loopStart));
        patchOperand(jumpIfFalseIndex, instructions.size());
        return;
    }

    if (const auto* letStmt = dynamic_cast<const LetStmt*>(stmt)) {
        compileExpression(letStmt->value.get());
        instructions.emplace_back(OpCode::StoreVar, letStmt->name);
        return;
    }

    if (const auto* assignStmt = dynamic_cast<const AssignStmt*>(stmt)) {
        compileExpression(assignStmt->value.get());
        instructions.emplace_back(OpCode::StoreVar, assignStmt->name);
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

    if (const auto* boolExpr = dynamic_cast<const BoolExpr*>(expr)) {
        instructions.emplace_back(OpCode::PushInt, boolExpr->value ? "1" : "0");
        return;
    }

    if (dynamic_cast<const InputExpr*>(expr) != nullptr) {
        instructions.emplace_back(OpCode::Input);
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
        } else if (binaryExpr->op == "==") {
            instructions.emplace_back(OpCode::Equal);
        } else if (binaryExpr->op == "<") {
            instructions.emplace_back(OpCode::Less);
        } else {
            throw std::runtime_error("Unknown binary operator: " + binaryExpr->op);
        }

        return;
    }

    throw std::runtime_error("Unsupported expression type.");
}

std::size_t Compiler::emit(OpCode opcode, const std::string& operand) {
    instructions.emplace_back(opcode, operand);
    return instructions.size() - 1;
}

void Compiler::patchOperand(std::size_t index, std::size_t target) {
    instructions[index].operand = std::to_string(target);
}
