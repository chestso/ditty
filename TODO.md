# Ditty TODO

## Error Handling

### unwind-protect + tail-call optimization

**Status:** FIXED - cleanup forms now run correctly when errors occur in tail-recursive loops.

**Implementation:**

- Added `CleanupContext` struct and `cleanup_stack` field to `Environment`
- Cleanups are pushed/popped in `eval_unwind_protect()`
- Cleanups are inherited across tail calls (dynamic scope)
- `run_pending_cleanups()` helper drains the stack when errors escape
- Cleanups only run once: if a handler catches the error, cleanup runs on normal return; if unhandled, cleanup runs before error propagates

**Files changed:**

- `include/lisp.h` - Added `CleanupContext` struct, `cleanup_stack` to `Environment`
- `src/env.c` - Initialize `cleanup_stack` in `env_create()`
- `src/eval.c` - Added `run_pending_cleanups()`, modified `eval_unwind_protect()`, cleanup checks in trampolines
- `tests/regression/unwind_protect_tail_call.lisp` - 8 test cases

**Related:** See `ditty-bug-condition-case-tail-call.md` for the similar `condition-case` fix.

## Terminal input

### Decode UTF-8 in the event parser

**Status:** OPEN - `terminal-read-event` decodes input one byte at a time.

- Assemble UTF-8 continuation bytes into a single key whose name is the
  character, instead of reporting several one-byte "characters".
- Make the byte count of a key explicit in the event.

### Byte-accurate input on Windows

**Status:** OPEN - `read-byte` uses `_getch`, which truncates wide characters.

- Replace `_getch` with a `ReadFile` on the console handle (console input code
  page set to UTF-8) so `read-byte` behaves like POSIX.
- Handle redirected stdin, which `read-byte` currently ignores.
