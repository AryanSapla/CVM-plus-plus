#ifndef COMPILER_H
#define COMPILER_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "ast.h"
#include "opcode.h"

class Compiler {
public:
    std::vector<Instruction> compile(const std::vector<std::unique_ptr<Stmt>>& statements);

private:
    void compileStatement(const Stmt* stmt);
    void compileExpression(const Expr* expr);
    std::size_t emit(OpCode opcode, const std::string& operand = "");
    void patchOperand(std::size_t index, std::size_t target);

    std::vector<Instruction> instructions;
    int currentLine = 0;
};

#endif