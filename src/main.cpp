#include <iostream>
#include <memory>
#include <vector>

#include "lexer.h"
#include "parser.h"
#include "token.h"

int main() {
    std::string source = "let x = 10 + 20; print x;";

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
        if (statement) {
            std::cout << statement->toString() << '\n';
        } else {
            std::cout << "Parse error\n";
        }
    }

    return 0;
}
