# Repository Guidelines

fungcc is a self-hosted ambition: a C compiler written in C. Keep the tree predictable so each compiler stage matures behind stable interfaces and thorough tests.

## Project Structure & Module Organization
- Core sources live in `src/`, grouped by pipeline stage: `src/frontend/lexer.c`, `src/frontend/parser.c`, `src/ir/`, and `src/backend/codegen.c`.
- Shared headers sit in `include/`; mirror directory names so `include/frontend/lexer.h` pairs with its `.c` implementation.
- Place executable entry points in `src/driver/` and sample programs in `samples/`.
- Tests belong in `tests/`, split into `tests/unit/` for stage-level checks and `tests/integration/` for end-to-end compilation fixtures. Golden outputs stay in `tests/fixtures/`.

Example skeleton:
```
mkdir -p src/frontend src/backend src/driver include/frontend tests/unit tests/integration tests/fixtures samples
```

## Build, Test, and Development Commands
- Configure once with `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`.
- Run incremental builds with `cmake --build build` or `ninja -C build` when enabled.
- Execute unit tests through `ctest --test-dir build --output-on-failure`.
- For quick experiments, compile a sample with `./build/fungcc samples/hello.c -o hello && ./hello`.

## Coding Style & Naming Conventions
- Use 4-space indentation, no tabs. Functions and variables follow `snake_case`; structs and enums use `PascalCase`.
- Keep one public function per header section and document preconditions with brief comment blocks.
- Run `clang-format -style=file` on all staged `.c`/`.h` files; add a `.clang-format` tuned for K&R braces and 100-column width.

## Testing Guidelines
- New front-end features require lexer/parser unit tests plus at least one compiled sample that proves emitted assembly runs.
- Name tests after behaviors (`lexer_handles_multiline_comments`) and store shared helpers in `tests/testlib/`.
- Hold coverage steady at ≥25% using `gcovr -r . build` after the suite.
- Record manual validation (e.g., running compiled binaries) in the PR description when automation cannot cover it.

## Progress Tracking & Planning
- Maintain a hierarchical roadmap in `todo.txt`; indent child tasks beneath their parent feature.
```
[ ] Frontend
  [ ] Lexer
    [ ] Recognize keywords
```
- Update the tree whenever scope changes: mark completed items with `[x]`, add follow-ups as indented siblings, and keep one feature per line.
- After finishing any task, log the date, scope, and validation commands in `PROGRESS.md`, then mirror the status change in `todo.txt`.
