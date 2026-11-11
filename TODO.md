# Ditty TODO

## Error Handling

### unwind-protect + tail-call optimization

**Status:** FIXED — cleanups run on the way out, innermost first, once.

`unwind-protect` installs an `EvalFrame`; a signal raised in its body
longjmps to that frame, `frame_unwind()` restores the call-stack depth, the
cleanup forms run, and the original error object is re-raised. The pending
error travels in a local (not a global), so a cleanup that raises and catches
its own error cannot clobber it; a cleanup that raises replaces the pending
error. See `src/eval.c` (`eval_unwind_protect`) and `src/condition.c`.

**Tests:** `tests/regression/unwind_protect_tail_call.lisp` (8 cases) and
`tests/regression/condition_case_unwind.lisp`.

### condition-case resumes at the failing call, not at the handler

**Status:** FIXED — signaling is a non-local exit.

`condition-case` installs an `EvalFrame`; `lisp_raise()` longjmps to it, the
frame is popped, the handler is matched, and the handler body runs in a child
environment with the error bound to VAR. Nothing between the signal and the
handler resumes: the rest of the body, the rest of the failing call, and every
frame in between are abandoned. A handler that raises propagates to an outer
handler instead of recursing into its own.

```lisp
(defun boom () (error "x"))

(condition-case e (progn (boom) 'more) (error 'caught))          ; => caught
(condition-case e (if (boom) 'yes 'no) (error 'caught))          ; => caught
(condition-case e (list (boom)) (error 'caught))                 ; => caught
(condition-case e (let ((r (boom))) (error? r)) (error 'caught)) ; => caught
```

Error objects are data: returning one from a handler, `car`, `apply`, `eval`
or a `map` callback does not re-signal it. `(signal ERR)` re-raises one.

The `error?` checks in `lisp/terminal.lisp`'s event loop are gone; the loop's
outer `condition-case` catches and returns the error object.

**Tests:** `tests/regression/condition_case_unwind.lisp`,
`tests/regression/test_caught_error_suppress_print.sh`,
`tests/advanced/condition-system.lisp`.

## Reader

### Hex escapes in string literals

**Status:** OPEN - `\xHH` is not an escape; it is silently read as text.

`read_string()` in `src/reader.c` handles `\n \t \r \\ \"` and one-to-three
octal digits; any other backslash escape drops the backslash and keeps the
next character. So `"\x1b"` (four characters in the source) is the three
characters `x1b`, with no error and no warning:

```lisp
(string-length "\x1b")   ; => 3
```

- `LANGUAGE_GUIDE.md` never documents the string escape set, so neither the
  supported escapes nor the missing `\x` form is discoverable.
- This was live: the `tui-events` library wrote `"\x1b[?1003h"` and friends
  for mouse, focus, paste, clipboard and alt-screen control, all of which
  printed the literal text `x1b[...` to the terminal. `lisp/terminal.lisp`
  now uses octal `"\033"`.
- Decide: add `\xHH` (and `\uXXXX`?), reject unknown escapes as a reader
  error, or at minimum document the supported set. `\033` must keep working.

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
