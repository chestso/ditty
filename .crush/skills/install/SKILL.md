---
name: install
description: Build, test, and install bloom-lisp and bloom-repl to ~/.local
user-invocable: true
disable-model-invocation: true
allowed-tools: Bash(./*), Bash(make *), Bash(PKG_CONFIG_PATH=*), Bash(cd ~/git && make *)
---

# Install bloom-lisp

Build the static library and `bloom-repl` binary, run the test suite,
and install everything to `~/.local`.

bloom-lisp depends on **Boehm GC** (`bdw-gc`) and **PCRE2**, which must
be installed via system packages. The optional **bloom-boba** dependency
enables the TUI REPL; without it, the REPL works in basic line mode.

## Option A: Monorepo build (recommended)

From `~/git`, this builds bloom-boba → bloom-lisp in dependency
order with matching ASan+UBSan flags:

```bash
cd ~/git && make bloom-lisp
```

This also runs `make check` and `make install`.

If the dependency projects are already installed and up-to-date, an
incremental rebuild from within bloom-lisp is faster:

```bash
cd build && make && make check && make install
```

## Option B: Build from within bloom-lisp

### If already configured

```bash
cd build && make && make check && make install
```

### Clean checkout path

```bash
./autogen.sh && \
mkdir -p build && cd build && \
PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig:$HOME/.local/lib64/pkgconfig \
  ../configure --prefix=$HOME/.local \
  CFLAGS='-O1 -g3 -fno-omit-frame-pointer -fno-common -fsanitize=address,undefined -fno-sanitize-recover=undefined' \
  LDFLAGS='-fsanitize=address,undefined' && \
make && make check && make install
```

### Debug build (no sanitizers)

```bash
./autogen.sh && \
mkdir -p build && cd build && \
PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig:$HOME/.local/lib64/pkgconfig \
  ../configure --prefix=$HOME/.local --enable-debug && \
make && make check && make install
```

## Build dependencies

bloom-lisp requires Boehm GC and PCRE2, discovered via pkg-config:

```bash
# Debian/Ubuntu
sudo apt install libgc-dev libpcre2-dev

# Fedora
sudo dnf install gc-devel pcre2-devel

# macOS
brew install bdw-gc pcre2
```

bloom-boba is optional. If installed, the REPL gets TUI support; if not,
it compiles without it.

## What gets installed

| Artifact | Destination |
| -------- | --------------------------------------- |
| `bloom-repl` | `~/.local/bin/bloom-repl` |
| `libbloomlisp.a` | `~/.local/lib/libbloomlisp.a` |
| `lisp.h` | `~/.local/include/lisp.h` |
| `lisp_value.h` | `~/.local/include/lisp_value.h` |
| `bloom-lisp.pc` | `~/.local/lib/pkgconfig/bloom-lisp.pc` |

## Other commands

| Task | Command |
| ---- | ------- |
| Incremental rebuild | `cd build && make` |
| Run tests only | `cd build && make check` |
| Format source code | `cd build && make format` |
| Generate compile_commands.json | `cd build && make bear` |
| Run single test | `./tests/run-test.sh tests/basic/strings.lisp` |

## Troubleshooting

- **`configure: error: Package 'bdw-gc' not found`** — Install Boehm GC
  development headers (see system packages above).
- **`configure: error: Package 'libpcre2-8' not found`** — Install PCRE2
  development headers.
- **Linker errors about `__asan_*` / `__ubsan_*`** — The installed
  `libbloomlisp.a` was built with ASan+UBSan, so downstream projects
  must be configured with matching `CFLAGS`/`LDFLAGS` containing
  `-fsanitize=address,undefined`.
- **LeakSanitizer in the REPL** — This is expected (Boehm GC never
  explicitly frees, so LSan reports at process exit). The REPL disables
  LSan at startup; your own embedder may need
  `LSAN_OPTIONS=exitcode=0` or `__lsan_disable()`.
- **clangd errors in lisp_value.h** — Expected without
  `compile_commands.json`. Run `make bear` from the build directory after
  a successful build.
