# Errors

Error creation, signaling, and introspection.

Signaling and data are separate channels. `error`, `signal` and every runtime
failure _raise_: control transfers to the innermost matching `condition-case`
and nothing between the signal and the handler resumes. An error object,
once caught, is ordinary data: binding it, returning it from a handler,
putting it in a list, or passing it to `car`/`apply`/`eval` never re-signals.
Use `(signal ERR)` to re-raise one.

## `error?`

Test if value is an error object.

### Parameters

- `value` - Value to test

### Returns

`#t` if value is an error object, `#f` otherwise.

### Examples

```lisp
(define err nil)
(condition-case e
    (signal 'test-error "boom")
  (error (set! err e)))

(error? err)            ; => #t
(error? 42)             ; => #f
(error? "string")       ; => #f
```

### See Also

- `error-type` - Get error type symbol
- `error-message` - Get error message
- `signal` - Raise typed error

## `error-type`

Get error type symbol from error object.

### Parameters

- `error` - Error object

### Returns

Symbol representing the error type (e.g., `'simple-error`, `'division-by-zero`, `'file-error`).

### Examples

```lisp
(define err nil)
(condition-case e
    (signal 'my-error "test")
  (error (set! err e)))

(error-type err)        ; => my-error

; Different error types
(condition-case e
    (/ 10 0)
  (error (error-type e)))  ; => division-by-zero
```

### Errors

Returns error if argument is not an error object.

### See Also

- `error?` - Test if value is error
- `error-message` - Get error message
- `signal` - Raise typed error

## `error-message`

Get error message string from error object.

### Parameters

- `error` - Error object

### Returns

String containing the human-readable error message.

### Examples

```lisp
(define err nil)
(condition-case e
    (signal 'custom-error "Something went wrong")
  (error (set! err e)))

(error-message err)     ; => "Something went wrong"

; With division-by-zero error
(condition-case e
    (/ 10 0)
  (error (error-message e)))  ; => "Division by zero"
```

### Errors

Returns error if argument is not an error object.

### See Also

- `error-type` - Get error type
- `error-data` - Get error data
- `error-stack` - Get call stack

## `error-stack`

Get call stack trace from error object.

### Parameters

- `error` - Error object

### Returns

List of function names in the call stack, or `nil` if no stack trace.

### Examples

```lisp
(define outer (lambda (x) (middle x)))
(define middle (lambda (x) (inner x)))
(define inner (lambda (x) (/ x 0)))

(define err nil)
(condition-case e
    (outer 10)
  (error (set! err e)))

(error-stack err)       ; => ("/" "inner" "middle" "outer")
```

### Notes

Stack traces show the sequence of function calls from innermost (where error occurred) to outermost.

### Errors

Returns error if argument is not an error object.

### See Also

- `error-message` - Get error message
- `error-type` - Get error type

## `error-data`

Get error data payload from error object.

### Parameters

- `error` - Error object

### Returns

The data associated with the error, or `nil` if no data.

### Examples

```lisp
; Error with simple data
(define err nil)
(condition-case e
    (signal 'test-error "message")
  (error (set! err e)))

(error-data err)        ; => "message"

; Error with complex data
(condition-case e
    (signal 'file-error '("cannot open" "file.txt" 404))
  (error (error-data e)))  ; => ("cannot open" "file.txt" 404)
```

### Notes

Error data can be any Lisp value: string, number, list, etc.

### Errors

Returns error if argument is not an error object.

### See Also

- `error-message` - Get error message
- `error-type` - Get error type
- `signal` - Raise error with data

## `signal`

Raise an error (Emacs Lisp-style exception): control transfers to the
innermost matching `condition-case`.

### Parameters

- `error-type` - Symbol identifying the error type, **or** an error object
- `data` - Optional data payload (any Lisp value)

### Returns

Does not return (raises an error that must be caught with `condition-case`
or reported by the top level).

### Forms

