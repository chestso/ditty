# Ditty Lisp

An embeddable Lisp interpreter library and syntax highlighter written in C. This implementation follows traditional Lisp naming conventions and provides a REPL for testing and demonstration.

**Documentation:**

- **[Language Guide](LANGUAGE_GUIDE.md)** — Conceptual guide covering evaluation, special forms, macros, conventions, and more
- **[Built-in Function Reference](BUILTIN_REFERENCE.md)** — Auto-generated per-function documentation with parameters, return values, and examples
- **[Embedding Guide](EMBEDDING.md)** — C API reference, tagged-pointer object representation, and Boehm GC rules for embedders

## Features

### Core Language

- **Data Types**: Numbers, integers, booleans, strings (UTF-8), characters, symbols, keywords, lists, vectors, hash tables, lambdas, errors, regex, string ports, file streams
- **Special Forms**: `quote`, `quasiquote`, `if`, `define`, `set!`, `lambda`, `defmacro`, `let`/`let*`, `progn`, `do`, `cond`, `case`, `and`, `or`, `condition-case`, `unwind-protect`
- **Reader Syntax**: `pkg:symbol` qualified access (resolved at eval time via `env_lookup_in_package`)
- **Function Parameters**: Required, optional (`&optional`), and rest (`&rest`) parameters with `lambda`
- **Macros**: Code transformation with `defmacro`, quasiquote (`` ` ``), unquote (`,`), unquote-splicing (`,@`), and built-in `defun`, `defvar`, `defconst`, `defalias` macros
- **Functions**: Arithmetic, strings, lists, vectors, hash tables, regex (PCRE2), file I/O, string ports, filesystem, packages, profiling
- **Type Predicates**: `null?`, `atom?`, `pair?`, `list?`, `integer?`, `boolean?`, `number?`, `string?`, `char?`, `symbol?`, `keyword?`, `vector?`, `hash-table?`, `function?`, `callable?`, `error?`, `regex?`

See **[LANGUAGE_GUIDE.md](LANGUAGE_GUIDE.md)** for concepts and **[BUILTIN_REFERENCE.md](BUILTIN_REFERENCE.md)** for complete function listings and examples.

### Advanced Features

- **Package System**: Symbol-based namespace management with `in-package`, `current-package`, and `pkg:symbol` qualified access syntax
- **Library System**: Load-once library loading with `require`/`provide`, `export`/`use-package` for selective imports, and `DITTY_LISP_PATH` + XDG-based search paths
- **Condition System**: Emacs Lisp-style error handling with `signal`, `condition-case`, `unwind-protect`, and error introspection
- **Tail Call Optimization**: Trampoline-based tail recursion enables efficient recursive algorithms without stack overflow
- **Lexical Scoping**: First-class functions and closures with captured environments
- **Regex Support**: Full PCRE2 integration for advanced pattern matching with UTF-8 support
- **UTF-8 Support**: Full Unicode string support with codepoint-based operations
  - `length`, `substring`, and `string-ref` use codepoint indexing
  - Handles multi-byte characters correctly (CJK, emoji, etc.)
- **EOL-Aware File I/O**: File streams auto-detect their line-ending style (LF or CRLF) on open-for-read and transparently preserve it on write, Emacs Lisp style. `write-string` and `write-line` translate any embedded `\n` to the stream's stored EOL, so tools like source formatters can round-trip Windows files without any manual `\r` handling.
- **Memory Management**: Automatic garbage collection with Boehm GC
- **Semantic Classification**: The `lisp_sf_kind()` API classifies special forms by role (quote, define, control, macro-def, binding, body, exception) — used by Flare for accurate highlighting without hardcoded keyword lists

### Flare — Syntax Highlighting

Flare is a built-in syntax highlighting module (compiled into `libditty.a`) that produces ANSI terminal output at 8-color, 16-color, 256-color, and truecolor depths.

- **Runtime-driven classification**: Uses the ditty `Environment` as the single source of truth for symbol classification — no parallel hardcoded keyword lists
- **Two lexers**: Ditty Lisp and CommonMark/Markdown (fenced code blocks with `lisp`/`ditty` info strings are sub-lexed through the Lisp lexer)
- **Four built-in styles**: Dracula, Monokai, GitHub Dark, GitHub Light
- **Modular pipeline**: Lex → Style lookup → Format, each stage pluggable
- **`flare` CLI**: A `cat`-like tool that highlights source files to the terminal, with auto-detection of file language

## Quick Start

```lisp
; Arithmetic and variables
(define x 10)
(+ x 20)                             ; => 30

; Functions
(define square (lambda (n) (* n n)))
(square 5)                           ; => 25

; Named functions (defun macro)
(defun cube (n) (* n n n))
(cube 3)                             ; => 27

; Optional and rest parameters
(defun flex (a &optional b &rest rest)
  (list a b rest))
(flex 1 2 3 4 5)                     ; => (1 2 (3 4 5))

; Lists
(map (lambda (x) (* x 2)) '(1 2 3))  ; => (2 4 6)
(filter even? '(1 2 3 4 5 6))        ; => (2 4 6)

; Strings with Unicode
(length "Hello, 世界! 🌍")            ; => 12

; Conditionals
(if (> 10 5) "yes" "no")             ; => "yes"

; Error handling
(condition-case err
  (/ 1 0)
  (error (format nil "caught: ~A" (error-message err))))
```

**For full documentation, see [LANGUAGE_GUIDE.md](LANGUAGE_GUIDE.md)** (concepts) and [BUILTIN_REFERENCE.md](BUILTIN_REFERENCE.md) (function reference)

## Building

### Prerequisites

Install the required dependencies:

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

### Build from Source

Pure GNU Autotools — no wrapper script:

```bash
./autogen.sh
mkdir build && cd build
PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig:$HOME/.local/lib64/pkgconfig \
  ../configure --prefix=$HOME/.local
make -j$(nproc)
make check          # Run the test suite
make install        # Install library, headers, REPL, pkg-config file
```

Useful targets (from `build/`):

- `make` — build `libditty.a` (includes Flare) and `flare` and `cli/ditty`
- `make check` — run the test suite
- `make install` — install library, headers, REPL, `flare`, pkg-config files
- `make format` — clang-format on C, shfmt on shell, prettier on Markdown, lisp-fmt on Lisp
- `make bear` — produce `compile_commands.json` for clangd

The default build enables AddressSanitizer and UndefinedBehaviorSanitizer. Use `--enable-release` for an optimized production build or `--enable-debug` for an unsanitized debug build (-O0 -g3).

### Windows

Ditty builds natively on Windows using MSYS2/UCRT64. The `#ifdef _WIN32`
guards in the source handle Windows-specific APIs (file paths, timing,
environment variables). The interactive REPL requires
[boba](https://github.com/chestso/boba), which also builds natively
on Windows via MSYS2.

```bash
# In an MSYS2 UCRT64 shell:
./autogen.sh
mkdir build && cd build
../configure --prefix=$HOME/.local
make -j$(nproc)
make check
make install
```

### Emacs Major Mode

An Emacs major mode for ditty source (`ditty-mode.el`) lives in `emacs/` and is installed by `make install` to `$(datadir)/emacs/site-lisp/ditty/`. See the `;;; Commentary:` section at the top of `emacs/ditty-mode.el` for installation and usage instructions.

### Installation

```bash
sudo make install
```

This installs:

- `ditty` executable
- `flare` executable (syntax highlighting CLI)
- `libditty.a` static library (interpreter + syntax highlighting)
- Header files to `$(includedir)/ditty/` (including `highlight.h` for Flare)
- `ditty.pc` pkg-config file
- `lisp-fmt.lisp` Lisp source formatter

## Usage

### Command Line Options

```bash
ditty                      # Start interactive REPL
ditty -e "CODE"            # Execute CODE and exit
ditty FILE [FILE...]       # Execute FILE(s) and exit
ditty FILE -- [ARG...]     # Run FILE with script arguments
ditty -h, --help           # Show help message
```

### `flare` — Syntax Highlighting CLI

```bash
flare FILE                        # Highlight FILE to stdout (truecolor + dracula)
flare -f 256 -s monokai FILE      # 256-color monokai
flare -l commonmark README.md    # Highlight Markdown
flare -                           # Read stdin
flare --help                      # Show options
```

Options: `-f`/`--format` (`truecolor`, `256`, `16`, `8`), `-s`/`--style` (`dracula`, `monokai`, `github-dark`, `github-light`), `-l`/`--language` (`auto`, `ditty`, `commonmark`/`markdown`). Auto-detection selects by file extension.

### REPL Mode

```bash
./cli/ditty
```

The REPL uses [boba](https://github.com/chestso/boba) for inline terminal
rendering — no alternate screen, output flows into the terminal's own
scrollback. Syntax highlighting is applied to the input as you type via
the Flare highlighting engine.

Features:

- **Live syntax highlighting** - Input is highlighted as you type using Flare
- **Inline rendering** - No alt-screen takeover; output appears in normal scrollback
- **Multi-line editing** - Incomplete forms get continuation lines with auto-indent
- **Eval on Enter** - Enter evaluates when the form is complete; inserts a newline otherwise
- **Input history** - Arrow keys navigate previous inputs
- **Tab completion** - Completes symbol names from the environment
- **Ctrl+C** - Aborts the current edit and starts a fresh prompt
- **Ctrl+D** - Exits the REPL on empty input; deletes char under cursor otherwise

REPL Commands:

- `/quit` - Exit the REPL
- `/load <filename>` - Load and execute a Lisp file

### Command-Line Execution

```bash
./cli/ditty -e "(+ 1 2 3)"                               # => 6
./cli/ditty -e "(map (lambda (x) (* x 2)) '(1 2 3 4 5))" # => (2 4 6 8 10)
./cli/ditty -e '(concat "hello" " " "world")'            # => "hello world"
```

Exit code is 0 on success, 1 on error.

### Running Files

```bash
./cli/ditty script.lisp
```

## Embedding

The library is self-contained and can be integrated into any C project:

```c
#include <ditty/lisp.h>

int main() {
    Environment* env = lisp_init();

    LispObject* result = lisp_eval_string("(+ 1 2 3)", env);
    char* output = lisp_print(result);
    printf("%s\n", output);  // Prints: 6

    lisp_cleanup();
    return 0;
}
```

For the full C API reference, object representation, GC rules, and integration options (pkg-config, subproject, vendoring), see **[EMBEDDING.md](EMBEDDING.md)**.

## Authors

- Thomas Christensen

## License

[MIT License](COPYING)
