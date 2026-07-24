# TUI

Terminal user interface primitives. These functions provide the minimal
OS-level support needed for interactive terminal applications. Higher-level
features (keyboard decoding, mouse parsing, focus events, bracketed paste,
clipboard integration) are implemented in the `tui-events.lisp` library by
parsing the raw byte sequences produced by these primitives.

## `tui-events` library

The `tui-events.lisp` library is a proper Ditty module. It lives in its own
`tui-events` package, exports a public API, and registers itself via
`provide` so it can be loaded with `require`:

```lisp
(require 'tui-events)
(use-package 'tui-events)

(set-terminal-raw)
(let loop ((ev (tui-read-event read-byte)))
  (when ev
    (princ (format nil "~A~%" (tui-key-name ev)))
    (loop (tui-read-event read-byte))))
(restore-terminal)
```

Public symbols exported from the `tui-events` package:

- `tui-read-event`, `tui-read-escape-sequence`
- `tui-key-event?`, `tui-key-name`
- `tui-enable-mouse`, `tui-disable-mouse`, `tui-mouse-event?`
- `tui-enable-focus-events`, `tui-disable-focus-events`, `tui-focus-event?`
- `tui-enable-bracketed-paste`, `tui-disable-bracketed-paste`, `tui-paste-start?`, `tui-paste-end?`
- `tui-set-clipboard`, `tui-request-clipboard`

Internal helpers are not exported and can only be accessed via the
`tui-events:` package prefix (for example, `tui-events:tui-last`).

## `sleep`

Suspend execution for the given number of milliseconds.

### Parameters

- `milliseconds` - Non-negative integer duration

### Returns

`#t`

### Examples

```lisp
(sleep 100)                  ; pause for 100ms
(let ((start (current-time-ms)))
  (sleep 50)
  (- (current-time-ms) start)) ; => ~50
```

### Errors

Returns an error if the argument is not an integer or is negative.

## `set-terminal-raw`

Put the terminal into raw (non-canonical) mode. Disables line buffering and
echo so that single keypresses can be read without waiting for Enter.

### Parameters

None.

### Returns

`#t` on success. Returns `#t` even if stdin is not a tty (no-op).

### Examples

```lisp
(set-terminal-raw)
(restore-terminal)
```

## `restore-terminal`

Restore the terminal state saved by the most recent `set-terminal-raw` call.
This is a no-op if raw mode was never set or if stdin is not a tty.

### Parameters

None.

### Returns

`#t`

### Examples

```lisp
(condition-case err
  (progn
    (set-terminal-raw)
    (read-char))
  (t (restore-terminal)))
```

## `terminal-size`

Return the current terminal size as `(rows . cols)`.

### Parameters

None.

### Returns

A cons pair `(rows . cols)` on success, or `nil` if the size cannot be
determined.

### Examples

```lisp
(terminal-size)              ; => (24 . 80)
(car (terminal-size))          ; number of rows
(cdr (terminal-size))          ; number of columns
```

## `terminal-input-available-p`

Return `#t` if stdin has input ready to read without blocking.

### Parameters

None.

### Returns

`#t` if input is available, `nil` otherwise.

### Examples

```lisp
;; Non-blocking event loop
(let loop ()
  (when (terminal-input-available-p)
    (handle-key (read-char)))
  (sleep 10)
  (loop))
```

## `terminal-resized-p`

Return `#t` once after each terminal resize. The flag is cleared by calling
this function, so a second call returns `nil` unless another resize occurred.

### Parameters

None.

### Returns

`#t` if the terminal was resized since the last call, `nil` otherwise.

### Examples

```lisp
(when (terminal-resized-p)
  (redraw-screen))
```

## `read-byte`

Read a single byte from standard input.

### Parameters

None.

### Returns

An integer in the range 0-255, or `nil` on end-of-file.

### Examples

```lisp
(set-terminal-raw)
(define b (read-byte))     ; wait for a keypress
(restore-terminal)
```

## `read-byte-timeout`

Read a single byte from standard input, waiting at most the given number of
milliseconds.

### Parameters

- `milliseconds` - Maximum time to wait for input

### Returns

An integer in the range 0-255, or `nil` if the timeout expires or EOF is
reached.

### Examples

```lisp
;; Non-blocking event loop
(let loop ()
  (define b (read-byte-timeout 50))
  (when b (handle-input b))
  (loop))
```

### Errors

Returns an error if the timeout is not a non-negative integer.
