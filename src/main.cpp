#include <cctype>
#include <fstream>
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

// ─── File I/O ─────────────────────────────────────────────────────────────────

struct CommandLineOptions {
    enum class Mode {
        RunSource,
        CompileSource,
        RunBytecode
    };

    Mode        mode = Mode::RunSource;
    bool        debugMode = false;
    std::string scriptPath;
};

static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        // [File Error]:\nCould not open source file: <path>
        throw std::runtime_error("[File Error]:\nCould not open source file: " + path);
    }
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// ─── Source line extractor ────────────────────────────────────────────────────

static std::string getSourceLine(const std::string& source, int lineNo) {
    std::istringstream ss(source);
    std::string line;
    int ln = 0;
    while (std::getline(ss, line)) {
        if (++ln == lineNo) return line;
    }
    return "";
}

// ─── Underline builder ────────────────────────────────────────────────────────
//
// Builds the "  ^^^" or "  ~~~" underline string for a source line.
// If `underlineToken` is non-empty, tries to find the exact substring
// in the source line and underlines only that span.
// Falls back to whole-line underline (~) if not found.

static std::string buildUnderline(const std::string& srcLine,
                                   const std::string& underlineToken) {
    if (underlineToken.empty()) {
        // Whole line
        std::string ul;
        for (char c : srcLine) ul += (c == '\t' ? '\t' : '~');
        return ul;
    }

    // Try to find the token in the source line
    // 1. Exact match
    auto tryPos = [&](const std::string& needle) -> std::size_t {
        return srcLine.find(needle);
    };

    std::string needle = underlineToken;
    std::size_t pos = tryPos(needle);

    // 2. Spaced variant: "a+b" → "a + b"
    if (pos == std::string::npos) {
        for (std::size_t i = 1; i + 1 < needle.size(); ++i) {
            char op = needle[i];
            if (op == '+' || op == '-' || op == '*' || op == '/' || op == '%' || op == '^') {
                std::string spaced = needle.substr(0, i) + " " + op + " " + needle.substr(i + 1);
                pos = tryPos(spaced);
                if (pos != std::string::npos) { needle = spaced; break; }
            }
        }
    }

    if (pos == std::string::npos) {
        // Fall back to whole line
        std::string ul;
        for (char c : srcLine) ul += (c == '\t' ? '\t' : '~');
        return ul;
    }

    std::string ul;
    for (std::size_t i = 0; i < pos; ++i)
        ul += (srcLine[i] == '\t' ? '\t' : ' ');
    for (std::size_t i = 0; i < needle.size(); ++i)
        ul += '^';
    return ul;
}

// ─── Pretty error printer ─────────────────────────────────────────────────────
//
// Output format:
//
//   \033[1;31m[Category Error] [Line N]:\033[0m
//   \033[1;31mMessage text\033[0m
//     source line
//     ^^^^^ (red underline)
//
// `header`          – e.g. "[Lexer Error] [Line 3]"
// `detail`          – the message after the header
// `errLine`         – 1-based line number (0 = no source snippet)
// `source`          – full source text
// `underlineToken`  – substring to underline; "" = whole line

static void printError(const std::string& header,
                       const std::string& detail,
                       int errLine,
                       const std::string& source,
                       const std::string& underlineToken = "") {
    // Print header in bold red
    std::cerr << "\033[1;31m" << header << ":\033[0m\n";
    // Print detail in bold red
    std::cerr << "\033[1;31m" << detail << "\033[0m\n";

    if (errLine > 0 && !source.empty()) {
        std::string srcLine = getSourceLine(source, errLine);
        if (!srcLine.empty()) {
            std::cerr << "  " << srcLine << "\n";
            std::string ul = buildUnderline(srcLine, underlineToken);
            std::cerr << "  \033[1;31m" << ul << "\033[0m\n";
        }
    }
}

// Overload: parses "header\ndetail" from what() string (legacy path)
static void printErrorFromWhat(const std::string& what,
                                int errLine,
                                const std::string& source,
                                const std::string& underlineToken = "") {
    // Split at first '\n'
    auto nl = what.find('\n');
    std::string header = (nl != std::string::npos) ? what.substr(0, nl) : what;
    std::string detail = (nl != std::string::npos) ? what.substr(nl + 1) : "";
    printError(header, detail, errLine, source, underlineToken);
}

