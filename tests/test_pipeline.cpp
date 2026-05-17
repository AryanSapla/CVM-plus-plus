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
void expectLexerError(const std::string& expectedDetail,
                      int expectedLine,
                      const std::string& expectedToken,
                      Func func) {
    try {
        func();
        throw std::runtime_error("Expected lexer error was not thrown.");
    } catch (const LexerError& error) {
        expect(error.detail == expectedDetail, "Unexpected lexer error detail.");
        expect(error.line == expectedLine, "Unexpected lexer error line.");
        expect(error.token == expectedToken, "Unexpected lexer underline token.");
    }
}

template <typename Func>
void expectParseError(const std::string& expectedDetail,
                      int expectedLine,
                      const std::string& expectedToken,
                      Func func) {
    try {
        func();
        throw std::runtime_error("Expected parse error was not thrown.");
    } catch (const ParseError& error) {
        expect(error.detail == expectedDetail, "Unexpected parse error detail.");
        expect(error.line == expectedLine, "Unexpected parse error line.");
        expect(error.token == expectedToken, "Unexpected parse underline token.");
    }
}

template <typename Func>
void expectSemanticError(const std::string& expectedDetail,
                         int expectedLine,
                         const std::string& expectedToken,
                         Func func) {
    try {
        func();
        throw std::runtime_error("Expected semantic error was not thrown.");
    } catch (const SemanticError& error) {
        expect(error.detail == expectedDetail, "Unexpected semantic error detail.");
        expect(error.line == expectedLine, "Unexpected semantic error line.");
        expect(error.token == expectedToken, "Unexpected semantic underline token.");
    }
}

template <typename Func>
void expectRuntimeError(const std::string& expectedDetail,
                        int expectedLine,
                        const std::string& expectedToken,
                        Func func) {
    try {
        func();
        throw std::runtime_error("Expected runtime error was not thrown.");
    } catch (const RuntimeError& error) {
        expect(error.detail == expectedDetail, "Unexpected runtime error detail.");
        expect(error.line == expectedLine, "Unexpected runtime error line.");
        expect(error.token == expectedToken, "Unexpected runtime underline token.");
    }
}

std::vector<Token> lexSource(const std::string& source) {
    Lexer lexer(source);
    return lexer.tokenize();
}

std::vector<std::unique_ptr<Stmt>> parseSource(const std::string& source) {
    std::vector<Token> tokens = lexSource(source);
    Parser parser(tokens);
    return parser.parse();
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
    vm.execute(bytecode, source, in, out);
    return out.str();
}

void testLexerKeywords() {
    std::vector<Token> tokens = lexSource("let x = 1; int y = 2; long int z = 3; print z;");

    expect(tokens[0].type == TokenType::Let, "Expected let token.");
    expect(tokens[5].type == TokenType::IntKeyword, "Expected int keyword token.");
    expect(tokens[10].type == TokenType::LongKeyword, "Expected long keyword token.");
    expect(tokens[11].type == TokenType::IntKeyword, "Expected trailing int keyword token.");
}

void testComments() {
    expect(executeSource(
               "# skip this line\n"
               "print 2;\n"
               "/* multi\n"
               "line\n"
               "comment */\n"
               "print 3;\n"
               "// c++ style comment\n"
               "print 4;\n") == "2\n3\n4\n",
           "Comment handling failed.");

    expectLexerError("Unterminated block comment", 1, "/*", []() {
        lexSource("/* missing end");
    });
}

void testTypedDeclarations() {
    expect(executeSource("let x = 5; print x;") == "5\n", "let declaration failed.");
    expect(executeSource("int x = 2147483647; print x;") == "2147483647\n", "int declaration failed.");
    expect(executeSource("long x = 2147483648; print x;") == "2147483648\n", "long declaration failed.");
    expect(executeSource("long int x = 2147483648; print x;") == "2147483648\n",
           "long int declaration failed.");
}