- `(signal TYPE DATA)` — signal a new error of type `TYPE`.
- `(signal ERROR)` — re-raise that exact error object: same type, message,
  data and the stack captured where it was first raised. This is how a
  handler re-signals an error it has inspected.

### Examples

```lisp
; Simple error
(signal 'my-error "Something went wrong")
; => ERROR: [my-error] Something went wrong

; Error with structured data
(signal 'file-error '("cannot open" "file.txt" 404))
; => ERROR: [file-error] file-error: ("cannot open" "file.txt" 404)

; Catching errors
(condition-case e
    (signal 'division-by-zero "Cannot divide by zero")
  (division-by-zero "Caught division error")
  (error "Caught other error"))
; => "Caught division error"

; Re-signaling a caught error object keeps its identity
(condition-case outer
    (condition-case inner (error "boom") (error (signal inner)))
  (error (error-message outer)))   ; => "boom"
```

### Runtime Error Types

The runtime signals a small, Common Lisp-leaning set of condition types.
Each type has a parent chain ending at `error`; `condition-case` matches a
handler against the whole chain, so a handler for a parent type catches its
children. Clauses are tried in order and the first match wins.

| Type               | Parent             | Signaled when                                      |
| ------------------ | ------------------ | -------------------------------------------------- |
| `simple-error`     | `error`            | Generic error (the type `error` signals); fallback |
| `type-error`       | `error`            | Wrong-type operand, or an index out of bounds      |
| `program-error`    | `error`            | Arity mismatch or a malformed special form         |
| `unbound-variable` | `error`            | Undefined symbol (data: the symbol)                |
| `arithmetic-error` | `error`            | Non-finite result or a math domain violation       |
| `division-by-zero` | `arithmetic-error` | Integer division by zero                           |
| `file-error`       | `error`            | File I/O failure (data: the pathname)              |
| `stream-error`     | `error`            | Operation on a closed stream                       |
| `reader-error`     | `error`            | Reader/parse syntax failure                        |
| `unclosed-input`   | `reader-error`     | End-of-input inside an unclosed list/vector/string |
| `package-error`    | `error`            | Unknown package                                    |

`error` is the root of the hierarchy and the catch-all handler name. A
handler for `arithmetic-error` catches `division-by-zero`; a handler for
`reader-error` catches `unclosed-input`; `error` catches everything.

`unclosed-input` is what the reader signals for incomplete input. The REPL
uses it to decide whether to keep reading another line; user code can catch
it (or its parent `reader-error`) the same way:

```lisp
(condition-case e
  (load "snippet-with-unclosed-paren.lisp")
  (unclosed-input "snippet was incomplete")
  (error (concat "other error: " (error-message e))))
```

You can signal a custom type with any symbol. An unregistered type is
treated as a direct child of `error`: it matches a handler of that exact
name, or the `error` catch-all, but no other registered type.

### Notes

If `data` is a string, it's used as the error message. Otherwise, a message is constructed from the error type and data.

The stack trace (`error-stack`) is captured when the error is first raised,
not when it is constructed, so a re-signaled error keeps its original trace.

### See Also

- `error` - Raise generic error (convenience function)
- `condition-case` - Catch and handle errors
- `error-type` - Get error type from error object

## `error`

Raise a generic error with given message (convenience function).

### Parameters

- `message` - Error message string (or any value, will be converted to string)

### Returns

Does not return (raises error that must be caught with `condition-case`).

### Examples

```lisp
(error "Something went wrong")
; => ERROR: [simple-error] Something went wrong

; With non-string message
(error 404)
; => ERROR: [simple-error] 404

; Catching errors
(condition-case e
    (error "test error")
  (error (error-message e)))
; => "test error"
```

### Notes

This is equivalent to `(signal 'simple-error message)`. For typed errors, use `signal` instead.

### See Also

- `signal` - Raise typed error
- `condition-case` - Catch and handle errors
- `error-message` - Get error message from error object
