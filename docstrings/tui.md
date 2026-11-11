# TUI

Terminal user interface primitives. These functions provide the minimal
OS-level support needed for interactive terminal applications: raw mode, byte
input, size, resize notification, and a blocking wait that an event loop can
build on. The higher-level features (keyboard decoding, mouse and focus events,
bracketed paste, clipboard integration, capability probes, and the event loop
itself) are implemented in the `terminal.lisp` library by parsing the raw byte
sequences produced by these primitives.

## `terminal` library

The `terminal.lisp` library is a proper Ditty module. It lives in its own
`terminal` package, exports a public API, and registers itself via `provide`
so it can be loaded with `require`:

```lisp
(require 'terminal)
(use-package 'terminal)

(terminal-event-loop
  (lambda (ev)
    (cond ((terminal-key-event? ev) (princ (terminal-key-name ev)))
          ((eq? (cdr (assoc 'type ev)) 'resize) (redraw))))
  '((:mouse . #t) (:tick-ms 100)))
```

### Design rules

- **No blocking waits, no polling.** The loop blocks in exactly one place,
  `terminal-wait`, which wakes on input, resize, or the next timer deadline.
  It never calls `sleep` and never spins on `terminal-input-available-p`.
- **Escape is not guessed with a timeout.** A lone Escape is detected because
  no continuation byte is already available, and the kitty keyboard protocol
  (flag 1, "disambiguate") makes Escape arrive as `ESC [ 27 u`. No timer is
  involved.
- **Queries are events too.** Inside the loop, capability replies arrive as
  `reply` events. The blocking `terminal-query` exists only for plain scripts.
- **Errors are values.** The loop returns the error object; it never prints an
  error and never exits the process.

### Public symbols exported from the `terminal` package

- `terminal-read-event`, `terminal-read-escape-sequence`
- `terminal-key-event?`, `terminal-key-name`, `terminal-key-byte`,
  `terminal-key-mods`
- `terminal-reply-event?`, `terminal-parse-csi-reply`
- `terminal-mouse-event?`, `terminal-focus-event?`
- `terminal-paste-start?`, `terminal-paste-end?`
- `terminal-enable-mouse`, `terminal-disable-mouse`
- `terminal-enable-focus-events`, `terminal-disable-focus-events`
- `terminal-enable-bracketed-paste`, `terminal-disable-bracketed-paste`
- `terminal-set-clipboard`, `terminal-request-clipboard`
- `terminal-enter-alt-screen`, `terminal-exit-alt-screen`,
  `terminal-hide-cursor`, `terminal-show-cursor`
- `terminal-push-keyboard-flags`, `terminal-pop-keyboard-flags`,
  `terminal-query-keyboard-flags`
- `terminal-call-with-raw`, `terminal-event-loop`
- `terminal-env-capabilities`, `terminal-capabilities-from-replies`,
  `terminal-capabilities`, `terminal-query`

Internal helpers are not exported and can only be accessed via the `terminal:`
package prefix (for example, `terminal:terminal-last`).

### Event shapes

Every event is an alist. The `type` field decides the shape:

| Type           | Fields                                                                                       |
| -------------- | -------------------------------------------------------------------------------------------- |
| `key`          | `key` (name string), `mods` (bitmask), `event` (`press`/`repeat`/`release`), optional `byte` |
| `mouse`        | `button`, `x`, `y`, `release`                                                                |
| `focus`        | `focus` (`in`/`out`)                                                                         |
| `paste-start`  | -                                                                                            |
| `paste-end`    | -                                                                                            |
| `reply`        | `prefix` (`"?"`, `">"`, `"<"`, `"="`, or `""`), `params` (list), `final`                     |
| `tick`         | -                                                                                            |
| `resize`       | `rows`, `cols`                                                                               |
| `capabilities` | `caps` (the alist from `terminal-capabilities-from-replies`)                                 |
| `unknown`      | `data` (the raw byte list)                                                                   |

Key events always carry `mods` and `event`; legacy bytes report `0` and
`press`. Names decode the common cases: printable input is the character
itself, control bytes are `C-x`, and named keys are `up`, `down`, `left`,
`right`, `home`, `end`, `pageup`, `pagedown`, `insert`, `delete`, `f1`…`f12`,
`return`, `newline`, `tab`, `backspace`, `escape`.

Under the kitty keyboard protocol, modified keys get a prefix so that a
modified key never looks like typed text: `CSI 97;5u` is `C-a`,
`CSI 120;3u` is `M-x`, and `CSI 97;7u` is `C-M-a`. The prefix order is ctrl,
alt, super; Shift is visible only in `mods`. `CSI 1;5A` and `CSI 3;2~` (legacy
functional keys with modifiers) keep their plain names and set `mods`.

CSI replies (bodies starting with `?`, `>`, or `=`, or ending in `t`) are
never keys: `ESC [ ? 64 ; 1 ; 4 c` arrives as a `reply` event with prefix
`"?"`, params `(64 1 4)`, and final `"c"`.

### The loop contract