void testNegativeNumbers() {
    expect(executeSource("print -5;") == "-5\n", "Negative literal execution failed.");
    expect(executeSource("print -(2 + 3);") == "-5\n", "Unary minus execution failed.");
}

void testPowerOperator() {
    expect(executeSource("print 2 ^ 3;") == "8\n", "Basic power execution failed.");
    expect(executeSource("print 2 ^ 3 ^ 2;") == "512\n", "Power should be right-associative.");
    expect(executeSource("print 2 * 3 ^ 2;") == "18\n", "Power precedence over multiply failed.");
    expect(executeSource("print -2 ^ 2;") == "-4\n", "Unary minus with power precedence failed.");
}

void testControlFlowAndInput() {
    const std::string source =
        "let limit = input; "
        "let x = 0; "
        "while (x < limit) { "
        "  print x; "
        "  x = x + 1; "
        "} "
        "if (limit == 2) { "
        "  print true; "
        "} else { "
        "  print false; "
        "}";

    expect(executeSource(source, "2\n") == "input> 0\n1\n1\n", "Control flow or input failed.");
}

void testRuntimeFailures() {
    expectRuntimeError("Division by zero", 1, "10/0", []() {
        executeSource("print 10 / 0;");
    });

    expectRuntimeError("Integer overflow during addition", 1, "2147483647+1", []() {
        executeSource("print 2147483647 + 1;");
    });

    expectRuntimeError("Value 2147483648 is outside int range", 1, "x", []() {
        executeSource("let x = 2147483648; print x;");
    });

    expectRuntimeError("Expected integer input", 1, "input", []() {
        executeSource("let x = input; print x;", "hello\n");
    });

    expectRuntimeError("Integer overflow during multiplication", 1, "2147483648^3", []() {
        executeSource("print 2147483648 ^ 3;");
    });

    expectRuntimeError("Negative exponent is not supported", 1, "2^-1", []() {
        executeSource("print 2 ^ -1;");
    });
}

void testSemanticFailures() {
    expectSemanticError("Undefined variable 'x'", 1, "x", []() {
        parseSource("print x;");
    });

    expectSemanticError("Undefined variable 'x'", 1, "x", []() {
        parseSource("x = 5;");
    });
}

void testLexerAndParseFailures() {
    expectLexerError("Unexpected character '@'", 1, "@", []() {
        lexSource("print 5 @ 3;");
    });

    expectParseError("Expected ';' after variable declaration. Found 'print'", 2, "print", []() {
        parseSource("let x = 5\nprint x;");
    });

    expectParseError("Unexpected identifier 'repeat'", 1, "repeat", []() {
        parseSource("repeat 5;");
    });

    expectParseError("Expected expression after 'print'", 1, ";", []() {
        parseSource("print ;");
    });

    expectParseError("Expected ')' after expression. Found ';'", 1, ";", []() {
        parseSource("print (5 + 2;");
    });

    expectParseError("Invalid assignment target", 1, "5", []() {
        parseSource("5 = x;");
    });

    expectParseError("Expected expression inside condition", 1, ")", []() {
        parseSource("while () {}");
    });

    expectParseError("Expected '}'", 2, ";", []() {
        parseSource("if (x < 5) {\nprint x;\n");
    });

    expectParseError("Invalid assignment inside print statement", 1, "x", []() {
        parseSource("print x = 3;");
    });

    expectParseError("Unexpected token ';' in expression", 1, ";", []() {
        parseSource("let x = ;");
    });
}

}  // namespace

int main() {
    try {
        testLexerKeywords();
        testComments();
        testTypedDeclarations();
        testNegativeNumbers();
        testPowerOperator();
        testControlFlowAndInput();
        testRuntimeFailures();
        testSemanticFailures();
        testLexerAndParseFailures();
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }

    std::cout << "All CVM++ tests passed.\n";
    return 0;
}
