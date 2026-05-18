#include "bytecode_io.h"

#include <fstream>
#include <sstream>

namespace {

constexpr const char* kBytecodeMagic = "CVMBC1";

BytecodeError malformedRecordError(int lineNo) {
    return BytecodeError("Malformed bytecode record at file line " + std::to_string(lineNo));
}

} // namespace

std::string formatBytecodeListing(const std::vector<Instruction>& instructions) {
    std::ostringstream out;
    for (std::size_t i = 0; i < instructions.size(); ++i) {
        out << i << ": " << opcodeToString(instructions[i].opcode);
        if (!instructions[i].operand.empty())
            out << ' ' << instructions[i].operand;
        out << '\n';
    }
    return out.str();
}

std::string encodeBytecode(const std::vector<Instruction>& instructions) {
    std::ostringstream out;
    out << kBytecodeMagic << '\n';
    for (const Instruction& instruction : instructions) {
        out << opcodeToString(instruction.opcode) << '\t'
            << instruction.line << '\t'
            << instruction.operand << '\n';
    }
    return out.str();
}

std::vector<Instruction> decodeBytecode(const std::string& text) {
    std::istringstream input(text);
    std::string line;

    if (!std::getline(input, line) || line != kBytecodeMagic) {
        throw BytecodeError("Invalid bytecode file header");
    }

    std::vector<Instruction> instructions;
    int fileLine = 1;
    while (std::getline(input, line)) {
        ++fileLine;
        if (line.empty()) continue;

        std::size_t firstTab = line.find('\t');
        if (firstTab == std::string::npos) throw malformedRecordError(fileLine);

        std::size_t secondTab = line.find('\t', firstTab + 1);
        if (secondTab == std::string::npos) throw malformedRecordError(fileLine);

        std::string opcodeText = line.substr(0, firstTab);
        std::string lineText = line.substr(firstTab + 1, secondTab - firstTab - 1);
        std::string operand = line.substr(secondTab + 1);

        int sourceLine = 0;
        try {
            std::size_t parsed = 0;
            sourceLine = std::stoi(lineText, &parsed);
            if (parsed != lineText.size())
                throw malformedRecordError(fileLine);
        } catch (const std::invalid_argument&) {
            throw malformedRecordError(fileLine);
        } catch (const std::out_of_range&) {
            throw malformedRecordError(fileLine);
        }

        instructions.emplace_back(opcodeFromString(opcodeText), operand, sourceLine);
    }

    return instructions;
}

void writeBytecodeFile(const std::string& path, const std::vector<Instruction>& instructions) {
    std::ofstream out(path);
    if (!out) {
        throw BytecodeError("Could not open bytecode file for writing: " + path);
    }
    out << encodeBytecode(instructions);
    if (!out) {
        throw BytecodeError("Failed while writing bytecode file: " + path);
    }
}

std::vector<Instruction> readBytecodeFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw BytecodeError("Could not open bytecode file: " + path);
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    return decodeBytecode(buffer.str());
}

std::string defaultBytecodePath(const std::string& sourcePath) {
    return sourcePath + ".bc";
}
