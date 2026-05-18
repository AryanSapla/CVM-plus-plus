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
    void        patchOperand(std::size_t index, std::size_t target);

    std::vector<Instruction> instructions;
    int currentLine = 0;

    // Each entry is one active loop.  break jumps are patched to the loop's
    // exit target; continue jumps are patched to the loop's continue target
    // (top of condition for while, top of update for for).
    struct LoopContext {
        std::vector<std::size_t> breakJumps;    // indices of Jump instrs for break
        std::vector<std::size_t> continueJumps; // indices of Jump instrs for continue
    };
    std::vector<LoopContext> loopStack;
};

#endif