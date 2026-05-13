#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "vm.h"

std::string readFile(const std::string& path) {
    std::ifstream input(path);

    if (!input) {
        throw std::runtime_error("Could not open file: " + path);
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ./cvmpp <file.cvm>\n";
        return 1;
    }

    std::string source;

    try {
        source = readFile(argv[1]);
    } catch (const std::exception& error) {
        std::cout << error.what() << '\n';
        return 1;
    }

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

    std::cout << "\nVM Output:\n";
    VM vm;
    vm.execute(bytecode);

    return 0;
}
