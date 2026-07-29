# Printing

Output functions (Common Lisp style).

## `princ`

Print object in human-readable form (without quotes on strings).

### Parameters

- `object` - Any object to print

### Returns

The object that was printed.

### Examples

```lisp
;; User-facing messages
(princ "Hello, world!")        ; Prints: Hello, world! (no quotes)
(princ "Line 1\nLine 2")      ; Prints: Line 1 (actual newline)
                               ;         Line 2

;; Escape sequences are interpreted
(princ "Tab:\there")          ; Prints: Tab:  here (actual tab)

;; Numbers and lists look natural
(princ 42)                     ; Prints: 42
(princ '(1 2 3))               ; Prints: (1 2 3)
```

### Notes

Output goes to stdout. Strings print without surrounding quotes.

Designed for producing human-readable output. The "c" in `princ` stands for
"character" — the function outputs characters as they would appear to a reader,
without escape sequences or quoting. Use this for messages, labels, and any
output intended for people to read directly. Contrast with `prin1`, which
outputs in a form that can be read back by the Lisp reader.

## `prin1`

Print object in readable representation (with quotes on strings).

### Parameters

- `object` - Any object to print

### Returns

The object that was printed.

### Examples

```lisp
;; Machine-readable output - can be read back
(prin1 "Hello, world!")        ; Prints: "Hello, world!" (with quotes)
(prin1 "Line 1\nLine 2")       ; Prints: "Line 1\nLine 2" (escape preserved)

;; Escape sequences are preserved, not interpreted
(prin1 "Tab:\there")           ; Prints: "Tab:\there" (literal backslash-t)

;; Numbers and lists in readable form
(prin1 42)                     ; Prints: 42
(prin1 '(1 2 3))               ; Prints: (1 2 3)

;; Contrast with princ: escape sequences
(princ "a\nb")                 ; Prints: a (newline) b
(prin1 "a\nb")                 ; Prints: "a\nb" (literal string)
```

### Notes

Output goes to stdout. Strings print with surrounding quotes.

Designed for producing machine-readable output. The output format is suitable
for reading back by the Lisp reader — strings are quoted, special characters
are escaped. The "1" in `prin1` indicates it uses the same format as the Lisp
reader's level 1 syntax. Use this when you need to serialize data that can be
read back later, or when you want to display the literal representation of an
object. Contrast with `princ`, which produces human-readable output without
quoting.

## `print`

Print object like prin1 but preceded by a newline and followed by a space.

### Parameters

- `object` - Any object to print

### Returns

The object that was printed.

### Examples

```lisp
;; Interactive debugging - each value on its own line
(do ((i 1 (+ i 1)))
    ((> i 3))
  (print i))
;; Prints:
;;
;; 1
;;
;; 2
;;
;; 3

;; The space after makes output separation clear in REPL
;; When REPL prints the return value, it doesn't run together
```

### Notes

Output goes to stdout. Outputs a newline before and a space after the object
(Common Lisp spec).

Designed for interactive use and debugging. The newline before ensures each
printed value starts on a fresh line, preventing output from running together.
The space after separates the value from subsequent output on the same line
(such as the REPL's prompt or return value). This convention makes it easy to
chain multiple `print` calls and see each value clearly separated. The output
format is the same as `prin1` (machine-readable with quoting).

## `format`

Formatted output with directives (Common Lisp style).

### Parameters

- `destination` - `nil` (return string) or `#t` (print to stdout)
- `format-string` - Format string with directives
- `args...` - Arguments to format

### Returns

Formatted string if `destination` is `nil`, otherwise `nil`.

### Format Directives

- `~A` or `~a` - Aesthetic (princ-style, no quotes)
- `~S` or `~s` - S-expression (prin1-style, with quotes)
- `~%` - Newline
- `~~` - Literal tilde (~)

### Examples

```lisp
(format nil "Hello, ~A!" "World")      ; => "Hello, World!"
(format nil "~A + ~A = ~A" 2 3 5)      ; => "2 + 3 = 5"
(format nil "String: ~S" "test")       ; => "String: \"test\""
(format nil "Line 1~%Line 2")          ; => "Line 1\nLine 2"
(format #t "Hello!~%")                 ; Prints: Hello! then newline
                                       ; => nil
```

## `terpri`

Print newline (terminate print).

### Returns

`nil`

### Examples

```lisp
(princ "Hello")
(terpri)                ; Prints newline
(princ "World")
```

### Notes

Useful for adding newlines between output.