`(terminal-event-loop HANDLER [OPTS])` calls `HANDLER` with one event per call
and returns `nil` after a clean stop or stdin EOF, or the error object.

`OPTS` is an alist:

- Session modes (`:mouse`, `:focus`, `:paste`, `:alt-screen`,
  `:hide-cursor`), all off by default, applied through
  `terminal-call-with-raw`.
- `:tick-ms` delivers `((type . tick))` at that interval.
- `:on-start` is a thunk called once after setup, before the first wait.
- `:probe` (default `#t`) sends the capability probe at start.
- `:keyboard` (default `#t`) pushes kitty keyboard flags 1 once the probe
  shows support, and pops them during cleanup.
- `:wait-fn`, `:read-byte-fn`, `:available-fn`, `:tty-p` are test/embedding
  seams that default to the builtins.

When the probe is enabled, the handler receives exactly one `capabilities`
event (built from the environment alone when stdin is not a terminal),
after the probe's replies are folded, and `reply` events after that arrive
raw. Return the symbol `stop` from the handler to end the loop.

`terminal-call-with-raw` tracks nesting with its own depth counter, so only
the outermost session calls `set-terminal-raw`/`restore-terminal`. Code that
calls `set-terminal-raw` directly bypasses the counter.

### Legacy Escape limit

A legacy (non-kitty) escape sequence is assumed to arrive in one `write`, so a
lone Escape is recognised by the absence of an immediately available
continuation byte, without a timer. A sequence split across two writes reads as
`escape` followed by text. Terminals write a sequence in one `write`, and the
kitty keyboard protocol removes the issue wherever it is supported.

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

## `terminal-pixel-size`

Return the size of the terminal text area in pixels as `(width . height)`.

### Parameters

None.

### Returns

A cons pair `(width . height)` on success, or `nil` if the size cannot be
determined. This is the pixel size of the whole text area, not of one
character cell: divide by the `terminal-size` columns and rows to get the cell
size. Many terminals do not report pixel dimensions (the values are 0), so
callers must handle `nil`.

### Examples

```lisp
;; Size of one character cell in pixels
(define px (terminal-pixel-size))
(when px
  (let ((size (terminal-size)))
    (list (floor (/ (car px) (cdr size)))     ; cell width
          (floor (/ (cdr px) (car size))))))  ; cell height
```

## `terminal-tty-p`

Return `#t` if both standard input and standard output are terminals.

### Parameters

None.

### Returns

`#t` when stdin and stdout are both terminals, `nil` otherwise (for example
when either stream is redirected to a file or a pipe). Use this to decide
whether terminal queries make sense: writing an escape sequence when stdout is
a file would corrupt it.

### Examples

```lisp
(when (terminal-tty-p)
  (princ "\033[?u"))          ; only ask a real terminal
```

## `terminal-wait`

Block until input arrives, the terminal is resized, or a timeout expires.

This is the single blocking point of an event loop: it never polls and never
sleeps, so the process uses no CPU while idle and reacts to input immediately.
It does not read any byte itself; after it returns `input`, call `read-byte`.

### Parameters

- `timeout-ms` - Optional non-negative integer maximum wait in milliseconds.
  Omitted or `nil` waits forever.

### Returns

One of the symbols:

- `resize` - the terminal was resized (pending resize flag cleared)
- `input` - stdin is readable; this includes end-of-file, which the following
  `read-byte` reports as `nil`
- `timeout` - the timeout expired with nothing to report

### Examples

```lisp
;; Event loop skeleton: no sleeping, no polling.
(set-terminal-raw)
(let loop ()
  (let ((why (terminal-wait)))
    (cond ((eq? why 'resize) (redraw))
          ((eq? why 'input)
           (let ((b (read-byte)))
             (when b (handle-byte b))))
          (#t nil))
    (loop)))
```

### Errors

Returns an error if the argument is neither a non-negative integer nor `nil`.

## `terminal-input-available-p`

Return `#t` if stdin has input ready to read without blocking.

### Parameters

None.

### Returns

`#t` if input is available, `nil` otherwise. Prefer `terminal-wait` in an
event loop: this function is a zero-timeout poll. On Windows, only keys that
can produce input count.

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

`#t` if the terminal was resized since the last call, `nil` otherwise. On
Windows, only real input and resize records count.

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

## Known limitations

- Keyboard input is decoded byte by byte: a multi-byte UTF-8 character arrives
  as several wrong one-byte "characters". Windows `read-byte` also truncates
  wide characters (`_getch` result `& 0xFF`).
- The Windows terminal code (`terminal-wait`, `terminal-tty-p`, the console
  input helper) has not been tested on Windows.
- On Windows, when stdin is not a console, `terminal-wait` returns `input`
  immediately, and `read-byte` still reads the console, not the redirected
  stdin.
- Legacy (non-kitty) Escape detection assumes a sequence arrives in one
  `write`; a split sequence reads as `escape` plus text.
- The capability probe only handles CSI replies; kitty graphics (APC) and
  XTVERSION (DCS) replies are ignored.
