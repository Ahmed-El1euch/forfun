# Work Log

Track each completed step for fungcc so the history of decisions stays transparent.

## 2024-11-15
- Established project guidelines in `AGENTS.md` describing structure, tooling, and workflow.
- Captured upcoming milestones in `todo.txt` to guide compiler development.

- Created initial CMake project scaffolding with interface library stub and driver executable placeholder.
- Added canonical directory tree (src/frontend, include/, tests/unit, etc.) plus samples/ and docs/ roots.
- Verified build configuration conceptually; local `cmake` unavailable in environment, so compilation remains unvalidated.

## 2024-11-16
- Introduced token definitions and lexer implementation that handles keywords, identifiers, numbers, strings, operators, and comments.
- Rewired CMake targets to build `fungcc_core` as a static library and link the driver; rebuilt with `cmake -S . -B build` and `cmake --build build`.
- Exercised the driver demo via `./build/src/fungcc_driver` to visually inspect emitted tokens.
- Added lexer unit tests exercising whitespace/comments skipping and numeric/string literals; integrated executable `test_lexer` with CTest.
- Rebuilt and ran suite via `cmake --build build` and `ctest --test-dir build --output-on-failure`.
- Implemented recursive-descent parser for simple `int` functions with `return` statements, including AST models and cleanup helpers.
- Updated driver to parse and display function summaries; rebuilt via `cmake --build build` and ran `./build/src/fungcc_driver`.
- Added parser unit tests covering success, identifier returns, and error detection; executed with `ctest --test-dir build --output-on-failure`.
- Expanded parser coverage with multiple-function success path and failure scenarios; rebuilt (`cmake --build build`) and reran `ctest --test-dir build --output-on-failure`.
- Added parser negative tests for missing braces, unexpected keywords, and missing expressions; verified via `cmake --build build` and `ctest --test-dir build --output-on-failure`.
- Implemented minimal x86-64 code generator for literal return functions and integrated it into the driver demo; generated assembly with `./build/src/fungcc_driver`.
- Added backend unit coverage validating emitted assembly and failure paths; ran `cmake --build build` and `ctest --test-dir build --output-on-failure`.
- Assembled generated assembly via `cc build/fungcc_output.s -o build/fungcc_output` and confirmed exit status 42.
