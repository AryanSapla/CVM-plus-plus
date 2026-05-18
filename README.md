<div align="center">

```
 ██████╗██╗   ██╗███╗   ███╗    ██╗    ██╗
██╔════╝██║   ██║████╗ ████║   ██╔╝   ██╔╝
██║     ██║   ██║██╔████╔██║  ██╔╝   ██╔╝ 
██║     ╚██╗ ██╔╝██║╚██╔╝██║ ██╔╝   ██╔╝  
╚██████╗ ╚████╔╝ ██║ ╚═╝ ██║██╔╝   ██╔╝   
 ╚═════╝  ╚═══╝  ╚═╝     ╚═╝╚═╝    ╚═╝   
```

**A hand-crafted compiler and stack-based virtual machine, written from scratch in C++.**

![Language](https://img.shields.io/badge/language-C%2B%2B17-blue?style=flat-square)
![Scripts](https://img.shields.io/badge/scripts-.cvm-orange?style=flat-square)
![Build](https://img.shields.io/badge/build-CMake-green?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-purple?style=flat-square)

</div>

---

## What is CVM++?

CVM++ is a fully hand-written scripting language implementation — no ANTLR, no LLVM, no shortcuts.
Every stage of the pipeline is written from scratch in C++17:

```
Source Code  ──▶  Lexer  ──▶  Tokens  ──▶  Parser  ──▶  AST
                                                           │
    Output  ◀──  VM  ◀──  Bytecode  ◀──  Compiler  ◀──────╯
```

Write `.cvm` scripts and run them directly. It's fast, it has real error messages with source underlines, and it handles types, scopes, and control flow properly.

---

## Features

### Types
| Type | Description | Example |
|------|-------------|---------|
| `int` | 32-bit signed integer | `int x = 42;` |
| `long long int` | 64-bit signed integer | `long long int big = 2^^62;` |
| `float` | 64-bit double precision | `float pi = 3.14;` |
| `bool` | Boolean (prints as `1`/`0`) | `bool flag = true;` |

### Operators
| Category | Operators |
|----------|-----------|
| Arithmetic | `+` `-` `*` `/` `%` `^^` (power) |
| Comparison | `==` `!=` `<` `<=` `>` `>=` |
| Logical | `and` `or` `not` (with short-circuit evaluation) |
| Bitwise | `&` `\|` `^` `~` `<<` `>>` |
| Cast | `(int)` `(float)` `(long long int)` `(bool)` |

### Control Flow
```cvm
// if / else
if (x > 0) {
    print x;
} else {
    print 0;
}

// while loop with break and continue
while (x < 100) {
    if (x == 50) { break; }
    if (x % 2 == 0) { continue; }
    print x;
    x = x + 1;
}

// for loop — all three forms work
for (int i = 0; i < 10; i++) { print i; }
for (int i = 10; i > 0; --i) { print i; }
for (int i = 0; i < 10; i = i + 2) { print i; }
for (;;) { break; }   // infinite loop
```

### Other
- `let` — type-inferred variable declaration
- `print` — print any value to stdout
- `input` — read a value from stdin at runtime
- `size(type)` — bit-width of a type (`size(int)` → `32`)
- Block scoping `{ }` — variables declared inside don't leak out
- `//`, `#`, `/* */` comments — all three styles supported

---

## Error Messages

CVM++ gives you precise, coloured error messages with source underlines:

```
[Semantic Error] [Line 5]:
Undefined variable 'count'
  print count;
        ^^^^^
```

```
[Runtime Error] [Line 3]:
Division by zero
  int result = x / 0;
               ^^^^^
```

Errors are categorised into: `Lexer`, `Parse`, `Semantic`, `Runtime`, and `VM`.

---

## Project Structure

```
CVM++/
├── include/
│   ├── lexer.h          # Token types and Lexer interface
│   ├── parser.h         # AST node types and Parser interface
│   ├── ast.h            # Full AST node definitions
│   ├── compiler.h       # Bytecode Compiler interface
│   ├── opcode.h         # Opcodes, ValueType, Instruction struct
│   ├── token.h          # TokenType enum and Token struct
│   └── vm.h             # Virtual Machine interface
│
├── src/
│   ├── lexer.cpp        # Tokeniser
│   ├── parser.cpp       # Recursive-descent parser + semantic checks
│   ├── compiler.cpp     # AST → bytecode compiler
│   ├── vm.cpp           # Stack-based bytecode interpreter
│   └── main.cpp         # Entry point, error pretty-printer
│
├── examples/            # Sample .cvm programs
├── tests/               # Pipeline and runtime tests
├── build/               # Generated build output
└── CMakeLists.txt
```

---

## Build

**Requirements:** C++17 compiler, CMake 3.15+

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

---

## Run

```bash
# Run a script
./build/cvmpp examples/hello.cvm

# Debug mode — prints tokens, AST, and bytecode before running
./build/cvmpp --debug examples/hello.cvm
```

---

## Example Programs

### FizzBuzz
```cvm
for (int i = 1; i <= 20; i++) {
    if (i % 15 == 0) { print 0; }   // "FizzBuzz" → 0 as placeholder
    else if (i % 3 == 0) { print 3; }
    else if (i % 5 == 0) { print 5; }
    else { print i; }
}
```

### Sum of user input
```cvm
int n = input;
int sum = 0;
for (int i = 1; i <= n; i++) {
    sum = sum + i;
}
print sum;
```

### Power of two checker
```cvm
int x = input;
bool isPow2 = (x > 0) and ((x & (x - 1)) == 0);
print isPow2;
```

### Overflow-safe big numbers
```cvm
long long int result = 2^^62;
print result;   // 4611686018427387904
```

---

## Tests

```bash
cd build
ctest --output-on-failure
```

---

## How It Works

| Stage | File | What it does |
|-------|------|--------------|
| **Lexer** | `lexer.cpp` | Converts raw source text into a flat list of typed tokens. Handles all three comment styles, integer/float literals, `LL` suffix, keywords, and multi-char operators. |
| **Parser** | `parser.cpp` | Recursive-descent parser producing a typed AST. Runs a semantic pass (scope analysis, undefined variable detection) before returning. |
| **Compiler** | `compiler.cpp` | Tree-walks the AST and emits a flat vector of `Instruction` structs. Short-circuit `and`/`or` compile to jump chains. `break`/`continue` use a patch list resolved after the loop is fully emitted. |
| **VM** | `vm.cpp` | Stack-based interpreter with a scope stack for variable storage. All arithmetic uses overflow-checked helpers (via `__int128`). |

---

## Notes

- Booleans print as `1` (true) and `0` (false).
- `input` reads one value; if it contains `.` or `e` it is parsed as float, otherwise integer.
- `let` infers type from the literal on the right-hand side. Use an explicit type (`int`, `float`, etc.) for clarity.
- `for` loop init variables are scoped to the loop — they don't exist after it ends.
- `break` and `continue` outside any loop are a **parse error**, not a runtime one.
- `^^` (power) always promotes to `long long int` internally to avoid mid-computation overflow.