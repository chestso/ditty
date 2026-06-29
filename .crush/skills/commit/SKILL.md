---
name: commit
description: Format, test, and commit changes in ditty
user-invocable: true
allowed-tools: Bash(make *), Bash(git *)
---

# Commit Workflow for ditty

Format, build, test, and commit all current changes. Stop on any
failure.

## Pre-commit checklist (run in order, stop on failure)

### 1. Format

```bash
cd build && make format
```

If `Makefile` doesn't exist (tree not yet configured), skip this step.

### 2. Build

```bash
cd build && make
```

If the tree hasn't been configured yet, run the full bootstrap first:

```bash
./autogen.sh && \
mkdir -p build && cd build && \
PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig:$HOME/.local/lib64/pkgconfig \
  ../configure --prefix=$HOME/.local --enable-debug && \
make
```

### 3. Test

```bash
cd build && make check
```

All Lisp test files and the `test_sf_kind` C unit test must pass. If
any test fails, fix before continuing.

## Creating the commit

### Steps

1. Run `git status` (never use `-uall`), `git diff`, and `git log
   --oneline -5` to understand the changes and match existing commit
   message style
2. Stage the relevant changed files by name (do NOT use `git add -A` or
   `git add .`)
3. Commit with a clear imperative-mood message. Use a HEREDOC:

   ```bash
   git commit -m "$(cat <<'EOF'
   Message here
   EOF
   )"
   ```

4. Run `git status` to verify

### Commit message rules

- First line under 72 characters, imperative mood ("Fix...", "Add...",
  "Refactor...")
- Focus on **why** and **user-facing impact**, not implementation details
- No code identifiers or filenames in the summary line unless necessary
  for understanding
- Body (optional) explains reasoning, tradeoffs, or context; wrap at 72
  chars
- Do NOT use conventional commits (no `feat:`, `fix:`, etc.)
- Do NOT push unless explicitly asked

If $ARGUMENTS is provided, use it as guidance for the commit message.

## Quick reference

|| Step   | Command                                  |
| ------ | ---------------------------------------- |
| Format | `cd build && make format`                |
| Build  | `cd build && make`                       |
| Test   | `cd build && make check`                 |
| Commit | `git commit` (stage files by name)        |
