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
    bool debugMode = false;
    std::string scriptPath;

    if (argc == 2) {
        scriptPath = argv[1];
    } else if (argc == 3 && std::string(argv[1]) == "--debug") {
        debugMode = true;
        scriptPath = argv[2];
    } else {
        std::cout << "Usage: ./cvmpp <file.cvm>\n";
        std::cout << "   or: ./cvmpp --debug <file.cvm>\n";
        return 1;
    }

    std::string source;

    try {
        source = readFile(scriptPath);
    } catch (const std::exception& error) {
        std::cout << error.what() << '\n';
        return 1;
    }

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    if (debugMode) {
        std::cout << "Tokens:\n";
        for (const Token& token : tokens) {
            std::cout << tokenTypeToString(token.type) << " -> " << token.lexeme << '\n';
        }
    }

    try {
        Parser parser(tokens);
        std::vector<std::unique_ptr<Stmt>> statements = parser.parse();

        if (debugMode) {
            std::cout << "\nAST:\n";
            for (const auto& statement : statements) {
                if (!statement) {
                    std::cout << "Parse error\n";
                    return 1;
                }
                std::cout << statement->toString() << '\n';
            }
        } else {
            for (const auto& statement : statements) {
                if (!statement) {
                    std::cout << "Parse error\n";
                    return 1;
                }
            }
        }

        Compiler compiler;
        std::vector<Instruction> bytecode = compiler.compile(statements);

        if (debugMode) {
            std::cout << "\nBytecode:\n";
            for (std::size_t i = 0; i < bytecode.size(); ++i) {
                std::cout << i << ": " << opcodeToString(bytecode[i].opcode);
                if (!bytecode[i].operand.empty()) {
                    std::cout << " " << bytecode[i].operand;
                }
                std::cout << '\n';
            }

            std::cout << "\nVM Output:\n";
        }

        VM vm;
        vm.execute(bytecode);
    } catch (const std::exception& error) {
        if (debugMode) {
            std::cout << "\n";
        }
        std::cout << "Runtime error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
