<div align="center">

<a href="#what-is-cvm">
<pre>
 ██████╗██╗   ██╗███╗   ███╗    ██╗    ██╗
██╔════╝██║   ██║████╗ ████║   ██╔╝   ██╔╝
██║     ██║   ██║██╔████╔██║  ██╔╝   ██╔╝ 
██║     ╚██� ██╔╝██║╚██╔╝██║ ██╔╝   ██╔╝  
╚██████╗ ╚████╔╝ ██║ ╚═╝ ██║██╔╝   ██╔╝   
 ╚═════╝  ╚═══╝  ╚═╝     ╚═╝╚═╝    ╚═╝   
                      ++
</pre>
</a>

**A hand-crafted compiler and stack-based virtual machine — written from scratch in C++17.**

![Language](https://img.shields.io/badge/language-C%2B%2B17-blue?style=flat-square)
![Scripts](https://img.shields.io/badge/scripts-.cvm-orange?style=flat-square)
![Build](https://img.shields.io/badge/build-CMake-green?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-purple?style=flat-square)

</div>

---

## <a id="what-is-cvm"></a>What is CVM++?

CVM++ is a fully hand-written scripting language — no ANTLR, no LLVM, no shortcuts. Every stage of the pipeline is implemented from scratch in C++17, from raw source text to execution:

```
Source (.cvm)  ──▶  Lexer  ──▶  Parser  ──▶  Compiler  ──▶  VM
```

Write `.cvm` scripts and run them with a single command. CVM++ gives you real types, scoped variables, proper control flow, and precise error messages with coloured source underlines.

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
./build/cvmpp script.cvm

# Debug mode — prints tokens, AST, and bytecode before executing
./build/cvmpp --debug script.cvm
```

---

## Language Reference

### Types

| Type | Size | Example |
|------|------|---------|
| `int` | 32-bit signed | `int x = 42;` |
| `long long int` | 64-bit signed | `long long int big = 2^^62;` |
| `float` | 64-bit double | `float pi = 3.14;` |
| `bool` | 1-bit | `bool flag = true;` |

Use `let` for type inference from the right-hand side literal. Use `size(type)` to get the bit-width of a type at runtime (e.g. `size(int)` → `32`).

### Operators

| Category | Operators |
|----------|-----------|
| Arithmetic | `+` `-` `*` `/` `%` `^^` (power) |
| Comparison | `==` `!=` `<` `<=` `>` `>=` |
| Logical | `and` `or` `not` — short-circuit evaluated |
| Bitwise | `&` `\|` `^` `~` `<<` `>>` |
| Cast | `(int)` `(float)` `(long long int)` `(bool)` |

### Control Flow

```cvm
// if / else if / else
if (x > 0) {
    print x;
} else if (x == 0) {
    print 0;
} else {
    print -1;
}

// while with break / continue
while (x < 100) {
    if (x % 2 == 0) { continue; }
    if (x == 99)    { break;    }
    x = x + 1;
}

// for — init, condition, and update are all optional
for (int i = 0; i < 10; i++) { print i; }
for (;;) { break; }   // infinite loop
```

### I/O

```cvm
print x;        // print any value to stdout
int n = input;  // read one value from stdin (auto-parsed as int or float)
```

### Comments

```cvm
// line comment
# also a line comment
/* block comment */
```

---

## Error Messages

CVM++ reports precise, coloured errors with source underlines. Errors are categorised into five phases:

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

| Category | When it fires |
|----------|--------------|
| `Lexer` | Unrecognised character or unterminated block comment |
| `Parse` | Syntax error — unexpected or missing token |
| `Semantic` | Undefined variable, redeclaration, `break`/`continue` outside loop |
| `Runtime` | Division by zero, type mismatch, undefined variable at runtime |
| `VM` | Internal interpreter fault |

---

## Project Structure

```
CVM++/
├── src/
│   ├── lexer.cpp        # Tokeniser
│   ├── parser.cpp       # Recursive-descent parser + semantic analysis
│   ├── compiler.cpp     # AST → bytecode compiler
│   ├── vm.cpp           # Stack-based bytecode interpreter
│   └── main.cpp         # Entry point and error pretty-printer
│
├── include/
│   ├── lexer.h
│   ├── parser.h
│   ├── ast.h            # Full AST node definitions
│   ├── compiler.h
│   ├── opcode.h         # Opcodes, ValueType, Instruction
│   ├── token.h
│   └── vm.h
│
├── examples/            # Sample .cvm programs
├── tests/               # Pipeline and runtime tests
└── CMakeLists.txt
```

---

## How It Works

| Stage | File | Responsibility |
|-------|------|----------------|
| **Lexer** | `lexer.cpp` | Converts source text into a flat token stream. Handles all comment styles, integer/float literals, the `LL` suffix, keywords, and multi-character operators. |
| **Parser** | `parser.cpp` | Recursive-descent parser that builds a typed AST, then runs a semantic pass for scope analysis and undefined-variable detection. |
| **Compiler** | `compiler.cpp` | Tree-walks the AST and emits a flat `Instruction` vector. Short-circuit `and`/`or` compile to jump chains; `break`/`continue` use a patch list resolved after the loop. |
| **VM** | `vm.cpp` | Stack-based interpreter with a scope stack for variable storage. Arithmetic uses overflow-checked helpers via `__int128`. |

---

## Tests

```bash
cd build
ctest --output-on-failure
```

---

## Notes

- `bool` values print as `1` (true) and `0` (false).
- `input` parses the value as float if it contains `.` or `e`; otherwise as integer.
- `let` infers type from the right-hand literal — use an explicit type keyword for clarity.
- `for` init variables are scoped to the loop and do not exist after it ends.
- `break` and `continue` outside any loop are a **parse error**, caught before execution.
- `^^` (power) promotes internally to `long long int` to avoid mid-computation overflow.
- `size(type)` returns bit-widths: `bool` → `1`, `int` → `32`, `long long int` → `64`, `float` → `64`.

---

## License

MIT
