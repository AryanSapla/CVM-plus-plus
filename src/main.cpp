#include <iostream>
#include <vector>

#include "lexer.h"
#include "token.h"

int main() {
    std::string source = "let x = 10 + 20; print x;";
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "Lexer output:\n";

    for (const Token& token : tokens) {
        std::cout << tokenTypeToString(token.type) << " -> " << token.lexeme << '\n';
    }

    return 0;
}
