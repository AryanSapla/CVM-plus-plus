#include "compiler.h"

#include <stdexcept>

std::vector<Instruction> Compiler::compile(const std::vector<std::unique_ptr<Stmt>>& statements) {
    instructions.clear();

    for (const auto& statement : statements) {
        if (statement) {
            compileStatement(statement.get());
        }
    }

    emit(OpCode::Halt);
    return instructions;
}

void Compiler::compileStatement(const Stmt* stmt) {
    if (stmt->line > 0) currentLine = stmt->line;

    if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(stmt)) {
        for (const auto& s : blockStmt->statements) {
            if (!s) throw std::runtime_error("Invalid statement inside block.");
            compileStatement(s.get());
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
        if (letStmt->declaredType == ValueType::Long) {
            emit(OpCode::DeclareLong, letStmt->name);
        } else {
            emit(OpCode::DeclareInt, letStmt->name);
        }
        return;
    }

    if (const auto* assignStmt = dynamic_cast<const AssignStmt*>(stmt)) {
        compileExpression(assignStmt->value.get());
        emit(OpCode::StoreVar, assignStmt->name);
        return;
    }

    if (const auto* printStmt = dynamic_cast<const PrintStmt*>(stmt)) {
        compileExpression(printStmt->value.get());
        emit(OpCode::Print);
        return;
    }

    if (const auto* exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        compileExpression(exprStmt->expression.get());
        emit(OpCode::Pop);
        return;
    }

    throw std::runtime_error("Compiler error: Unsupported statement type.");
}

void Compiler::compileExpression(const Expr* expr) {
    if (const auto* numberExpr = dynamic_cast<const NumberExpr*>(expr)) {
        emit(OpCode::PushInt, numberExpr->value);
        return;
    }

    if (const auto* boolExpr = dynamic_cast<const BoolExpr*>(expr)) {
        emit(OpCode::PushInt, boolExpr->value ? "1" : "0");
        return;
    }

    if (dynamic_cast<const InputExpr*>(expr) != nullptr) {
        emit(OpCode::Input);
        return;
    }

    if (const auto* identifierExpr = dynamic_cast<const IdentifierExpr*>(expr)) {
        emit(OpCode::LoadVar, identifierExpr->name);
        return;
    }

    if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(expr)) {
        if (unaryExpr->op == "-") {
            if (const auto* numExpr = dynamic_cast<const NumberExpr*>(unaryExpr->right.get())) {
                emit(OpCode::PushInt, "-" + numExpr->value);
                return;
            }
            compileExpression(unaryExpr->right.get());
            emit(OpCode::PushInt, "-1");
            emit(OpCode::Multiply);
            return;
        }
        if (unaryExpr->op == "not") {
            compileExpression(unaryExpr->right.get());
            emit(OpCode::Not);
            return;
        }
        throw std::runtime_error("Compiler error: Unknown unary operator '" + unaryExpr->op + "'.");
    }

    if (const auto* binaryExpr = dynamic_cast<const BinaryExpr*>(expr)) {
        compileExpression(binaryExpr->left.get());
        compileExpression(binaryExpr->right.get());

        const std::string& op = binaryExpr->op;
        if      (op == "+")   emit(OpCode::Add);
        else if (op == "-")   emit(OpCode::Subtract);
        else if (op == "^")   emit(OpCode::Power);
        else if (op == "*")   emit(OpCode::Multiply);
        else if (op == "/")   emit(OpCode::Divide);
        else if (op == "%")   emit(OpCode::Modulo);
        else if (op == "==")  emit(OpCode::Equal);
        else if (op == "!=")  emit(OpCode::NotEqual);
        else if (op == "<")   emit(OpCode::Less);
        else if (op == "<=")  emit(OpCode::LessEqual);
        else if (op == ">")   emit(OpCode::Greater);
        else if (op == ">=")  emit(OpCode::GreaterEqual);
        else if (op == "and") emit(OpCode::And);
        else if (op == "or")  emit(OpCode::Or);
        else throw std::runtime_error("Compiler error: Unknown binary operator '" + op + "'.");
        return;
    }

    throw std::runtime_error("Compiler error: Unsupported expression type.");
}

std::size_t Compiler::emit(OpCode opcode, const std::string& operand) {
    instructions.emplace_back(opcode, operand, currentLine);
    return instructions.size() - 1;
}

void Compiler::patchOperand(std::size_t index, std::size_t target) {
    instructions[index].operand = std::to_string(target);
}
