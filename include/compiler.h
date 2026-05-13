#ifndef COMPILER_H
#define COMPILER_H

#include <memory>
#include <vector>

#include "ast.h"
#include "opcode.h"

class Compiler {
public:
    std::vector<Instruction> compile(const std::vector<std::unique_ptr<Stmt>>& statements);

private:
    void compileStatement(const Stmt* stmt);
    void compileExpression(const Expr* expr);

    std::vector<Instruction> instructions;
};

#endif
