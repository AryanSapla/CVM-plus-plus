#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "vm.h"

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::vector<Token> lexSource(const std::string& source) {
    Lexer lexer(source);
    return lexer.tokenize();
}

std::vector<std::unique_ptr<Stmt>> parseSource(const std::string& source) {
    std::vector<Token> tokens = lexSource(source);
    Parser parser(tokens);
    std::vector<std::unique_ptr<Stmt>> statements = parser.parse();

    for (const auto& statement : statements) {
        expect(statement != nullptr, "Parse produced a null statement.");
    }

    return statements;
}

std::vector<Instruction> compileSource(const std::string& source) {
    std::vector<std::unique_ptr<Stmt>> statements = parseSource(source);
    Compiler compiler;
    return compiler.compile(statements);
}

std::string executeSource(const std::string& source, const std::string& input = "") {
    std::vector<Instruction> bytecode = compileSource(source);
    std::istringstream in(input);
    std::ostringstream out;
    VM vm;
    vm.execute(bytecode, in, out);
    return out.str();
}

void testLexerKeywords() {
    std::vector<Token> tokens = lexSource("let x = input; if (x < 3) { print true; } else { print false; }");

    expect(tokens.size() > 10, "Lexer did not produce enough tokens.");
    expect(tokens[0].type == TokenType::Let, "Expected first token to be let.");
    expect(tokens[3].type == TokenType::Input, "Expected input token.");
    expect(tokens[5].type == TokenType::If, "Expected if token.");
    expect(tokens[9].type == TokenType::Number, "Expected number token.");
}

void testArithmeticAndAssignment() {
    const std::string source =
        "let x = 2; "
        "x = x + 3; "
        "print x;";

    expect(executeSource(source) == "5\n", "Arithmetic or assignment execution failed.");
}

void testControlFlow() {
    const std::string source =
        "let x = 0; "
        "while (x < 3) { "
        "    print x; "
        "    x = x + 1; "
        "} "
        "if (x == 3) { "
        "    print true; "
        "} else { "
        "    print false; "
        "}";

    expect(executeSource(source) == "0\n1\n2\n1\n", "Control-flow execution failed.");
}

void testInputExecution() {
    const std::string source =
        "let limit = input; "
        "let x = 0; "
        "while (x < limit) { "
        "    print x; "
        "    x = x + 1; "
        "}";

    expect(executeSource(source, "2\n") == "input> 0\n1\n", "Input execution failed.");
}

}  // namespace

int main() {
    try {
        testLexerKeywords();
        testArithmeticAndAssignment();
        testControlFlow();
        testInputExecution();
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }

    std::cout << "All CVM++ tests passed.\n";
    return 0;
}
