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

**Status:** FIXED in `eae2a6a` — signaling is a non-local exit.

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

**Status:** FIXED — strict escape set with `\xHH`, `\uXXXX`, `\UXXXXXXXX`.

`read_string()` in `src/reader.c` accepts `\a \b \t \n \v \f \r \e \\ \"`,
1–3 octal digits (`\033` keeps working), and fixed-width hex escapes:
`\xHH` (2 digits), `\uXXXX` (4), `\UXXXXXXXX` (8). All numeric escapes
denote Unicode codepoints encoded as UTF-8, so the reader always
produces valid UTF-8 strings (octal `\377` no longer emits a raw byte).
Hex escapes are fixed-width — `"\x41bc"` reads as `"Abc"` — unlike the
greedy `#\x` character literals, because inside a string there is no
delimiter.

Unknown escapes are no longer silently read as text: they signal
`reader-error`, as do malformed hex escapes, surrogates (U+D800–U+DFFF)
and codepoints above U+10FFFF. Input ending mid-escape returns
`unclosed-input`, so the REPL continues the line. This also fixed an
out-of-bounds read when a string ended in a lone backslash at end of
input. The escape set is documented in `LANGUAGE_GUIDE.md`
(String Escapes).

**Tests:** `tests/basic/string_escapes.lisp` and
`tests/test_repl_completeness.c` (trailing backslash, truncated hex,
malformed escapes, fixed width).
