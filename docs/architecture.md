# fungcc Architecture Overview

## Pipeline Stages
fungcc currently follows a straight-through pipeline:
1. **Lexer (`src/frontend/lexer.c`)** transforms raw source into tokens. It tracks lexeme spans and source coordinates, handling whitespace, line/block comments, and basic literals.
2. **Parser (`src/frontend/parser.c`)** consumes tokens via a recursive-descent strategy to produce an AST that models translation units, functions, return statements, identifiers, literals, and chained `+`/`-` expressions.
3. **Code Generator (`src/backend/codegen.c`)** walks the AST to emit x86-64 assembly. It currently supports literal immediates, RIP-relative global loads, and left-associative addition/subtraction sequences, writing assembly into a file for later assembly/linking.

## Key Data Structures
- **Tokens** (`include/frontend/token.h`): `TokenKind`, lexeme pointer/length, and line/column metadata.
- **AST Nodes** (`include/frontend/ast.h`): tagged union representing translation unit, function declarations, return statements, identifiers, numbers, and binary expressions. Nodes own their children and are freed via `ast_free`.
- **Parser State** (`include/frontend/parser.h`): embeds a lexer instance, tracks current token, and records status for error propagation.

## Build Targets & Flow
- `fungcc_core`: static library bundling frontend/backend modules.
- `fungcc_driver`: demo executable that parses a hard-coded program, dumps summaries, and emits assembly to `build/fungcc_output.s`.
- Test executables: `test_lexer`, `test_parser`, `test_codegen` registered with CTest; they exercise whitespace/comment handling, parser error detection, and assembly emission scenarios.

Typical loop:
```
cmake --build build
./build/src/fungcc_driver
cc build/fungcc_output.s -o build/fungcc_output
./build/fungcc_output
```

## Near-Term Extensions
- Frontend: parse local variable declarations, parameter lists, and richer expression grammars (multiplication/division, parentheses, comparisons).
- Backend: generate stack frames for locals/params, support register allocation for expression trees, and lower to object code via an assembler toolchain.
- Tooling: integrate formatting (`clang-format`), linting, and CI hooks once the pipeline stabilizes.
