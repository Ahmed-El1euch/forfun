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
