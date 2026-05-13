#ifndef VM_H
#define VM_H

#include <string>
#include <unordered_map>
#include <vector>

#include "opcode.h"

class VM {
public:
    void execute(const std::vector<Instruction>& instructions);

private:
    void push(int value);
    int pop();

    std::vector<int> stack;
    std::unordered_map<std::string, int> variables;
};

#endif
