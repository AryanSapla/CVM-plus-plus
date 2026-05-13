#ifndef VM_H
#define VM_H

#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

#include "opcode.h"

class VM {
public:
    void execute(const std::vector<Instruction>& instructions);
    void execute(const std::vector<Instruction>& instructions, std::istream& input, std::ostream& output);

private:
    void push(int value);
    int pop();

    std::vector<int> stack;
    std::unordered_map<std::string, int> variables;
};

#endif
