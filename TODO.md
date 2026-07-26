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
