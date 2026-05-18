#ifndef BYTECODE_IO_H
#define BYTECODE_IO_H

#include <stdexcept>
#include <string>
#include <vector>

#include "opcode.h"

struct BytecodeError : std::runtime_error {
    std::string header;
    std::string detail;

    explicit BytecodeError(const std::string& detail)
        : std::runtime_error("[Bytecode Error]:\n" + detail),
          header("[Bytecode Error]"),
          detail(detail) {}
};

std::string formatBytecodeListing(const std::vector<Instruction>& instructions);
std::string encodeBytecode(const std::vector<Instruction>& instructions);
std::vector<Instruction> decodeBytecode(const std::string& text);
void writeBytecodeFile(const std::string& path, const std::vector<Instruction>& instructions);
std::vector<Instruction> readBytecodeFile(const std::string& path);
std::string defaultBytecodePath(const std::string& sourcePath);

#endif
