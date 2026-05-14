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

template <typename Func>
void expectRuntimeError(const std::string& expectedMessage, Func func) {
    try {
        func();
        throw std::runtime_error("Expected runtime error was not thrown.");
    } catch (const std::exception& error) {
        expect(error.what() == expectedMessage,
               "Expected runtime error '" + expectedMessage + "' but got '" + error.what() + "'.");
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

std::string parseErrorFor(const std::string& source) {
    std::vector<Token> tokens = lexSource(source);
    Parser parser(tokens);
    std::vector<std::unique_ptr<Stmt>> statements = parser.parse();

    for (const auto& statement : statements) {
        if (!statement) {
            return parser.getErrorMessage();
        }
    }

    throw std::runtime_error("Expected parse error was not produced.");
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
    vm.execute(bytecode, "", in, out);
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

void testNegativeNumbers() {
    expect(executeSource("print -5;") == "-5\n", "Negative literal execution failed.");
    expect(executeSource("print -(2 + 3);") == "-5\n", "Unary minus expression execution failed.");
}

void testPrintDefinedVariable() {
    expect(executeSource("let x = 42; print x;") == "42\n", "Printing a variable failed.");
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

void testDivisionByZero() {
    expectRuntimeError("Division by zero.", []() {
        executeSource("print 10 / 0;");
    });
}

void testIntegerLiteralOutOfRange() {
    expectRuntimeError("Integer literal is outside 32-bit int range: 2147483648", []() {
        executeSource("print 2147483648;");
    });
}

void testInputOutOfRange() {
    expectRuntimeError("Integer input is outside 32-bit int range: 2147483648", []() {
        executeSource("let x = input; print x;", "2147483648\n");
    });
}

void testInvalidInput() {
    expectRuntimeError("Invalid integer input: hello", []() {
        executeSource("let x = input; print x;", "hello\n");
    });
}

void testArithmeticOverflow() {
    expectRuntimeError("Integer overflow during addition.", []() {
        executeSource("print 2147483647 + 1;");
    });
}

void testUndefinedVariable() {
    expectRuntimeError("Undefined variable: x", []() {
        executeSource("print x;");
    });
}

void testParseErrors() {
    expect(parseErrorFor("print @;") == "Unexpected token '@' in expression.",
           "Invalid token parse error message failed.");
    expect(parseErrorFor("print 5") == "Expected ';' after print value. Found end of file.",
           "Missing semicolon parse error message failed.");
}

}  // namespace

int main() {
    try {
        testLexerKeywords();
        testArithmeticAndAssignment();
        testNegativeNumbers();
        testPrintDefinedVariable();
        testControlFlow();
        testInputExecution();
        testDivisionByZero();
        testIntegerLiteralOutOfRange();
        testInputOutOfRange();
        testInvalidInput();
        testArithmeticOverflow();
        testUndefinedVariable();
        testParseErrors();
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }

    std::cout << "All CVM++ tests passed.\n";
    return 0;
}
