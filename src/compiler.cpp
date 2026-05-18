#include "compiler.h"

#include <stdexcept>

std::vector<Instruction> Compiler::compile(const std::vector<std::unique_ptr<Stmt>>& statements) {
    instructions.clear();

    for (const auto& s : statements) {
        if (s) compileStatement(s.get());
    }

    emit(OpCode::Halt);
    return instructions;
}

// ─── Statements ──────────────────────────────────────────────────────────────

void Compiler::compileStatement(const Stmt* stmt) {
    if (stmt->line > 0) currentLine = stmt->line;

    if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(stmt)) {
        emit(OpCode::PushScope);
        for (const auto& s : blockStmt->statements) {
            if (!s) throw std::runtime_error("Invalid statement inside block.");
            compileStatement(s.get());
        }
        emit(OpCode::PopScope);
        return;
    }

    if (const auto* ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        compileExpression(ifStmt->condition.get());
        std::size_t jumpIfFalse = emit(OpCode::JumpIfFalse);
        emit(OpCode::PushScope);
        compileStatement(ifStmt->thenBranch.get());
        emit(OpCode::PopScope);
        if (ifStmt->elseBranch) {
            std::size_t jumpEnd = emit(OpCode::Jump);
            patchOperand(jumpIfFalse, instructions.size());
            emit(OpCode::PushScope);
            compileStatement(ifStmt->elseBranch.get());
            emit(OpCode::PopScope);
            patchOperand(jumpEnd, instructions.size());
        } else {
            patchOperand(jumpIfFalse, instructions.size());
        }
        return;
    }

    if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        std::size_t loopStart = instructions.size();
        compileExpression(whileStmt->condition.get());
        std::size_t jumpIfFalse = emit(OpCode::JumpIfFalse);

        // Push loop context so break/continue can register their jumps
        loopStack.push_back({});

        emit(OpCode::PushScope);
        compileStatement(whileStmt->body.get());
        emit(OpCode::PopScope);

        // continue → jump back to condition
        std::size_t continueTarget = instructions.size();
        emit(OpCode::Jump, std::to_string(loopStart));

        // break → jump past loop
        std::size_t breakTarget = instructions.size();
        patchOperand(jumpIfFalse, breakTarget);

        // Patch all collected break/continue jumps
        LoopContext ctx = std::move(loopStack.back());
        loopStack.pop_back();
        for (std::size_t idx : ctx.continueJumps) patchOperand(idx, continueTarget);
        for (std::size_t idx : ctx.breakJumps)    patchOperand(idx, breakTarget);
        return;
    }

    if (const auto* forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        // Layout:
        //   PushScope              ← for-loop scope (holds init variable)
        //   [init]
        //   loopStart:
        //   [condition + JumpIfFalse → breakTarget]   (omitted if no condition)
        //   PushScope              ← body scope
        //   [body]
        //   PopScope
        //   updateTarget:
        //   [update]
        //   Jump loopStart
        //   breakTarget:
        //   PopScope               ← for-loop scope

        emit(OpCode::PushScope);  // for-loop variable scope
        if (forStmt->init) compileStatement(forStmt->init.get());

        std::size_t loopStart = instructions.size();
        std::size_t jumpIfFalse = 0;
        bool hasCondition = forStmt->condition != nullptr;
        if (hasCondition) {
            compileExpression(forStmt->condition.get());
            jumpIfFalse = emit(OpCode::JumpIfFalse);
        }

        loopStack.push_back({});

        emit(OpCode::PushScope);  // body scope
        compileStatement(forStmt->body.get());
        emit(OpCode::PopScope);

        // continue → run update then jump to condition
        std::size_t updateTarget = instructions.size();
        if (forStmt->update) compileStatement(forStmt->update.get());
        emit(OpCode::Jump, std::to_string(loopStart));

        std::size_t breakTarget = instructions.size();
        if (hasCondition) patchOperand(jumpIfFalse, breakTarget);

        LoopContext ctx = std::move(loopStack.back());
        loopStack.pop_back();
        for (std::size_t idx : ctx.continueJumps) patchOperand(idx, updateTarget);
        for (std::size_t idx : ctx.breakJumps)    patchOperand(idx, breakTarget);

        emit(OpCode::PopScope);  // pop for-loop variable scope
        return;
    }

    if (dynamic_cast<const BreakStmt*>(stmt)) {
        // Jump target is patched once the enclosing loop is fully compiled
        std::size_t jmp = emit(OpCode::Jump);
        loopStack.back().breakJumps.push_back(jmp);
        return;
    }

    if (dynamic_cast<const ContinueStmt*>(stmt)) {
        std::size_t jmp = emit(OpCode::Jump);
        loopStack.back().continueJumps.push_back(jmp);
        return;
    }

    if (const auto* letStmt = dynamic_cast<const LetStmt*>(stmt)) {
        compileExpression(letStmt->value.get());
        switch (letStmt->declaredType) {
            case ValueType::Bool:     emit(OpCode::DeclareBool,     letStmt->name); break;
            case ValueType::LongLong: emit(OpCode::DeclareLongLong, letStmt->name); break;
            case ValueType::Float:    emit(OpCode::DeclareFloat,    letStmt->name); break;
            default:                  emit(OpCode::DeclareInt,      letStmt->name); break;
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

// ─── Expressions ─────────────────────────────────────────────────────────────

void Compiler::compileExpression(const Expr* expr) {

    // ── Literals ──────────────────────────────────────────────────────────────

    if (const auto* numExpr = dynamic_cast<const NumberExpr*>(expr)) {
        emit(OpCode::PushInt, numExpr->value);
        return;
    }

    if (const auto* llExpr = dynamic_cast<const LongLongExpr*>(expr)) {
        emit(OpCode::PushLongLong, llExpr->value);
        return;
    }

    if (const auto* floatExpr = dynamic_cast<const FloatExpr*>(expr)) {
        emit(OpCode::PushFloat, floatExpr->value);
        return;
    }

    if (const auto* boolExpr = dynamic_cast<const BoolExpr*>(expr)) {
        emit(OpCode::PushBool, boolExpr->value ? "1" : "0");
        return;
    }

    // ── Input ─────────────────────────────────────────────────────────────────

    if (dynamic_cast<const InputExpr*>(expr)) {
        emit(OpCode::Input);
        return;
    }

    // ── size(type) ────────────────────────────────────────────────────────────

    if (const auto* sizeExpr = dynamic_cast<const SizeOfExpr*>(expr)) {
        emit(OpCode::PushInt, std::to_string(sizeExpr->bits));
        return;
    }

    // ── Identifier ────────────────────────────────────────────────────────────

    if (const auto* identExpr = dynamic_cast<const IdentifierExpr*>(expr)) {
        emit(OpCode::LoadVar, identExpr->name);
        return;
    }

    // ── Explicit cast ─────────────────────────────────────────────────────────

    if (const auto* castExpr = dynamic_cast<const CastExpr*>(expr)) {
        compileExpression(castExpr->operand.get());
        switch (castExpr->targetType) {
            case ValueType::Int:      emit(OpCode::CastToInt);      break;
            case ValueType::LongLong: emit(OpCode::CastToLongLong); break;
            case ValueType::Float:    emit(OpCode::CastToFloat);    break;
            case ValueType::Bool:     emit(OpCode::CastToBool);     break;
            default:
                throw std::runtime_error("Compiler error: Unknown cast target type.");
        }
        return;
    }

    // ── Unary ─────────────────────────────────────────────────────────────────

    if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(expr)) {
        if (unaryExpr->op == "-") {
            // Optimise: constant folding for negated int/float literals
            if (const auto* numExpr = dynamic_cast<const NumberExpr*>(unaryExpr->right.get())) {
                emit(OpCode::PushInt, "-" + numExpr->value);
                return;
            }
            if (const auto* floatExpr = dynamic_cast<const FloatExpr*>(unaryExpr->right.get())) {
                emit(OpCode::PushFloat, "-" + floatExpr->value);
                return;
            }
            if (const auto* llExpr = dynamic_cast<const LongLongExpr*>(unaryExpr->right.get())) {
                emit(OpCode::PushLongLong, "-" + llExpr->value);
                return;
            }
            compileExpression(unaryExpr->right.get());
            emit(OpCode::PushInt, "-1");
            emit(OpCode::Multiply);
            return;
        }
        if (unaryExpr->op == "~") {
            compileExpression(unaryExpr->right.get());
            emit(OpCode::BitNot);
            return;
        }
        if (unaryExpr->op == "not") {
            compileExpression(unaryExpr->right.get());
            emit(OpCode::Not);
            return;
        }
        throw std::runtime_error("Compiler error: Unknown unary operator '" + unaryExpr->op + "'.");
    }

    // ── Binary ────────────────────────────────────────────────────────────────

    if (const auto* binExpr = dynamic_cast<const BinaryExpr*>(expr)) {

        // Short-circuit AND
        if (binExpr->op == "and") {
            compileExpression(binExpr->left.get());
            std::size_t leftFalse = emit(OpCode::JumpIfFalse);
            compileExpression(binExpr->right.get());
            std::size_t rightFalse = emit(OpCode::JumpIfFalse);
            emit(OpCode::PushBool, "1");
            std::size_t jumpEnd = emit(OpCode::Jump);
            std::size_t falseTarget = instructions.size();
            emit(OpCode::PushBool, "0");
            patchOperand(leftFalse, falseTarget);
            patchOperand(rightFalse, falseTarget);
            patchOperand(jumpEnd, instructions.size());
            return;
        }

        // Short-circuit OR
        if (binExpr->op == "or") {
            compileExpression(binExpr->left.get());
            std::size_t evalRight = emit(OpCode::JumpIfFalse);
            emit(OpCode::PushBool, "1");
            std::size_t jumpEnd = emit(OpCode::Jump);
            std::size_t rightStart = instructions.size();
            compileExpression(binExpr->right.get());
            std::size_t rightFalse = emit(OpCode::JumpIfFalse);
            emit(OpCode::PushBool, "1");
            std::size_t rightEnd = emit(OpCode::Jump);
            std::size_t falseTarget = instructions.size();
            emit(OpCode::PushBool, "0");
            patchOperand(evalRight, rightStart);
            patchOperand(rightFalse, falseTarget);
            patchOperand(jumpEnd, instructions.size());
            patchOperand(rightEnd, instructions.size());
            return;
        }

        compileExpression(binExpr->left.get());
        compileExpression(binExpr->right.get());

        const std::string& op = binExpr->op;
        if      (op == "+")   emit(OpCode::Add);
        else if (op == "-")   emit(OpCode::Subtract);
        else if (op == "*")   emit(OpCode::Multiply);
        else if (op == "/")   emit(OpCode::Divide);
        else if (op == "%")   emit(OpCode::Modulo);
        else if (op == "^^")  emit(OpCode::Power);
        else if (op == "==")  emit(OpCode::Equal);
        else if (op == "!=")  emit(OpCode::NotEqual);
        else if (op == "<")   emit(OpCode::Less);
        else if (op == "<=")  emit(OpCode::LessEqual);
        else if (op == ">")   emit(OpCode::Greater);
        else if (op == ">=")  emit(OpCode::GreaterEqual);
        else if (op == "&")   emit(OpCode::BitAnd);
        else if (op == "|")   emit(OpCode::BitOr);
        else if (op == "^")   emit(OpCode::BitXor);
        else if (op == "<<")  emit(OpCode::ShiftLeft);
        else if (op == ">>")  emit(OpCode::ShiftRight);
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