static void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <file.cvm>\n"
              << "   or: " << programName << " --debug <file.cvm>\n"
              << "   or: " << programName << " --compile <file.cvm>\n"
              << "   or: " << programName << " --run-bytecode <file.bc>\n";
}

static bool parseArguments(int argc, char* argv[], CommandLineOptions& options) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug") {
            options.debugMode = true;
            continue;
        }
        if (arg == "--compile") {
            if (options.mode != CommandLineOptions::Mode::RunSource) {
                std::cerr << "\033[1;31m[CLI Error]:\033[0m\n"
                          << "\033[1;31mChoose only one execution mode\033[0m\n";
                return false;
            }
            options.mode = CommandLineOptions::Mode::CompileSource;
            continue;
        }
        if (arg == "--run-bytecode") {
            if (options.mode != CommandLineOptions::Mode::RunSource) {
                std::cerr << "\033[1;31m[CLI Error]:\033[0m\n"
                          << "\033[1;31mChoose only one execution mode\033[0m\n";
                return false;
            }
            options.mode = CommandLineOptions::Mode::RunBytecode;
            continue;
        }
        if (arg.rfind("--", 0) == 0) {
            std::cerr << "\033[1;31m[CLI Error]:\033[0m\n"
                      << "\033[1;31mUnknown option '" << arg << "'\033[0m\n";
            return false;
        }
        if (!options.scriptPath.empty()) {
            std::cerr << "\033[1;31m[CLI Error]:\033[0m\n"
                      << "\033[1;31mExpected a single source file. Received extra argument '"
                      << arg << "'\033[0m\n";
            return false;
        }
        options.scriptPath = arg;
    }

    if (options.scriptPath.empty()) {
        printUsage(argv[0]);
        return false;
    }

    if (options.debugMode && options.mode != CommandLineOptions::Mode::RunSource) {
        std::cerr << "\033[1;31m[CLI Error]:\033[0m\n"
                  << "\033[1;31m'--debug' is only supported when running source files\033[0m\n";
        return false;
    }

    return true;
}

