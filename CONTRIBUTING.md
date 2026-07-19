# Contributing to Ditty

Thanks for your interest in contributing to Ditty! This document covers everything you need to set up a development environment, run the test suite, follow the project's code conventions, and submit changes.

## Reporting issues

Use the issue trackers on [GitHub](https://github.com/chestso/ditty/issues) or [Codeberg](https://codeberg.org/chestso/ditty/issues) to report bugs or request features. Please include:

- Ditty version (`ditty -e "(print *ditty-version*)"`) and platform
- A minimal reproduction (a Lisp snippet or C example)
- Expected vs actual behavior

## Development setup

### Prerequisites

**Linux (Debian/Ubuntu):**

```bash
sudo apt-get install libgc-dev libpcre2-dev autoconf automake
```

**Linux (Fedora 41+):**

```bash
sudo dnf install gc-devel pcre2-devel autoconf automake
```

**macOS:**

```bash
brew install bdw-gc pcre2 autoconf automake
```

**Windows (MSYS2 UCRT64):** Same dependencies via `pacman`; ASan is auto-disabled since MinGW lacks libasan. `scripts/build-ucrt64.sh` is available.

### Build

This is an out-of-tree Autotools build. Always build in a `build/` subdirectory, never in the source root:

```bash
./autogen.sh
mkdir build && cd build
PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig:$HOME/.local/lib64/pkgconfig \
  ../configure --prefix=$HOME/.local
make -j$(nproc)
```

Build modes (passed to `../configure`):

- **Default**: ASan + UBSan enabled (`-O1 -g3 -fsanitize=address,undefined`) — the recommended mode for development
- **`--enable-debug`**: unsanitized debug (`-O0 -g3`)
- **`--enable-release`**: optimized (`-O3 -DNDEBUG`)

### Useful make targets

Run from the `build/` directory:

| Target         | Description                                                                  |
| -------------- | ---------------------------------------------------------------------------- |
| `make`         | Build `libditty.a` (includes Flare), `flare`, and `cli/ditty`                |
| `make check`   | Run the full test suite (Lisp + C tests in parallel)                         |
| `make format`  | clang-format on C, shfmt on shell, prettier on Markdown, lisp-fmt on Lisp    |
| `make install` | Install library, headers, REPL, `flare`, pkg-config files                    |
| `make bear`    | Produce `compile_commands.json` for clangd (runs clean + rebuild under bear) |

## Running tests

### Lisp tests

Lisp integration tests live in `tests/basic/`, `tests/advanced/`, and `tests/regression/`. They load `tests/test-helpers.lisp` for assertion macros (`assert-equal`, `assert-true`, `assert-false`, `assert-error`, `assert-nil`) and abort on first failure via `(error ...)`.

```bash
# From build/
make check                            # full suite
make check TESTS=basic/strings.lisp   # single test
```

To run a single Lisp test directly (the `DITTY_BIN` env var overrides which binary to use):

```bash
DITTY_LISP_PATH=/path/to/ditty/lisp ./tests/run-test.sh tests/basic/strings.lisp
```

The `run-test.sh` wrapper `cd`s to the project root so `(load "tests/...")` paths resolve correctly.

### C unit tests

C tests (`test_sf_kind`, `test_file_utils`, `test_token_type`, `test_color`, `test_style`, `test_lexer`, `test_lexer_commonmark`, `test_formatter`, `test_api`, `test_roundtrip`) are compiled as plain executables and linked against `libditty.a`. They use `flare_testkit.h` macros (`RUN_TEST`, `TEST_SUMMARY`, `ASSERT_EQ`, `ASSERT_STR_EQ`, `ASSERT_TRUE`, `ASSERT_NULL`, `ASSERT_NOT_NULL`) and `abort()` on first failure.

These run as part of `make check` alongside the Lisp tests.

### Emacs major mode tests

The Emacs major mode has ERT tests and byte-compilation targets (Emacs is not a build dependency):

```bash
make -C build/emacs byte-compile   # .el → .elc
make -C build/emacs test           # ERT test suite
make -C build/emacs elisp-format   # auto-indent .el files
```

## Code style

Run `make format` before submitting changes. The formatter is the source of truth.

### C conventions

- **Formatter**: `.clang-format` at project root. Key settings: 4-space indent, no tabs, Allman-style braces (brace on new line for functions/structs/enums, same line for control statements), `ColumnLimit: 0` (no line wrapping), pointer alignment right (`char *name`).
- **Disable clang-format** in a section: wrap with `/* clang-format off */` / `/* clang-format on */`.
- **Includes**: Never sorted (`.clang-format` sets `SortIncludes: Never`). `#include "builtins_internal.h"` is always first in `builtins_*.c` files.
- **Memory**: Boehm GC (`GC_malloc`, `GC_strdup`, `GC_malloc_atomic`). No manual `free()` for GC-managed allocations. Use `GC_MALLOC_ATOMIC` for pointer-free data (numbers, raw bytes).
- **Error returns**: Return `lisp_make_error("message")` or `lisp_make_error_with_stack("message", env)`. Check propagation via `should_propagate_error()` (type is `LISP_ERROR` and `!LISP_ERROR_CAUGHT`).
- **Unused parameter suppression**: `(void)env;` pattern in builtins that don't use the environment.
- **Argument checking**: Use `CHECK_ARGS_1(name)`, `CHECK_ARGS_2(name)`, `CHECK_ARGS_3(name)` macros from `builtins_internal.h`.
- **List building**: Use `LIST_APPEND(result, tail, item)` macro for efficient tail-insertion.
- **Windows compat**: `#ifdef _WIN32` guards for platform-specific code (timing, file paths, env vars).

### Lisp test conventions

- Tests load helpers: `(load "tests/test-helpers.lisp")`
- Assertions: `assert-equal`, `assert-true`, `assert-false`, `assert-error`, `assert-nil` (all abort on failure via `error`)
- Use `unwind-protect` for cleanup (see `tests/advanced/load.lisp`, `require_provide.lisp`)
- Tests run from project root via `tests/run-test.sh`

## Project architecture

### Repository layout

```
include/           Public headers (installed to $includedir/ditty/)
  lisp.h           Core API: types, env, eval, object creation, SPECIAL_FORMS X-macro
  lisp_value.h     Tagged-pointer accessors (LISP_TYPE, LISP_CAR, LISP_INT_VAL, etc.)
  utf8.h           UTF-8 utilities
  file_utils.h     Cross-platform file helpers
  ditty/highlight.h  Flare public API (lexers, styles, formatters, highlight)

src/               Library source → builds libditty.a
  lisp.c           Object constructors, symbol interning, lisp_init(), lisp_eval_string()
  reader.c         S-expression parser (lisp_read)
  eval.c           Evaluator with trampoline-based TCO, special form dispatch
  print.c          Print/princ/prin1 output
  env.c            Environment frames, profiling, call stack
  builtins.c       Dispatches to per-category registration, assigns SF docstrings
  builtins_*.c     Category-specific builtins (one file per doc/ category)
  builtins_internal.h  Shared header for all builtins_*.c files
  flare_*.c        Flare pipeline: lexer, style, formatter, highlight, color
  hash_table.c     Hash table implementation (used for env bindings, intern tables)
  utf8.c / regex.c / file_utils.c  Utility implementations

  docstrings.gen.h  AUTO-GENERATED from doc/*.md (by scripts/gen-docstrings.sh)
  ditty_version.h   AUTO-GENERATED from git state on every make

cli/               REPL executable
  main.c           Entry point; HAVE_BOBA gates TUI REPL vs dumb fallback
  repl_app.c/h     TUI REPL (requires boba library)
flare/             Standalone flare CLI (cat-like syntax highlighter)
lisp/              Lisp libraries shipped with ditty (lisp-fmt.lisp source formatter)
doc/               Per-function markdown docs (source of truth for runtime docstrings)
tests/             Test suite (basic/, advanced/, regression/, C unit tests)
emacs/             Emacs major mode (ditty-mode.el), not a build dependency
scripts/           Build/codegen helpers (gen-docstrings.sh, gen-builtin-ref.sh, etc.)
```

### Tagged pointer representation

`LispObject*` uses low-3-bit tagging (defined in `lisp_value.h`):

- `0b000`: heap pointer (real `LispObject` on the heap)
- `0b001`: fixnum (61-bit signed integer, no allocation)
- `0b010`: character (21-bit Unicode codepoint, no allocation)
- `0b011`: singleton (`NIL = 0x3`, `LISP_TRUE = 0xB`)

**Critical rule**: NEVER dereference a `LispObject*` directly (`obj->type`, `obj->value.X`). Tagged values will SIGSEGV. Always use `LISP_TYPE()`, `LISP_INT_VAL()`, `LISP_CAR()`, etc. The only exception is inside `lisp.c` constructors initializing a freshly `GC_malloc`'d heap object.

For the full object representation and GC rules, see [EMBEDDING.md](EMBEDDING.md).

### Evaluator flow

1. `lisp_eval()` is the public entry point. It calls `lisp_eval_internal()` with `in_tail_position=1`.
2. `lisp_eval_internal()` dispatches by type: self-evaluating literals, symbol lookup via `env_lookup()`, or cons-cell form evaluation via `eval_list()`.
3. `eval_list()` checks the first element against interned special form symbols (`sym_quote`, `sym_if`, etc.), then falls through to function application.
4. For tail calls, the evaluator returns a `LISP_TAIL_CALL` sentinel. The trampoline loop in `lisp_eval()` unwraps it by calling `apply()`, repeating until a real result emerges.
5. `lisp_apply()` is the public "call a function with evaluated args" API.

### Flare pipeline

Lex → Style → Format, each stage independently pluggable:

1. **Lexer** (`flare_lexer_ditty` or `flare_lexer_commonmark`): Tokenize source into `FlareToken[]`. The ditty lexer uses the live `Environment` to classify symbols (no hardcoded keyword list).
2. **Style** (`flare_style_dracula`, etc.): Maps `FlareTokenType` → `FlareStyleEntry` (RGB fg/bg, bold/italic). Style lookup walks a hierarchy (subcategory → category → default).
3. **Formatter** (`flare_formatter_terminal`): Produces ANSI-escaped string at 8/16/256/truecolor depth.
4. **`flare_highlight()`**: One-shot convenience that chains all three stages.

### Special forms X-macro

`SPECIAL_FORMS(X)` in `include/lisp.h` is the single source of truth. It defines `(c_name, lisp_name, SfKind)` triples and is expanded via X-macros to:

- Declare global symbol pointers (`sym_quote`, `sym_if`, etc.)
- Build the `sf_table[]` classification array
- Generate the `lisp_special_forms[]` name list
- Assign docstrings in `builtins.c`

**When adding a new special form**: Add a row to the `SPECIAL_FORMS` X-macro and implement the eval case in `eval_list()`.

### Builtins registration

Each `builtins_*.c` file defines `register_*_builtins(Environment *env)` which uses the `REGISTER(name, func)` macro to intern a symbol, look up its docstring from `docstrings.gen.h`, and bind the builtin function. `builtins.c` calls all registration functions and also assigns docstrings to special form symbols via `SET_SF_DOC`.

## Adding a builtin

1. Implement `LispObject *my_func(LispObject *args, Environment *env)` in the appropriate `src/builtins_*.c` (or create a new category file and call its `register_*_builtins` from `src/builtins.c`).
2. Add `REGISTER("my-func", my_func)` inside that file's `register_*_builtins` function.
3. Document it in the matching `doc/*.md` so the docstring lands in `(documentation 'my-func)` automatically.

See `src/builtins_internal.h` for the `REGISTER` macro, `CHECK_ARGS_n` arity guards, and the `LIST_APPEND` helper.

## Generated files

Two files are auto-generated during `make` and should never be edited directly:

| File                   | Source                     | Generator                   |
| ---------------------- | -------------------------- | --------------------------- |
| `src/docstrings.gen.h` | `doc/*.md`                 | `scripts/gen-docstrings.sh` |
| `src/ditty_version.h`  | git state / `version` file | `build-aux/git-version.sh`  |

To manually regenerate docstrings: `scripts/gen-docstrings.sh doc > src/docstrings.gen.h`

To regenerate `BUILTIN_REFERENCE.md`: `scripts/gen-builtin-ref.sh doc > BUILTIN_REFERENCE.md`

`BUILTIN_REFERENCE.md` is fully auto-generated from `doc/*.md`. Do not edit it by hand; update the per-function docs in `doc/` and rerun the generator.

`NEWS` is intentionally not maintained in this repository; do not update it.

## Common pitfalls

- **Build directory required**: Always `mkdir build && cd build` before `../configure`. Never configure in the source root.
- **clangd requires compile_commands.json**: Run `make bear` from the build directory. This cleans and rebuilds under `bear` to capture all compile flags.
- **LSP diagnostics false positives**: `lisp_value.h` and `builtins_internal.h` show errors in clangd until `docstrings.gen.h` and `ditty_version.h` exist (they're generated at build time). Build first, then restart LSP.
- **`#f` is `NIL`**: `lisp_make_boolean(0)` returns `NIL`, not a distinct false object. `LISP_TRUE` (`0xB`) is the only truthy boolean. This means `nil?` and `boolean?` have overlapping behavior for false.
- **Fixnum overflow**: Integers in the range `[-2^60, 2^60-1]` are tagged fixnums (no allocation). Outside that range, they fall back to heap-allocated `LISP_INTEGER`. Code must handle both paths (use `LISP_INT_VAL()` which decodes both).
- **Boba is optional**: The TUI REPL (`repl_app.c`) and `test_repl_app` only compile when `pkg-config` finds boba. Without it, `ditty` falls back to a simple non-interactive mode. Tests gating: `if HAVE_BOBA` in Makefile.am.
- **Docstring format**: Runtime docstrings are CommonMark markdown stored in `Symbol->docstring` and `LambdaInfo->docstring`. They're generated from `doc/*.md` where `` ## `func-name` `` headings delimit entries.
- **File stream EOL**: File streams auto-detect LF vs CRLF on read and transparently preserve on write. `write-string` translates `\n` to the stream's stored EOL. Don't manually handle `\r`.
- **ASan leak detection disabled**: `__asan_default_options()` returns `"detect_leaks=0"` because Boehm GC finalizers don't guarantee cleanup at exit, causing LSan false positives.
- **Test environment**: Lisp tests are executed via `run-test.sh` which `cd`s to the project root so `(load "tests/...")` paths resolve correctly. The `DITTY_BIN` env var points to the built binary.
- **Hash table key equality**: `hash_keys_equal()` determines matching. Strings and symbols use different comparison semantics.
- **Tail call must be the last expression**: The evaluator only marks the last expression in a body as `in_tail_position`. Intermediate expressions in `progn`/`let`/etc. are not TCO'd.
- **`require` and `*load-path*`**: `require` resolves libraries by consulting the Lisp variable `*load-path*` (in list order), then falls back to env-var search (`DITTY_LISP_PATH`, XDG) when `*load-path*` is nil. The CLI prepends the entry script's absolute directory to `*load-path*` so a self-contained app can `require` sibling libraries regardless of CWD. Mutating `*load-path*` at runtime (`set!`) is supported.

## Pull request process

1. **Branch**: Create a feature branch from `master`.
2. **Build & test**: Ensure `make check` passes and `make format` produces no changes.
3. **Docs**: If you add or change a builtin, update the corresponding `doc/*.md` file. Regenerate `BUILTIN_REFERENCE.md` with `scripts/gen-builtin-ref.sh doc > BUILTIN_REFERENCE.md`.
4. **Tests**: Add tests for new behavior — Lisp tests in `tests/basic/` or `tests/advanced/`, C unit tests where appropriate. Add new test files to `TESTS` and `EXTRA_DIST` in `tests/Makefile.am`.
5. **Commit**: Write clear commit messages that explain why a change was made, not just what changed. See the existing commit history for style.
6. **Push**: Push to your fork and open a pull request on [GitHub](https://github.com/chestso/ditty/pulls) or [Codeberg](https://codeberg.org/chestso/ditty/pulls).
