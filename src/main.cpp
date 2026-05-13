#include <iostream>
#include <memory>
#include <vector>

#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"

int main() {
    std::string source = "let x = 10 + 20 * 2; print x;";

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "Tokens:\n";
    for (const Token& token : tokens) {
        std::cout << tokenTypeToString(token.type) << " -> " << token.lexeme << '\n';
    }

    Parser parser(tokens);
    std::vector<std::unique_ptr<Stmt>> statements = parser.parse();

    std::cout << "\nAST:\n";
    for (const auto& statement : statements) {
        if (!statement) {
            std::cout << "Parse error\n";
            return 1;
        }
        std::cout << statement->toString() << '\n';
    }

    Compiler compiler;
    std::vector<Instruction> bytecode = compiler.compile(statements);

    std::cout << "\nBytecode:\n";
    for (std::size_t i = 0; i < bytecode.size(); ++i) {
        std::cout << i << ": " << opcodeToString(bytecode[i].opcode);
        if (!bytecode[i].operand.empty()) {
            std::cout << " " << bytecode[i].operand;
        }
        std::cout << '\n';
    }

    return 0;
}