static bool hasSuffix(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static void printBytecode(const std::vector<Instruction>& bytecode) {
    std::cout << "Bytecode:\n" << formatBytecodeListing(bytecode);
}

static void printFinalResult(const ExecutionResult& result) {
    if (!result.hasValue) {
        std::cout << "Final Result: <no value>\n";
        return;
    }
    std::cout << "Final Result: " << result.value << '\n';
}

static int runBytecodeProgram(const std::vector<Instruction>& bytecode,
                              bool debugMode,
                              const std::string& sourceForErrors = "") {
    if (debugMode) {
        printBytecode(bytecode);
        std::cout << "\nVM Output:\n";
    }

    try {
        VM vm;
        ExecutionResult result = vm.execute(bytecode, sourceForErrors, std::cin, std::cout);
        if (debugMode) {
            std::cout << '\n';
            printFinalResult(result);
        } else if (!result.printedValue && result.hasValue) {
            std::cout << result.value << '\n';
        }
        return 0;
    } catch (const VMError& e) {
        if (debugMode) std::cout << '\n';
        std::cerr << "\033[1;31m" << e.header << ":\033[0m\n"
                  << "\033[1;31m" << e.detail << "\033[0m\n";
        return 1;
    } catch (const RuntimeError& e) {
        if (debugMode) std::cout << '\n';
        printError(e.header, e.detail, e.line, sourceForErrors, e.token);
        return 1;
    } catch (const std::exception& e) {
        if (debugMode) std::cout << '\n';
        std::cerr << "\033[1;31m[Runtime Error]:\033[0m\n"
                  << "\033[1;31m" << e.what() << "\033[0m\n";
        return 1;
    }
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    CommandLineOptions options;
    if (!parseArguments(argc, argv, options)) {
        if (!options.scriptPath.empty()) printUsage(argv[0]);
        return 1;
    }

    // ── File extension check ──────────────────────────────────────────────────
    {
        bool validSource = hasSuffix(options.scriptPath, ".cvm");
        bool validBytecode = hasSuffix(options.scriptPath, ".bc");
        bool extensionOk =
            (options.mode == CommandLineOptions::Mode::RunBytecode) ? validBytecode : validSource;

        if (!extensionOk) {
            std::cerr << "\033[1;31m[File Error]:\033[0m\n"
                      << "\033[1;31mInvalid file format. Expected '"
                      << ((options.mode == CommandLineOptions::Mode::RunBytecode) ? ".bc" : ".cvm")
                      << "' input\033[0m\n";
            return 1;
        }
    }

    if (options.mode == CommandLineOptions::Mode::RunBytecode) {
        try {
            std::vector<Instruction> bytecode = readBytecodeFile(options.scriptPath);
            return runBytecodeProgram(bytecode, false);
        } catch (const BytecodeError& e) {
            std::cerr << "\033[1;31m" << e.header << ":\033[0m\n"
                      << "\033[1;31m" << e.detail << "\033[0m\n";
            return 1;
        } catch (const std::exception& e) {
            std::cerr << "\033[1;31m[Bytecode Error]:\033[0m\n"
                      << "\033[1;31m" << e.what() << "\033[0m\n";
            return 1;
        }
    }

    // ── Read source ───────────────────────────────────────────────────────────
    std::string source;
    try {
        source = readFile(options.scriptPath);
    } catch (const std::exception& e) {
        // e.what() already has "[File Error]:\n..." format
        std::string w = e.what();
        auto nl = w.find('\n');
        std::string hdr = (nl != std::string::npos) ? w.substr(0, nl) : w;
        std::string det = (nl != std::string::npos) ? w.substr(nl + 1) : "";
        std::cerr << "\033[1;31m" << hdr << "\033[0m\n"
                  << "\033[1;31m" << det << "\033[0m\n";
        return 1;
    }

    // ── Handle empty file ─────────────────────────────────────────────────────
    {
        bool allWhite = true;
        for (char c : source) if (!std::isspace((unsigned char)c)) { allWhite = false; break; }
        if (allWhite) {
            // Valid empty program — no output, exit 0
            return 0;
        }
    }

    // ── Lex ───────────────────────────────────────────────────────────────────
    std::vector<Token> tokens;
    try {
        Lexer lexer(source);
        tokens = lexer.tokenize();
    } catch (const LexerError& e) {
        printError(e.header, e.detail, e.line, source, e.token);
        return 1;
    }

    // ── Parse (+ semantic analysis) ───────────────────────────────────────────
    std::vector<std::unique_ptr<Stmt>> statements;
    try {
        Parser parser(tokens);
        statements = parser.parse();
    } catch (const SemanticError& e) {
        printError(e.header, e.detail, e.line, source, e.token);
        return 1;
    } catch (const ParseError& e) {
        printError(e.header, e.detail, e.line, source, e.token);
        return 1;
    }

    if (options.debugMode) {
        std::cout << "AST:\n";
        for (const auto& stmt : statements)
            std::cout << stmt->toString() << '\n';
    }

    // ── Compile ───────────────────────────────────────────────────────────────
    std::vector<Instruction> bytecode;
    try {
        Compiler compiler;
        bytecode = compiler.compile(statements);
    } catch (const std::exception& e) {
        std::cerr << "\033[1;31m[Compiler Error]:\033[0m\n"
                  << "\033[1;31m" << e.what() << "\033[0m\n";
        return 1;
    }

    if (options.mode == CommandLineOptions::Mode::CompileSource) {
        std::string outputPath = defaultBytecodePath(options.scriptPath);
        try {
            writeBytecodeFile(outputPath, bytecode);
        } catch (const BytecodeError& e) {
            std::cerr << "\033[1;31m" << e.header << ":\033[0m\n"
                      << "\033[1;31m" << e.detail << "\033[0m\n";
            return 1;
        } catch (const std::exception& e) {
            std::cerr << "\033[1;31m[Bytecode Error]:\033[0m\n"
                      << "\033[1;31m" << e.what() << "\033[0m\n";
            return 1;
        }

        printBytecode(bytecode);
        std::cout << "Bytecode file: " << outputPath << '\n';
        return 0;
    }

    return runBytecodeProgram(bytecode, options.debugMode, source);
}
