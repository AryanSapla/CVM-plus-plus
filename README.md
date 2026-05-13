# CVM++

CVM++ is a small compiler project written in C++ that takes a custom scripting language through the full pipeline:

`source code -> tokens -> AST -> bytecode -> stack-based VM execution`

## Features

- Integer values and boolean values
- Arithmetic: `+`, `-`, `*`, `/`
- Comparisons: `==`, `<`
- Variable declaration with `let`
- Variable reassignment
- `print` statements
- Built-in `input` keyword
- `if / else` control flow
- `while` loops
- File-based execution for `.cvm` scripts

## Project Structure

```text
CVM++/
├── include/
│   ├── ast.h
│   ├── compiler.h
│   ├── lexer.h
│   ├── opcode.h
│   ├── parser.h
│   ├── token.h
│   └── vm.h
├── src/
│   ├── compiler.cpp
│   ├── lexer.cpp
│   ├── main.cpp
│   ├── parser.cpp
│   └── vm.cpp
├── examples/
│   ├── control_flow.cvm
│   ├── input_demo.cvm
│   ├── math.cvm
│   └── test.cvm
├── tests/
│   └── test_pipeline.cpp
└── CMakeLists.txt
```

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Run

Run any `.cvm` file:

```bash
cd build
./cvmpp ../examples/test.cvm
```

Show tokens, AST, and bytecode only when needed:

```bash
cd build
./cvmpp --debug ../examples/test.cvm
```

Try the input demo:

```bash
cd build
./cvmpp ../examples/input_demo.cvm
```

## Tests

```bash
cd build
ctest --output-on-failure
```

## Example Language Syntax

```cvm
let limit = input;
let x = 0;

while (x < limit) {
    print x;
    x = x + 1;
}

if (limit == 3) {
    print true;
} else {
    print false;
}
```

## Notes

- Booleans are currently represented by the VM as `1` for true and `0` for false.
- The runtime reads integer input for the `input` keyword.
- Normal execution prints only the final program output.
- Use `--debug` to print tokens, AST, and bytecode.
