#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "bytecode_io.h"
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

struct CapturedExecution {
    std::string     output;
    ExecutionResult result;
};

CapturedExecution runSource(const std::string& source, const std::string& input = "") {
    std::vector<Instruction> bytecode = compileSource(source);
    std::istringstream in(input);
    std::ostringstream out;
    VM vm;
    ExecutionResult result = vm.execute(bytecode, source, in, out);
    return {out.str(), result};
}

std::string executeSource(const std::string& source, const std::string& input = "") {
    return runSource(source, input).output;
}

void testLexerKeywords() {
    std::vector<Token> tokens = lexSource("let x = 1; int y = 2; long int z = 3; bool ok = true; print z;");

    expect(tokens[0].type == TokenType::Let, "Expected let token.");
    expect(tokens[5].type == TokenType::IntKeyword, "Expected int keyword token.");
    expect(tokens[10].type == TokenType::LongKeyword, "Expected long keyword token.");
    expect(tokens[11].type == TokenType::IntKeyword, "Expected trailing int keyword token.");
    expect(tokens[16].type == TokenType::BoolKeyword, "Expected bool keyword token.");
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
    expect(executeSource("bool flag = true; print flag;") == "1\n", "bool declaration failed.");
    expect(executeSource("bool flag = false; print flag;") == "0\n", "bool false declaration failed.");
    expect(executeSource("bool flag = 42; print flag;") == "1\n", "bool numeric conversion failed.");
    expect(executeSource("bool flag = 0; print flag;") == "0\n", "bool zero conversion failed.");
    expect(executeSource("bool flag = false; flag = 9; print flag;") == "1\n", "bool assignment conversion failed.");
    expect(executeSource("int x = true + true; print x;") == "2\n", "bool arithmetic promotion failed.");
}

void testNegativeNumbers() {
    expect(executeSource("print -5;") == "-5\n", "Negative literal execution failed.");
    expect(executeSource("print -(2 + 3);") == "-5\n", "Unary minus execution failed.");
}

void testPowerOperator() {
    expect(executeSource("print 2 ^^ 3;") == "8\n", "Basic power execution failed.");
    expect(executeSource("print 2 ^^ 3 ^^ 2;") == "512\n", "Power should be right-associative.");
    expect(executeSource("print 2 * 3 ^^ 2;") == "18\n", "Power precedence over multiply failed.");
    expect(executeSource("print -2 ^^ 2;") == "-4\n", "Unary minus with power precedence failed.");
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

    expect(executeSource("bool ready = true; bool done = false; print !done && ready; print done || ready;") ==
               "1\n1\n",
           "C++-style boolean operators failed.");
    expect(executeSource("bool ready = false; print ready && (10 / 0);") == "0\n",
           "Short-circuit && failed.");
    expect(executeSource("bool ready = true; print ready || (10 / 0);") == "1\n",
           "Short-circuit || failed.");
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

    expectRuntimeError("Integer overflow during multiplication", 1, "2147483648^^3", []() {
        executeSource("print 2147483648 ^^ 3;");
    });

    expectRuntimeError("Negative exponent not supported for integers", 1, "2^^-1", []() {
        executeSource("print 2 ^^ -1;");
    });
}

void testExecutionResult() {
    CapturedExecution assignment = runSource("int total = 8 + 4;");
    expect(assignment.output.empty(), "Assignment should not print output.");
    expect(assignment.result.hasValue, "Assignment should produce a final result.");
    expect(!assignment.result.printedValue, "Assignment should not be flagged as printed output.");
    expect(assignment.result.value == "12", "Unexpected final result for assignment.");
    expect(assignment.result.type == ValueType::Int, "Assignment result type should be int.");

    CapturedExecution expression = runSource("2 + 3;");
    expect(expression.result.hasValue, "Expression statement should produce a final result.");
    expect(!expression.result.printedValue, "Expression result should not be flagged as printed output.");
    expect(expression.result.value == "5", "Unexpected final result for expression statement.");

    CapturedExecution printed = runSource("print 7 * 6;");
    expect(printed.output == "42\n", "Printed output should remain unchanged.");
    expect(printed.result.printedValue, "Printed execution should be flagged as printed output.");
    expect(printed.result.value == "42", "Print should also expose the final VM result.");
}

void testBytecodeRoundTrip() {
    std::vector<Instruction> original = compileSource("int value = 5 + 7; print value;");
    std::string encoded = encodeBytecode(original);
    std::vector<Instruction> decoded = decodeBytecode(encoded);

    expect(decoded.size() == original.size(), "Decoded bytecode size mismatch.");
    for (std::size_t i = 0; i < original.size(); ++i) {
        expect(decoded[i].opcode == original[i].opcode, "Decoded opcode mismatch.");
        expect(decoded[i].operand == original[i].operand, "Decoded operand mismatch.");
        expect(decoded[i].line == original[i].line, "Decoded line number mismatch.");
    }

    std::istringstream in;
    std::ostringstream out;
    VM vm;
    ExecutionResult result = vm.execute(decoded, "", in, out);
    expect(out.str() == "12\n", "Decoded bytecode should execute correctly.");
    expect(result.printedValue, "Decoded bytecode should retain print tracking.");
    expect(result.value == "12", "Decoded bytecode should keep the final result.");
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
        testExecutionResult();
        testBytecodeRoundTrip();
        testSemanticFailures();
        testLexerAndParseFailures();
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }

    std::cout << "All CVM++ tests passed.\n";
    return 0;
}
