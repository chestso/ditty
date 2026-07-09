# Built-in Function Reference

This reference is auto-generated from `doc/*.md`. Each section corresponds to a
documentation category, and each subsection documents a single built-in function,
macro, or variable. The source files in `doc/` are also used to generate runtime
docstrings via `scripts/gen-docstrings.sh`.

## Table of Contents

- [Arithmetic](#arithmetic)
  - `+`
  - `-`
  - `*`
  - `/`
  - `quotient`
  - `remainder`
  - `even?`
  - `odd?`
- [Characters](#characters)
  - `char?`
  - `char-code`
  - `code-char`
  - `char->string`
  - `string->char`
  - `char=?`
  - `char<?`
  - `char>?`
  - `char<=?`
  - `char>=?`
  - `char-upcase`
  - `char-downcase`
  - `char-alphabetic?`
  - `char-numeric?`
  - `char-whitespace?`
- [Comparison](#comparison)
  - `>`
  - `<`
  - `=`
  - `>=`
  - `<=`
  - `not`
  - `eq?`
  - `equal?`
  - `string=?`
- [Errors](#errors)
  - `error?`
  - `error-type`
  - `error-message`
  - `error-stack`
  - `error-data`
  - `signal`
  - `error`
- [File I/O](#file-io)
  - `open`
  - `close`
  - `read-line`
  - `write-line`
  - `write-string`
  - `stream-eol`
  - `read-sexp`
  - `read-json`
  - `read-file-raw`
  - `delete-file`
  - `load`
- [Filesystem](#filesystem)
  - `home-directory`
  - `expand-path`
  - `getenv`
  - `data-directory`
  - `config-directory`
  - `file-exists?`
  - `mkdir`
  - `file-is-directory?`
  - `delete-directory`
  - `system-type`
- [Functions](#functions)
  - `function-params`
  - `function-body`
  - `function-name`
  - `documentation`
  - `bound?`
  - `set-documentation!`
  - `eval`
  - `exit`
  - `quit`
- [Hash Tables](#hash-tables)
  - `make-hash-table`
  - `hash-ref`
  - `hash-set!`
  - `hash-remove!`
  - `hash-clear!`
  - `hash-count`
  - `hash-keys`
  - `hash-values`
  - `hash-entries`
- [Lists](#lists)
  - `car`
  - `cdr`
  - `caar`
  - `cadr`
  - `cdar`
  - `cddr`
  - `caddr`
  - `cadddr`
  - `first`
  - `second`
  - `third`
  - `fourth`
  - `rest`
  - `set-car!`
  - `set-cdr!`
  - `cons`
  - `list`
  - `length`
  - `list-ref`
  - `reverse`
  - `append`
  - `assoc`
  - `assq`
  - `assv`
  - `alist-get`
  - `member`
  - `memq`
  - `map`
  - `mapcar`
  - `filter`
  - `apply`
- [Packages](#packages)
  - `in-package`
  - `current-package`
  - `package-symbols`
  - `list-packages`
  - `package-save`
  - `provide`
  - `require`
  - `provided?`
  - `export`
  - `package-exports`
  - `use-package`
  - `*features*`
  - `*load-path*`
  - `*command-line-args*`
- [Printing](#printing)
  - `princ`
  - `prin1`
  - `print`
  - `format`
  - `terpri`
- [Regular Expressions](#regular-expressions)
  - `regex-compile`
  - `regex?`
  - `regex-match?`
  - `regex-find`
  - `regex-find-all`
  - `regex-extract`
  - `regex-replace`
  - `regex-replace-all`
  - `regex-split`
  - `regex-escape`
  - `regex-valid?`
- [Special Forms](#special-forms)
  - `quote`
  - `quasiquote`
  - `if`
  - `define`
  - `set!`
  - `lambda`
  - `defmacro`
  - `let`
  - `let*`
  - `progn`
  - `do`
  - `cond`
  - `case`
  - `and`
  - `or`
  - `condition-case`
  - `unwind-protect`
- [String Ports](#string-ports)
  - `open-input-string`
  - `port-peek-char`
  - `port-read-char`
  - `port-position`
  - `port-source`
  - `port-eof?`
  - `open-output-string`
  - `port-write-string`
  - `port-write-char`
  - `get-output-string`
  - `string-port?`
- [Strings](#strings)
  - `concat`
  - `substring`
  - `string-ref`
  - `split`
  - `join`
  - `number->string`
  - `string->number`
  - `string-contains?`
  - `string-index`
  - `string-match?`
  - `string-prefix?`
  - `string-replace`
  - `string-upcase`
  - `string-downcase`
  - `string-trim`
  - `string<?`
  - `string>?`
  - `string<=?`
  - `string>=?`
- [Symbols](#symbols)
  - `symbol->string`
  - `string->symbol`
  - `keyword-name`
- [Time and Profiling](#time-and-profiling)
  - `current-time-ms`
  - `profile-start`
  - `profile-stop`
  - `profile-report`
  - `profile-reset`
- [Type Predicates](#type-predicates)
  - `null?`
  - `atom?`
  - `pair?`
  - `integer?`
  - `boolean?`
  - `number?`
  - `vector?`
  - `hash-table?`
  - `string?`
  - `symbol?`
  - `keyword?`
  - `list?`
  - `function?`
  - `macro?`
  - `builtin?`
  - `callable?`
  - `regex?`
- [Vectors](#vectors)
  - `make-vector`
  - `vector-ref`
  - `vector-set!`
  - `vector-push!`
  - `vector-pop!`

---

## Arithmetic

Functions for numerical operations.

### `+`

Add numbers together.

#### Parameters

- `numbers...` - Zero or more numbers to add

#### Returns

Sum of all numbers. Returns `0` if no arguments provided.
If all arguments are integers, returns integer; otherwise returns float.

#### Examples

```lisp
(+)              ; => 0
(+ 1 2 3)        ; => 6 (integer)
(+ 10 5)         ; => 15 (integer)
(+ 1.5 2.5)      ; => 4.0 (float)
(+ 1 2.5)        ; => 3.5 (mixed integer/float -> float)
```

### `-`

Subtract numbers or negate a single number.

#### Parameters

- `number` - With one argument, returns the negation
- `minuend subtrahends...` - With multiple arguments, subtracts all subsequent numbers from the first

#### Returns

Result of subtraction. Returns integer if all arguments are integers; otherwise returns float.

#### Examples

```lisp
(- 5)            ; => -5 (unary negation)
(- 10 3)         ; => 7 (integer)
(- 10 3.5)       ; => 6.5 (float - mixed types)
(- 100 20 30)    ; => 50 (multiple subtractions)
```

### `*`

Multiply numbers together.

#### Parameters

- `numbers...` - Zero or more numbers to multiply

#### Returns

Product of all numbers. Returns `1` if no arguments provided.
If all arguments are integers, returns integer; otherwise returns float.

#### Examples

```lisp
(*)              ; => 1
(* 2 3 4)        ; => 24 (integer)
(* 3 4.0)        ; => 12.0 (float - mixed types)
(* 5 -2)         ; => -10
```

### `/`

Divide numbers or compute reciprocal.

#### Parameters

- `number` - With one argument, returns the reciprocal (1/number)
- `dividend divisors...` - With multiple arguments, divides the first number by all subsequent numbers

#### Returns

Result of division. **Always returns a float**, even for integer arguments.

#### Examples

```lisp
(/ 2)            ; => 0.5 (reciprocal: 1/2)
(/ 10 2)         ; => 5.0 (always float)
(/ 10 3)         ; => 3.333333...
(/ 100 2 5)      ; => 10.0 (multiple divisions: 100/2/5)
```

### `quotient`

Integer division - divide and truncate to integer.

#### Parameters

- `dividend` - The number to be divided (integer or float)
- `divisor` - The number to divide by (integer or float)

#### Returns

Integer result of division, truncated toward zero.

#### Examples

```lisp
(quotient 10 3)      ; => 3
(quotient 17 5)      ; => 3
(quotient -17 5)     ; => -3
(quotient 10.8 3.2)  ; => 3
```

#### See Also

- `remainder` - Get the remainder of integer division
- `/` - Regular division (returns float)

### `remainder`

Integer remainder (modulo operation).

#### Parameters

- `dividend` - The number to be divided (integer or float)
- `divisor` - The number to divide by (integer or float)

#### Returns

Integer remainder after division.

#### Examples

```lisp
(remainder 17 5)     ; => 2
(remainder 10 3)     ; => 1
(remainder 20 5)     ; => 0
(remainder -17 5)    ; => -2
```

#### See Also

- `quotient` - Integer division
- `/` - Regular division (returns float)

### `even?`

Test if number is even.

#### Parameters

- `n` - Number to test (integer or float)

#### Returns

`#t` if number is even, `#f` if odd.

#### Examples

```lisp
(even? 4)    ; => #t
(even? 5)    ; => #f
(even? 0)    ; => #t
(even? -2)   ; => #t
(even? 3.0)  ; => #f (converts to integer)
```

#### Notes

Converts floats to integers by truncation before testing.

#### See Also

- `odd?` - Test if number is odd
- `quotient` - Integer division
- `remainder` - Integer remainder

### `odd?`

Test if number is odd.

#### Parameters

- `n` - Number to test (integer or float)

#### Returns

`#t` if number is odd, `#f` if even.

#### Examples

```lisp
(odd? 5)     ; => #t
(odd? 4)     ; => #f
(odd? 1)     ; => #t
(odd? -3)    ; => #t
(odd? 2.0)   ; => #f (converts to integer)
```

#### Notes

Converts floats to integers by truncation before testing.

#### See Also

- `even?` - Test if number is even
- `quotient` - Integer division
- `remainder` - Integer remainder

---

## Characters

Character type predicates, comparisons, and conversions.

### `char?`

Check if a value is a character.

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is a character, `#f` otherwise.

#### Examples

```lisp
(char? #\a)          ; => #t
(char? #\space)      ; => #t
(char? "a")          ; => #f (string, not character)
(char? 65)           ; => #f (integer, not character)
```

### `char-code`

Return the Unicode code point of a character.

#### Parameters

- `char` - A character

#### Returns

Integer Unicode code point.

#### Examples

```lisp
(char-code #\a)      ; => 97
(char-code #\A)      ; => 65
(char-code #\space)  ; => 32
```

#### See Also

- `code-char` - Convert code point to character

### `code-char`

Convert a Unicode code point to a character.

#### Parameters

- `integer` - Unicode code point (integer)

#### Returns

The character with the given code point.

#### Examples

```lisp
(code-char 97)       ; => #\a
(code-char 65)       ; => #\A
(code-char 32)       ; => #\space
```

#### See Also

- `char-code` - Convert character to code point

### `char->string`

Convert a character to a one-character string.

#### Parameters

- `char` - A character

#### Returns

A string containing the single character.

#### Examples

```lisp
(char->string #\a)   ; => "a"
(char->string #\space) ; => " "
```

#### See Also

- `string->char` - Convert string to character

### `string->char`

Convert a one-character string to a character.

#### Parameters

- `string` - A string of length 1

#### Returns

The character from the string.

#### Examples

```lisp
(string->char "a")   ; => #\a
(string->char " ")   ; => #\space
```

#### See Also

- `char->string` - Convert character to string

### `char=?`

Test if two characters are equal.

#### Parameters

- `char1` - First character
- `char2` - Second character

#### Returns

`#t` if characters are equal, `#f` otherwise.

#### Examples

```lisp
(char=? #\a #\a)    ; => #t
(char=? #\a #\A)    ; => #f (case-sensitive)
(char=? #\space #\space) ; => #t
```

### `char<?`

Test if first character is less than second (by code point).

#### Parameters

- `char1` - First character
- `char2` - Second character

#### Returns

`#t` if `char1` has a lower code point than `char2`, `#f` otherwise.

#### Examples

```lisp
(char<? #\a #\b)    ; => #t
(char<? #\b #\a)    ; => #f
(char<? #\A #\a)    ; => #t (uppercase before lowercase in Unicode)
```

### `char>?`

Test if first character is greater than second (by code point).

#### Parameters

- `char1` - First character
- `char2` - Second character

#### Returns

`#t` if `char1` has a higher code point than `char2`, `#f` otherwise.

#### Examples

```lisp
(char>? #\b #\a)    ; => #t
(char>? #\a #\b)    ; => #f
(char>? #\a #\A)    ; => #t
```

### `char<=?`

Test if first character is less than or equal to second (by code point).

#### Parameters

- `char1` - First character
- `char2` - Second character

#### Returns

`#t` if `char1` code point is less than or equal to `char2`, `#f` otherwise.

#### Examples

```lisp
(char<=? #\a #\b)   ; => #t
(char<=? #\a #\a)   ; => #t
(char<=? #\b #\a)   ; => #f
```

### `char>=?`

Test if first character is greater than or equal to second (by code point).

#### Parameters

- `char1` - First character
- `char2` - Second character

#### Returns

`#t` if `char1` code point is greater than or equal to `char2`, `#f` otherwise.

#### Examples

```lisp
(char>=? #\b #\a)   ; => #t
(char>=? #\a #\a)   ; => #t
(char>=? #\a #\b)   ; => #f
```

### `char-upcase`

Convert a character to uppercase.

#### Parameters

- `char` - A character

#### Returns

The uppercase version of the character, or the character unchanged if not a letter.

#### Examples

```lisp
(char-upcase #\a)    ; => #\A
(char-upcase #\A)    ; => #\A
(char-upcase #\1)    ; => #\1 (unchanged)
```

#### See Also

- `char-downcase` - Convert to lowercase

### `char-downcase`

Convert a character to lowercase.

#### Parameters

- `char` - A character

#### Returns

The lowercase version of the character, or the character unchanged if not a letter.

#### Examples

```lisp
(char-downcase #\A)  ; => #\a
(char-downcase #\a)  ; => #\a
(char-downcase #\1)  ; => #\1 (unchanged)
```

#### See Also

- `char-upcase` - Convert to uppercase

### `char-alphabetic?`

Test if a character is alphabetic.

#### Parameters

- `char` - A character

#### Returns

`#t` if the character is a letter, `#f` otherwise.

#### Examples

```lisp
(char-alphabetic? #\a)    ; => #t
(char-alphabetic? #\Z)    ; => #t
(char-alphabetic? #\1)    ; => #f
(char-alphabetic? #\space) ; => #f
```

### `char-numeric?`

Test if a character is numeric (digit).

#### Parameters

- `char` - A character

#### Returns

`#t` if the character is a digit (0-9), `#f` otherwise.

#### Examples

```lisp
(char-numeric? #\5)       ; => #t
(char-numeric? #\0)       ; => #t
(char-numeric? #\a)       ; => #f
(char-numeric? #\space)   ; => #f
```

### `char-whitespace?`

Test if a character is whitespace.

#### Parameters

- `char` - A character

#### Returns

`#t` if the character is whitespace (space, tab, newline, etc.), `#f` otherwise.

#### Examples

```lisp
(char-whitespace? #\space)    ; => #t
(char-whitespace? #\newline)  ; => #t
(char-whitespace? #\tab)      ; => #t
(char-whitespace? #\a)        ; => #f
```

---

## Comparison

Comparison and equality predicates.

### `>`

Test if numbers are in strictly decreasing order.

#### Parameters

- `n1 n2 ...` - Two or more numbers to compare

#### Returns

`#t` if each number is strictly greater than the next, `#f` otherwise.

#### Examples

```lisp
(> 5 3)              ; => #t
(> 10 5 2)           ; => #t
(> 5 5)              ; => #f (not strictly greater)
(> 3 5)              ; => #f
```

### `<`

Test if numbers are in strictly increasing order.

#### Parameters

- `n1 n2 ...` - Two or more numbers to compare

#### Returns

`#t` if each number is strictly less than the next, `#f` otherwise.

#### Examples

```lisp
(< 3 5)              ; => #t
(< 1 2 3 4)          ; => #t
(< 5 5)              ; => #f (not strictly less)
(< 5 3)              ; => #f
```

### `=`

Test if numbers are all equal.

#### Parameters

- `n1 n2 ...` - Two or more numbers to compare

#### Returns

`#t` if all numbers are equal, `#f` otherwise. Works with integers and floats.

#### Examples

```lisp
(= 5 5)              ; => #t
(= 5 5 5)            ; => #t
(= 5.0 5)            ; => #t (cross-type comparison)
(= 5 3)              ; => #f
```

### `>=`

Test if numbers are in non-increasing order (greater than or equal).

#### Parameters

- `n1 n2 ...` - Two or more numbers to compare

#### Returns

`#t` if each number is greater than or equal to the next, `#f` otherwise.

#### Examples

```lisp
(>= 5 3)             ; => #t
(>= 5 5)             ; => #t (equal is OK)
(>= 10 5 5 2)        ; => #t
(>= 3 5)             ; => #f
```

### `<=`

Test if numbers are in non-decreasing order (less than or equal).

#### Parameters

- `n1 n2 ...` - Two or more numbers to compare

#### Returns

`#t` if each number is less than or equal to the next, `#f` otherwise.

#### Examples

```lisp
(<= 3 5)             ; => #t
(<= 5 5)             ; => #t (equal is OK)
(<= 1 2 2 3)         ; => #t
(<= 5 3)             ; => #f
```

### `not`

Logical negation.

#### Parameters

- `value` - Value to negate

#### Returns

`#t` if value is falsy (nil), `#f` if value is truthy.

#### Examples

```lisp
(not nil)            ; => #t
(not #f)             ; => #t
(not #t)             ; => #f
(not 0)              ; => #f (0 is truthy!)
(not "")             ; => #f (empty string is truthy!)
```

#### Notes

Only `nil` (and `#f`) are falsy in ditty

### `eq?`

Test pointer equality (same object in memory).

#### Parameters

- `obj1` - First object
- `obj2` - Second object

#### Returns

`#t` if both objects are the same object in memory, `#f` otherwise.

#### Examples

```lisp
(eq? 'foo 'foo)          ; => #t (symbols are interned)
(eq? '(1 2) '(1 2))      ; => #f (different list objects)
(define x '(1 2))
(define y x)
(eq? x y)                ; => #t (same object)
```

#### Notes

- Fast pointer comparison (raw `==` on tagged pointers)
- Reliable for symbols (always interned — same name = same pointer)
- Reliable for small integers (fixnums are tagged immediates — same value = same bit pattern)
- NOT reliable for floats (heap-allocated — each construction is a distinct object)
- NOT reliable for strings (heap-allocated — each literal is a distinct object)
- Use `equal?` for structural equality

### `equal?`

Test deep structural equality.

#### Parameters

- `obj1` - First object
- `obj2` - Second object

#### Returns

`#t` if objects have the same structure and values, `#f` otherwise.

#### Examples

```lisp
(equal? '(1 2 3) '(1 2 3))      ; => #t (same values)
(equal? "abc" "abc")            ; => #t (same string)
(equal? '(1 (2 3)) '(1 (2 3)))  ; => #t (nested lists)
(equal? 1 1.0)                  ; => #f (different types)
```

#### Notes

- Recursive comparison of lists, vectors, hash tables
- Default choice for "are these values the same?" checks

### `string=?`

Test string equality.

#### Parameters

- `str1` - First string
- `str2` - Second string

#### Returns

`#t` if strings have identical character sequences, `#f` otherwise.

#### Examples

```lisp
(string=? "foo" "foo")     ; => #t
(string=? "foo" "bar")     ; => #f
(string=? "" "")           ; => #t
```

### Practical Usage Guide

- **`eq?`** - For symbols, checking if two variables point to same object
- **`=`** - For numeric comparisons (supports integers and floats)
- **`string=?`** - For string equality
- **`equal?`** - Default choice for "are these values the same?" checks (lists, vectors, hash tables, any data structures)

---

## Errors

Error creation, signaling, and introspection.

### `error?`

Test if value is an error object.

#### Parameters

- `value` - Value to test

#### Returns

`#t` if value is an error object, `#f` otherwise.

#### Examples

```lisp
(define err nil)
(condition-case e
    (signal 'test-error "boom")
  (error (set! err e)))

(error? err)            ; => #t
(error? 42)             ; => #f
(error? "string")       ; => #f
```

#### See Also

- `error-type` - Get error type symbol
- `error-message` - Get error message
- `signal` - Raise typed error

### `error-type`

Get error type symbol from error object.

#### Parameters

- `error` - Error object

#### Returns

Symbol representing the error type (e.g., `'error`, `'division-by-zero`, `'file-error`).

#### Examples

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

#### Errors

Returns error if argument is not an error object.

#### See Also

- `error?` - Test if value is error
- `error-message` - Get error message
- `signal` - Raise typed error

### `error-message`

Get error message string from error object.

#### Parameters

- `error` - Error object

#### Returns

String containing the human-readable error message.

#### Examples

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

#### Errors

Returns error if argument is not an error object.

#### See Also

- `error-type` - Get error type
- `error-data` - Get error data
- `error-stack` - Get call stack

### `error-stack`

Get call stack trace from error object.

#### Parameters

- `error` - Error object

#### Returns

List of function names in the call stack, or `nil` if no stack trace.

#### Examples

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

#### Notes

Stack traces show the sequence of function calls from innermost (where error occurred) to outermost.

#### Errors

Returns error if argument is not an error object.

#### See Also

- `error-message` - Get error message
- `error-type` - Get error type

### `error-data`

Get error data payload from error object.

#### Parameters

- `error` - Error object

#### Returns

The data associated with the error, or `nil` if no data.

#### Examples

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

#### Notes

Error data can be any Lisp value: string, number, list, etc.

#### Errors

Returns error if argument is not an error object.

#### See Also

- `error-message` - Get error message
- `error-type` - Get error type
- `signal` - Raise error with data

### `signal`

Raise a typed error (Emacs Lisp-style exception).

#### Parameters

- `error-type` - Symbol identifying the error type
- `data` - Optional data payload (any Lisp value)

#### Returns

Does not return (raises error that must be caught with `condition-case`).

#### Examples

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
```

#### Common Error Types

- `'error` - Generic error (catch-all)
- `'division-by-zero` - Division by zero
- `'wrong-type-argument` - Wrong type passed to function
- `'wrong-number-of-arguments` - Arity mismatch
- `'void-variable` - Undefined symbol
- `'file-error` - File I/O errors
- `'range-error` - Out of bounds access
- `'unclosed-input` - Reader hit end-of-input inside an unclosed list or vector (signaled by `read-sexp` and the REPL parser). The REPL uses this to decide whether to keep reading another line; user code can catch it the same way:

```lisp
(condition-case e
  (load "snippet-with-unclosed-paren.lisp")
  (unclosed-input "snippet was incomplete")
  (error (concat "other error: " (error-message e))))
```

You can define custom error types using any symbol:

#### Notes

If `data` is a string, it's used as the error message. Otherwise, a message is constructed from the error type and data.

#### See Also

- `error` - Raise generic error (convenience function)
- `condition-case` - Catch and handle errors
- `error-type` - Get error type from error object

### `error`

Raise a generic error with given message (convenience function).

#### Parameters

- `message` - Error message string (or any value, will be converted to string)

#### Returns

Does not return (raises error that must be caught with `condition-case`).

#### Examples

```lisp
(error "Something went wrong")
; => ERROR: [error] Something went wrong

; With non-string message
(error 404)
; => ERROR: [error] 404

; Catching errors
(condition-case e
    (error "test error")
  (error (error-message e)))
; => "test error"
```

#### Notes

This is equivalent to `(signal 'error message)`. For typed errors, use `signal` instead.

#### See Also

- `signal` - Raise typed error
- `condition-case` - Catch and handle errors
- `error-message` - Get error message from error object

---

## File I/O

File reading, writing, and loading.

### `open`

Open a file for reading or writing.

#### Parameters

- `filename` - Path to file (string)
- `mode` - Optional mode: `"r"` (read, default), `"w"` (write), `"a"` (append)
- `eol` - Optional explicit end-of-line style: `"\n"` or `"\r\n"`.
  If omitted, read-mode streams auto-detect the EOL by peeking up to
  4 KB of the file; write-mode streams default to `"\n"`.

#### Returns

File stream object for use with `read-line`, `write-line`, `write-string`,
`close`, etc. Returns error if file cannot be opened.

#### Examples

```lisp
(define f (open "test.txt" "r"))          ; Open for reading, auto-detect EOL
(define f (open "out.txt" "w"))           ; Open for writing, defaults to LF
(define f (open "out.txt" "w" "\r\n"))    ; Open for writing with CRLF
(define f (open "log.txt" "a"))           ; Open for appending
```

#### Notes

- Always close files with `(close stream)` when done. Files are not auto-closed.
- The EOL style is stored on the stream and drives `write-line` and
  `write-string`: both translate any `\n` in their input to the stored
  EOL, Emacs-Lisp-style.

#### See Also

- `close` - Close file stream
- `read-line` - Read line from file
- `write-line` - Write line to file
- `write-string` - Write string verbatim (with EOL translation)
- `stream-eol` - Query the stream's current EOL style

### `close`

Close a file stream.

#### Parameters

- `stream` - File stream object from `open`

#### Returns

`nil`

#### Examples

```lisp
(define f (open "test.txt" "r"))
(read-line f)
(close f)
```

#### Notes

Always close files when done. Closing an already-closed stream is safe (no-op).

#### See Also

- `open` - Open file stream

### `read-line`

Read one line from file stream.

#### Parameters

- `stream` - File stream object from `open`

#### Returns

String containing the line (without newline), or `nil` at end-of-file.

#### Examples

```lisp
(define f (open "test.txt" "r"))
(define line (read-line f))  ; => "first line"
(read-line f)                ; => "second line"
(read-line f)                ; => nil (EOF)
(close f)
```

#### Notes

- Handles Unix (LF), Windows (CRLF), and old Mac (CR) line endings
- Returns `nil` at end-of-file
- Does not include the newline character in returned string

#### See Also

- `write-line` - Write line to file
- `read-sexp` - Read S-expressions from file

### `write-line`

Write string to file stream with a trailing newline.

#### Parameters

- `stream` - File stream object from `open`
- `text` - String to write

#### Returns

The text that was written.

#### Examples

```lisp
(define f (open "out.txt" "w"))
(write-line f "first line")
(write-line f "second line")
(close f)
```

#### Notes

- Automatically appends the stream's EOL after the text. This is
  `"\n"` by default for write-mode streams, `"\r\n"` for CRLF streams
  (auto-detected on read, or passed explicitly to `open`).
- Any `\n` inside `text` is also translated to the stream's EOL.
- Flushes output immediately.

#### See Also

- `read-line` - Read line from file
- `write-string` - Write without appending a terminator

### `write-string`

Write a string to a file stream verbatim (no trailing terminator).
Any `\n` in the string is translated to the stream's stored EOL.

#### Parameters

- `stream` - File stream object from `open`
- `text` - String to write

#### Returns

The text that was written.

#### Examples

```lisp
;; Round-trip a CRLF file without double newlines at EOF:
(let* ((in (open "src.txt" "r"))           ; auto-detects CRLF
       (eol (stream-eol in)))
  (close in)
  (let ((out (open "dest.txt" "w" eol)))   ; inherit the style
    (write-string out "line1\nline2\n")     ; "\n" becomes "\r\n"
    (close out)))                           ; file ends with exactly one \r\n
```

#### Notes

- Unlike `write-line`, does **not** append any terminator. Pair it with
  content that already ends in `\n` if you want a trailing EOL.
- Any `\n` bytes inside the text are translated to the stream's EOL,
  so an LF-internal string round-trips cleanly through a CRLF stream.
- Flushes output immediately.

#### See Also

- `write-line` - Write a line with a trailing terminator
- `stream-eol` - Query the stream's EOL style
- `open` - Set the EOL style explicitly

### `stream-eol`

Return the EOL style stored on a file stream.

#### Parameters

- `stream` - File stream object from `open`

#### Returns

`"\n"` for LF streams, `"\r\n"` for CRLF streams.

#### Examples

```lisp
(define f (open "windows.txt" "r"))
(stream-eol f)  ; => "\r\n"  (auto-detected)
(close f)

(define g (open "unix.txt" "r"))
(stream-eol g)  ; => "\n"
(close g)
```

#### Notes

- On read-mode streams the EOL is auto-detected at `open` time by
  peeking the first 4 KB of the file for a `\r\n` pair.
- On write-mode streams the EOL is either `"\n"` (default) or the
  explicit string passed as the third argument to `open`.
- The returned value is safe to pass as the third argument of `open`
  when creating a matching write stream — this is the normal pattern
  for tools that want to round-trip a file's line-ending style.

#### See Also

- `open` - Set the EOL explicitly via the third argument
- `write-line` / `write-string` - Both respect the stream's EOL

### `read-sexp`

Read S-expressions from file.

#### Parameters

- `stream-or-filename` - File stream or filename string

#### Returns

Single S-expression if file contains one, or list of all S-expressions.
Returns error if parse fails.

#### Examples

```lisp
; File contains: (+ 1 2) (* 3 4)
(read-sexp "math.lisp")  ; => ((+ 1 2) (* 3 4))

; File contains: (define x 10)
(read-sexp "single.lisp")  ; => (define x 10)
```

#### Notes

- Accepts filename string or open file stream
- Auto-closes file if filename provided
- Returns single expr if file has one, list if multiple

#### See Also

- `load` - Load and evaluate file
- `read-json` - Read JSON from file

### `read-json`

Read JSON from file (basic parser).

#### Parameters

- `stream-or-filename` - File stream or filename string

#### Returns

Lisp object representing JSON:

- JSON object → hash table
- JSON array → list (NOT YET SUPPORTED)
- JSON string → string
- JSON number → integer or float
- JSON true → `#t`
- JSON false/null → `nil`

#### Examples

```lisp
; File contains: {"name": "Alice", "age": 30}
(define obj (read-json "data.json"))
(hash-ref obj "name")  ; => "Alice"
(hash-ref obj "age")   ; => 30
```

#### Limitations

- Basic implementation: nested objects and arrays not yet supported
- Only top-level objects work reliably

#### See Also

- `read-sexp` - Read Lisp S-expressions from file

### `read-file-raw`

Read an entire file into a string verbatim, preserving every byte
including carriage returns.

#### Parameters

- `filename` - Path to file (string)

#### Returns

String containing the exact file contents (no newline normalisation).
Returns error if file cannot be opened.

#### Examples

```lisp
(read-file-raw "unix.txt")     ; => "line1\nline2\n"
(read-file-raw "windows.txt")  ; => "line1\r\nline2\r\n"
```

#### Notes

- Unlike `read-line`, this preserves CRLF sequences — useful for tools
  that need to detect or round-trip the original line-ending style.
- Opens the file in binary mode; suitable for text files only (the
  return type is a Lisp string).
- Reads the entire file into memory; avoid on very large files.

#### See Also

- `read-line` - Read line by line with newline normalisation
- `read-sexp` - Read Lisp S-expressions

### `delete-file`

Delete a file from filesystem. Refuses to delete directories — use `delete-directory` instead.

#### Parameters

- `filename` - Path to file (string)

#### Returns

`nil` on success, error if deletion fails.

#### Examples

```lisp
(delete-file "temp.txt")  ; => nil
(delete-file "/no/such/file")  ; => Error: ...
```

#### Notes

Permanent operation - deleted files cannot be recovered.
If the path is a directory, an error is raised directing you to `delete-directory`.

#### Errors

Returns error with system message if deletion fails (permission denied, file not found, is a directory, etc.)

#### See Also

- `delete-directory` - Delete a directory

### `load`

Load and evaluate Lisp file.

#### Parameters

- `filename` - Path to Lisp file (string)

#### Returns

Result of last expression in file, or error if load fails.

#### Examples

```lisp
(load "utils.lisp")       ; Load utility functions
(load "~/.lisprc")         ; Load config file
```

#### Notes

- Evaluates all expressions in file sequentially
- Returns value of last expression
- File must be valid Lisp syntax
- Definitions are added to current environment
- Use `expand-path` to handle `~/` in paths

#### See Also

- `read-sexp` - Read without evaluating
- `expand-path` - Expand `~/ paths

---

## Filesystem

Path expansion, environment variables, and filesystem operations.

### `home-directory`

Get the user's home directory path.

#### Parameters

None.

#### Returns

String with home directory path, or `nil` if home directory cannot be determined.

#### Examples

```lisp
(home-directory)                    ; => "/home/alice" (Unix)
(home-directory)                    ; => "C:\\Users\\Alice" (Windows)

; Use in paths
(define config-dir (concat (home-directory) "/.config"))
; => "/home/alice/.config"
```

#### Platform Behavior

- **Unix/Linux/macOS**: Uses `$HOME` environment variable
- **Windows**: Uses `%USERPROFILE%` or `%HOMEDRIVE%%HOMEPATH%`

#### See Also

- `expand-path` - Expand `~/` prefix in paths

### `expand-path`

Expand `~/` prefix in file paths to the user's home directory.

#### Parameters

- `path` - File path (string), may start with `~/`

#### Returns

String with expanded path (if path starts with `~/`), or original string (if path does not start with `~/`).

#### Examples

```lisp
; Basic expansion
(expand-path "~/config.lisp")
; => "/home/alice/config.lisp" (Unix)
; => "C:\\Users\\Alice\\config.lisp" (Windows)

; Subdirectories
(expand-path "~/Documents/notes.txt")
; => "/home/alice/Documents/notes.txt" (Unix)

; No expansion (no ~/ prefix)
(expand-path "/etc/config")         ; => "/etc/config"
(expand-path "relative/path")       ; => "relative/path"
(expand-path "./local.lisp")        ; => "./local.lisp"

; Just ~ expands to home directory
(expand-path "~")                   ; => "/home/alice"

; Use with file I/O
(define file (open (expand-path "~/my-config.lisp") "r"))
(load (expand-path "~/scripts/init.lisp"))
```

#### Notes

- Detects `~/` at start of path
- Replaces `~/` with home directory from `home-directory`
- Handles cross-platform path separators
- Works with both forward and backslashes after `~`

#### Errors

Returns error if:

- Home directory cannot be determined
- Argument is not a string

#### Use Cases

- Reading/writing user configuration files
- Loading user-specific scripts
- Saving data to user directories
- Cross-platform file operations

#### See Also

- `home-directory` - Get home directory path
- `open` - Open file
- `load` - Load Lisp file

### `getenv`

Read an environment variable.

#### Parameters

- `name` - Environment variable name (string)

#### Returns

String value of the variable, or nil if not set.

#### Examples

```lisp
(getenv "HOME")            ; => "/home/alice"
(getenv "NONEXISTENT")     ; => nil
(getenv "PATH")            ; => "/usr/bin:/bin:..."
```

### `data-directory`

Return the platform-specific user data directory for an application.

#### Parameters

- `app` - Application name (string)

#### Returns

String path. Does not create the directory.

#### Platform Behavior

- **Linux/macOS**: `$XDG_DATA_HOME/app` or `~/.local/share/app`
- **Windows**: `%LOCALAPPDATA%\app` or `%APPDATA%\app`

#### Examples

```lisp
(data-directory "my-app")  ; => "/home/alice/.local/share/my-app"
(mkdir (data-directory "my-app"))  ; create it if needed
```

#### See Also

- `config-directory` - Get platform config directory
- `mkdir` - Create directories
- `getenv` - Read environment variables

### `config-directory`

Return the platform-specific user config directory for an application.

#### Parameters

- `app` - Application name (string)

#### Returns

String path. Does not create the directory.

#### Platform Behavior

- **Linux/macOS**: `$XDG_CONFIG_HOME/app` or `~/.config/app`
- **Windows**: `%APPDATA%\app`

#### Examples

```lisp
(config-directory "my-app")  ; => "/home/alice/.config/my-app"
(mkdir (config-directory "my-app"))  ; create it if needed
```

#### See Also

- `data-directory` - Get platform data directory
- `mkdir` - Create directories
- `getenv` - Read environment variables

### `file-exists?`

Check if a file or directory exists.

#### Parameters

- `path` - File path (string)

#### Returns

##t if path exists, nil otherwise.

#### Examples

```lisp
(file-exists? "/tmp")          ; => #t
(file-exists? "/no/such/path") ; => nil
```

### `mkdir`

Create a directory and all parent directories (like mkdir -p).

#### Parameters

- `path` - Directory path (string)

#### Returns

##t on success. Succeeds silently if directory already exists.

#### Examples

```lisp
(mkdir "/tmp/my-app/data")  ; creates /tmp/my-app and /tmp/my-app/data
(mkdir (data-directory "my-app"))  ; create app data dir
```

#### See Also

- `data-directory` - Get platform data directory
- `config-directory` - Get platform config directory
- `file-exists?` - Check if path exists

### `file-is-directory?`

Test whether a path is a directory.

#### Parameters

- `path` - File or directory path (string)

#### Returns

`#t` if the path exists and is a directory, `nil` otherwise (including if the path does not exist or is a regular file).

#### Examples

```lisp
(file-is-directory? "/tmp")            ; => #t
(file-is-directory? "/etc/hosts")      ; => nil (regular file)
(file-is-directory? "/no/such/path")   ; => nil (does not exist)

; Use to check before calling delete-directory
(if (file-is-directory? path)
    (delete-directory path :recursive)
    (delete-file path))
```

#### See Also

- `file-exists?` - Check if path exists (file or directory)
- `delete-directory` - Delete a directory
- `mkdir` - Create a directory

### `delete-directory`

Delete a directory. Like Emacs Lisp's `delete-directory`.

#### Parameters

- `path` - Directory path (string)
- `:recursive` - Optional keyword; if present, delete directory and all its contents

#### Returns

`nil` on success, error if deletion fails.

#### Examples

```lisp
(delete-directory "/tmp/empty-dir")                  ; remove empty directory
(delete-directory "/tmp/parent-dir" :recursive)     ; remove directory and all contents
```

#### Notes

Without `:recursive`, only empty directories can be removed (like `rmdir`).
With `:recursive`, the directory and all files and subdirectories inside it are deleted.
Use `delete-file` for files — `delete-file` will refuse to delete directories.

#### Errors

- Error if path is not a directory
- Error if directory is not empty and `:recursive` is not specified
- Error if deletion fails (permission denied, etc.)

#### See Also

- `delete-file` - Delete a file
- `mkdir` - Create a directory
- `file-exists?` - Check if path exists

### `system-type`

Return a symbol identifying the operating system family. Emacs Lisp style.

#### Parameters

None.

#### Returns

Symbol identifying the OS:

| Symbol       | Platform                             |
| ------------ | ------------------------------------ |
| `windows-nt` | Native Windows (MSVC/MinGW)          |
| `msys`       | MSYS2 / Cygwin (case-insensitive FS) |
| `darwin`     | macOS                                |
| `gnu/linux`  | Linux                                |
| `freebsd`    | FreeBSD                              |
| `netbsd`     | NetBSD                               |
| `openbsd`    | OpenBSD                              |
| `unknown`    | Unrecognized platform                |

#### Examples

```lisp
(system-type)        ; => gnu/linux (on Linux)
(system-type)        ; => windows-nt (on native Windows)
(system-type)        ; => msys (on MSYS2/Cygwin)

;; Branch on platform
(if (eq? (system-type) 'windows-nt)
    (load "windows-init.lisp")
    (load "unix-init.lisp"))

;; Guard case-sensitive-filesystem assertions
(unless (eq? (system-type) 'msys)
  (assert-false (file-exists? "MIXED-CASE-File")
    "case-sensitive filesystem distinguishes case"))
```

#### Notes

- `msys` covers both Cygwin and MSYS2 (including UCRT64 and MINGW64
  shells). The filesystem is case-insensitive in these environments,
  which affects tests that assert file case sensitivity.
- On native Windows builds (not through MSYS2/Cygwin), `windows-nt` is
  returned instead.

#### See Also

- `home-directory` - Get home directory (platform-specific)
- `config-directory` - Get config directory (platform-specific)
- `data-directory` - Get data directory (platform-specific)

---

## Functions

Function introspection and evaluation.

### `function-params`

Return the parameter list of a lambda or macro.

#### Parameters

- `func` - A lambda or macro

#### Returns

The parameter list of the function.

#### Examples

```lisp
(function-params (lambda (x y) (+ x y)))  ; => (x y)
```

### `function-body`

Return the body expression list of a lambda or macro.

#### Parameters

- `func` - A lambda or macro

#### Returns

The body expressions as a list.

#### Examples

```lisp
(function-body (lambda (x) (+ x 1)))  ; => ((+ x 1))
```

### `function-name`

Return the name of a lambda, macro, or builtin, or nil.

#### Parameters

- `func` - A lambda, macro, or builtin

#### Returns

The name as a string, or nil if anonymous.

#### Examples

```lisp
(function-name +)              ; => "+"
(function-name (lambda (x) x)) ; => nil
```

### `documentation`

Get documentation string for function, macro, variable, or built-in bound to symbol.

First checks the symbol's own docstring (set via `set-documentation!` or copied from lambda/macro on `define`), then falls back to the value's docstring if bound to a lambda/macro/builtin.

#### Parameters

- `symbol` - Symbol name (quoted)

#### Returns

String containing the documentation, or `nil` if:

- No docstring exists
- Symbol is unbound
- Symbol's value is not a function, macro, or built-in

#### Examples

```lisp
; User-defined function
(define calculate-area
  (lambda (width height)
    "Calculate the area of a rectangle.

    ## Parameters
    - `width` - Width of the rectangle
    - `height` - Height of the rectangle

    ## Returns
    The area as a number."
    (* width height)))

(documentation 'calculate-area)
; => "Calculate the area of a rectangle.\
\
    ## Parameters..."

; Built-in function
(documentation 'car)
; => "Get the first element of a list (the car).\
\
#### Parameters..."

; Macro
(defmacro when (condition . body)
  "Execute BODY when CONDITION is true."
  `(if ,condition (progn ,@body) nil))

(documentation 'when)
; => "Execute BODY when CONDITION is true."

; Undefined symbol
(documentation 'nonexistent)  ; => ERROR: Undefined symbol

; Function without docstring
(define no-doc (lambda (x) (* x 2)))
(documentation 'no-doc)       ; => nil
```

#### Notes

- Similar to Emacs Lisp's `documentation` function
- Works with lambdas, macros, and built-in functions
- Docstrings use CommonMark (Markdown) format

#### Errors

Returns error if:

- Argument is not a symbol
- Symbol is undefined

#### See Also

- `bound?` - Check if symbol is bound
- `set-documentation!` - Set documentation for symbol

### `bound?`

Check if a symbol is bound in the environment.

#### Parameters

- `symbol` - Symbol name (quoted)

#### Returns

`#t` if the symbol is bound, `#f` otherwise.

#### Examples

```lisp
(bound? 'car)              ; => #t (built-in)
(bound? 'nonexistent)      ; => #f
(define my-var 42)
(bound? 'my-var)           ; => #t
```

#### Notes

- Used by `defvar` macro to conditionally define variables
- Checks current and parent environments

#### See Also

- `defvar` - Define variable only if unbound
- `documentation` - Get documentation for symbol

### `set-documentation!`

Set documentation string directly on a symbol. Since symbols are interned (globally shared), this sets the docstring globally. The symbol does not need to be bound.

#### Parameters

- `symbol` - Symbol name (quoted)
- `docstring` - Documentation string (CommonMark format)

#### Returns

`#t` on success.

#### Examples

```lisp
; Set docstring on a bound symbol
(define my-var 42)
(set-documentation! 'my-var "The answer to everything.")
(documentation 'my-var)  ; => "The answer to everything."

; Can also set docstring on unbound symbols
(set-documentation! 'future-var "Will be defined later.")
(documentation 'future-var)  ; => "Will be defined later."
```

#### Notes

- Used by `defvar` and `defconst` macros
- Docstrings use CommonMark (Markdown) format
- Docstrings are stored on symbols (like Emacs Lisp), not on bindings, so they are global per symbol name
- The symbol does not need to be bound

#### Errors

Returns error if:

- First argument is not a symbol
- Second argument is not a string

#### See Also

- `documentation` - Get documentation for symbol
- `defvar` - Define variable with optional docstring
- `defconst` - Define constant with optional docstring

### `eval`

Evaluate an expression in the current environment.

#### Parameters

- `expression` - A Lisp expression to evaluate

#### Returns

The result of evaluating the expression.

#### Examples

```lisp
(eval '(+ 1 2))         ; => 3
(eval 'my-function)      ; => #<lambda my-function ...>
```

#### Notes

Useful for evaluating quoted expressions and symbols to get their bound values.

### `exit`

`(exit)` or `(exit code)`

Terminates the process with the given exit code (default 0). `quit` is an alias.

### `quit`

Alias for `exit`. See `exit`.

### Standard Library Aliases

The following aliases are defined in the standard library via `defalias`. They resolve to the same function.

- `doc` - Alias for `documentation`
- `doc-set!` - Alias for `set-documentation!`

---

## Hash Tables

Mutable key-value hash table operations.

Hash tables support all key types: strings, symbols, keywords, integers, floats, characters, booleans, nil, lists, vectors, and other hash tables. Keys are compared by value (structural equality), so `(hash-ref ht '(1 2))` will find a key set with `(hash-set! ht '(1 2) val)`.

### `make-hash-table`

Create a new empty hash table.

#### Returns

A new hash table with no entries.

#### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "key" "value")
(hash-ref ht "key")         ; => "value"
```

### `hash-ref`

Get value for key from hash table.

#### Parameters

- `hash-table` - The hash table to query
- `key` - The key to look up

#### Returns

The value associated with `key`, or `nil` if key not found.

#### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "name" "Alice")
(hash-ref ht "name")        ; => "Alice"
(hash-ref ht "missing")     ; => nil
```

### `hash-set!`

Set key-value pair in hash table (mutating operation).

#### Parameters

- `hash-table` - The hash table to modify
- `key` - The key to set
- `value` - The value to associate with key

#### Returns

The value that was set.

#### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "name" "Alice")    ; => "Alice"
(hash-set! ht "age" 30)           ; => 30
(hash-ref ht "name")              ; => "Alice"
```

#### Notes

Modifies hash table in place. If key already exists, value is updated.

### `hash-remove!`

Remove key-value pair from hash table (mutating operation).

#### Parameters

- `hash-table` - The hash table to modify
- `key` - The key to remove

#### Returns

`#t` if key was found and removed, `nil` if key wasn't present.

#### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "key" "value")
(hash-remove! ht "key")       ; => #t
(hash-remove! ht "missing")   ; => nil
```

### `hash-clear!`

Remove all entries from hash table (mutating).

#### Parameters

- `table` - Hash table to clear

#### Returns

`nil`

#### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "key1" 10)
(hash-set! ht "key2" 20)
(hash-count ht)         ; => 2
(hash-clear! ht)
(hash-count ht)         ; => 0
```

#### Notes

This is a mutating operation (note the `!` suffix). The hash table is modified in place.

#### See Also

- `hash-remove!` - Remove single entry
- `hash-count` - Get entry count
- `make-hash-table` - Create new hash table

### `hash-count`

Get the number of key-value pairs in hash table.

#### Parameters

- `hash-table` - The hash table to measure

#### Returns

Integer count of entries in the hash table.

#### Examples

```lisp
(define ht (make-hash-table))
(hash-count ht)                  ; => 0
(hash-set! ht "a" 1)
(hash-set! ht "b" 2)
(hash-count ht)                  ; => 2
```

### `hash-keys`

Get list of all keys in hash table.

#### Parameters

- `hash-table` - The hash table to query

#### Returns

List of all keys in the hash table. Order is not guaranteed.

#### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "name" "Alice")
(hash-set! ht "age" 30)
(hash-keys ht)               ; => ("name" "age") or ("age" "name")
```

### `hash-values`

Get list of all values in hash table.

#### Parameters

- `hash-table` - The hash table to query

#### Returns

List of all values in the hash table. Order is not guaranteed.

#### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "name" "Alice")
(hash-set! ht "age" 30)
(hash-values ht)             ; => ("Alice" 30) or (30 "Alice")
```

### `hash-entries`

Get list of all key-value pairs as cons cells.

#### Parameters

- `hash-table` - The hash table to query

#### Returns

List of `(key . value)` pairs. Order is not guaranteed.

#### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "name" "Alice")
(hash-set! ht "age" 30)
(hash-entries ht)            ; => (("name" . "Alice") ("age" . 30))
```

#### Notes

Useful for iterating over hash tables with association list functions.

---

## Lists

List construction, access, and higher-order operations.

### `car`

Get the first element of a list.

#### Parameters

- `list` - A non-empty list

#### Returns

The first element of the list.

#### Examples

```lisp
(car '(1 2 3))           ; => 1
(car '("a" "b"))        ; => "a"
(car (list 10 20 30))    ; => 10
```

#### See Also

- `cdr` - Get the rest of a list
- `cons` - Construct a new list cell

### `cdr`

Get the rest of a list (all elements except the first).

#### Parameters

- `list` - A non-empty list

#### Returns

A list containing all elements except the first. Returns `nil` if the list has only one element.

#### Examples

```lisp
(cdr '(1 2 3))           ; => (2 3)
(cdr '(1))               ; => nil
(cdr '("a" "b" "c"))    ; => ("b" "c")
```

#### See Also

- `car` - Get the first element of a list
- `cons` - Construct a new list cell

### `caar`

Return the car of the car of a list. Equivalent to `(car (car x))`.

#### Parameters

- `list` - A nested list

#### Returns

The first element of the first element.

#### Examples

```lisp
(caar '((1 2) 3))    ; => 1
(caar '((a b) c d))  ; => a
```

#### See Also

- `car` - First element
- `cadr` - Second element

### `cadr`

Return the car of the cdr of a list. Equivalent to `(car (cdr x))`. Same as `second`.

#### Parameters

- `list` - A list with at least two elements

#### Returns

The second element of the list.

#### Examples

```lisp
(cadr '(1 2 3))      ; => 2
(cadr '(a b c))      ; => b
```

#### See Also

- `car` - First element
- `caddr` - Third element

### `cdar`

Return the cdr of the car of a list. Equivalent to `(cdr (car x))`.

#### Parameters

- `list` - A list whose first element is a list

#### Returns

The rest of the first element.

#### Examples

```lisp
(cdar '((1 2 3) 4))  ; => (2 3)
(cdar '((a b) c))    ; => (b)
```

#### See Also

- `cdr` - Rest of list
- `caar` - First of first

### `cddr`

Return the cdr of the cdr of a list. Equivalent to `(cdr (cdr x))`.

#### Parameters

- `list` - A list with at least two elements

#### Returns

The list without its first two elements.

#### Examples

```lisp
(cddr '(1 2 3 4))    ; => (3 4)
(cddr '(a b))        ; => nil
```

#### See Also

- `cdr` - Rest of list
- `caddr` - Third element

### `caddr`

Return the third element of a list. Equivalent to `(car (cdr (cdr x)))`. Same as `third`.

#### Parameters

- `list` - A list with at least three elements

#### Returns

The third element of the list.

#### Examples

```lisp
(caddr '(1 2 3 4))   ; => 3
(caddr '(a b c))     ; => c
```

#### See Also

- `cadr` - Second element
- `cadddr` - Fourth element

### `cadddr`

Return the fourth element of a list. Equivalent to `(car (cdr (cdr (cdr x))))`. Same as `fourth`.

#### Parameters

- `list` - A list with at least four elements

#### Returns

The fourth element of the list.

#### Examples

```lisp
(cadddr '(1 2 3 4 5)) ; => 4
(cadddr '(a b c d))   ; => d
```

#### See Also

- `caddr` - Third element
- `fourth` - Alias for this function

### `first`

Return the first element of a list. Alias for `car`.

#### Parameters

- `list` - A non-empty list

#### Returns

The first element of the list.

#### Examples

```lisp
(first '(1 2 3))     ; => 1
(first '(a b c))     ; => a
```

#### See Also

- `second` - Second element
- `rest` - All but first element

### `second`

Return the second element of a list. Alias for `cadr`.

#### Parameters

- `list` - A list with at least two elements

#### Returns

The second element of the list.

#### Examples

```lisp
(second '(1 2 3))    ; => 2
(second '(a b c))    ; => b
```

#### See Also

- `first` - First element
- `third` - Third element

### `third`

Return the third element of a list. Alias for `caddr`.

#### Parameters

- `list` - A list with at least three elements

#### Returns

The third element of the list.

#### Examples

```lisp
(third '(1 2 3 4))   ; => 3
(third '(a b c))     ; => c
```

#### See Also

- `second` - Second element
- `fourth` - Fourth element

### `fourth`

Return the fourth element of a list. Alias for `cadddr`.

#### Parameters

- `list` - A list with at least four elements

#### Returns

The fourth element of the list.

#### Examples

```lisp
(fourth '(1 2 3 4 5)) ; => 4
(fourth '(a b c d))   ; => d
```

#### See Also

- `third` - Third element
- `first` - First element

### `rest`

Return all elements except the first. Alias for `cdr`.

#### Parameters

- `list` - A non-empty list

#### Returns

A list of all elements except the first. Returns `nil` for a single-element list.

#### Examples

```lisp
(rest '(1 2 3))      ; => (2 3)
(rest '(a))          ; => nil
```

#### See Also

- `first` - First element
- `cdr` - Same function

### `set-car!`

Set the first element of a cons cell (mutating).

#### Parameters

- `pair` - A cons cell to modify
- `value` - The new value for the car

#### Returns

The new value.

#### Examples

```lisp
(define x '(1 2 3))
(set-car! x 99)          ; => 99
x                        ; => (99 2 3)

(define y (cons 'a 'b))
(set-car! y 'z)          ; => z
y                        ; => (z . b)
```

#### See Also

- `set-cdr!` - Set the rest of a cons cell
- `car` - Get the first element

### `set-cdr!`

Set the rest of a cons cell (mutating).

#### Parameters

- `pair` - A cons cell to modify
- `value` - The new value for the cdr

#### Returns

The new value.

#### Examples

```lisp
(define x '(1 2 3))
(set-cdr! x '(99))       ; => (99)
x                        ; => (1 99)

(define y (cons 'a 'b))
(set-cdr! y 'z)          ; => z
y                        ; => (a . z)
```

#### See Also

- `set-car!` - Set the first element of a cons cell
- `cdr` - Get the rest of a list

### `cons`

Construct a new list cell (cons cell).

#### Parameters

- `element` - The value to place in the car (first position)
- `list` - The list to place in the cdr (rest position)

#### Returns

A new list with `element` as the first item and `list` as the rest.

#### Examples

```lisp
(cons 1 '(2 3))          ; => (1 2 3)
(cons 'a nil)            ; => (a)
(cons 1 (cons 2 '()))    ; => (1 2)
```

#### See Also

- `car` - Get the first element
- `cdr` - Get the rest
- `list` - Create a list from multiple elements

### `list`

Create a list from the given elements.

#### Parameters

- `elements...` - Zero or more elements to put in the list

#### Returns

A new list containing all the provided elements. Returns `nil` for empty list.

#### Examples

```lisp
(list 1 2 3)             ; => (1 2 3)
(list "a" "b" "c")      ; => ("a" "b" "c")
(list)                   ; => nil
(list (+ 1 2) (* 3 4))   ; => (3 12) (evaluates arguments)
```

#### See Also

- `cons` - Construct a single list cell
- `append` - Concatenate lists

### `length`

Get the length of a sequence (list, string, or vector).

#### Parameters

- `sequence` - A list, string, or vector

#### Returns

Integer count of elements/characters in the sequence.

#### Examples

```lisp
(length '(1 2 3))        ; => 3
(length "hello")         ; => 5
(length #(a b c))        ; => 3
(length '())              ; => 0
```

#### Notes

Unified function for all sequence types. For strings, returns UTF-8 character count.

### `list-ref`

Get element at index from list (0-based).

#### Parameters

- `list` - The list to access
- `index` - Zero-based index (integer)

#### Returns

The element at the specified index.

#### Examples

```lisp
(list-ref '(a b c) 0)        ; => a
(list-ref '(a b c) 2)        ; => c
(list-ref '(1 2 3 4) 3)      ; => 4
```

#### Errors

Returns error if index is out of bounds.

### `reverse`

Reverse a list.

#### Parameters

- `list` - The list to reverse

#### Returns

New list with elements in reverse order.

#### Examples

```lisp
(reverse '(1 2 3))           ; => (3 2 1)
(reverse '(a))               ; => (a)
(reverse '())                ; => nil
```

#### Notes

Returns a new list (does not modify input).

### `append`

Concatenate multiple lists into a single list.

#### Parameters

- `lists...` - Zero or more lists to concatenate

#### Returns

A new list containing all elements from all input lists in order. Returns `nil` if no arguments provided.

#### Examples

```lisp
(append '(1 2) '(3 4))               ; => (1 2 3 4)
(append '(1 2) '(3 4) '(5 6))        ; => (1 2 3 4 5 6)
(append '() '(1 2))                  ; => (1 2)
(append '(1 2) '())                  ; => (1 2)
(append)                             ; => nil
```

#### Notes

- Returns a **new list** (does not modify input lists)
- Works with any number of lists

### `assoc`

Find key-value pair in association list using structural equality.

#### Parameters

- `key` - Key to search for
- `alist` - Association list (list of `(key . value)` pairs)

#### Returns

The first `(key . value)` pair where key matches (using `equal?`), or `nil` if not found.

#### Examples

```lisp
(assoc 'name '((name . "Alice") (age . 30)))     ; => (name . "Alice")
(assoc 'city '((name . "Bob") (age . 25)))       ; => nil
(assoc '(1 2) '(((1 2) . "pair") (3 . "x")))   ; => ((1 2) . "pair")
```

#### Notes

Uses deep structural comparison (`equal?`). For pointer equality, use `assq`.

#### See Also

- `assq` - Pointer equality search
- `assv` - Value equality search
- `alist-get` - Get value directly

### `assq`

Find key-value pair in association list using pointer equality.

#### Parameters

- `key` - Key to search for
- `alist` - Association list (list of `(key . value)` pairs)

#### Returns

The first `(key . value)` pair where key matches (using `eq?`), or `nil` if not found.

#### Examples

```lisp
(assq 'name '((name . "Alice") (age . 30)))      ; => (name . "Alice")
(assq 'city '((name . "Bob") (age . 25)))        ; => nil
```

#### Notes

- Uses pointer equality (`eq?`) - fastest but only reliable for symbols
- Symbols are always interned, so `assq` is safe for symbol keys
- NOT reliable for numbers or strings (use `assoc` instead)

#### See Also

- `assoc` - Structural equality search
- `eq?` - Pointer equality predicate

### `assv`

Find key-value pair in association list using value equality.

#### Parameters

- `key` - Key to search for
- `alist` - Association list (list of `(key . value)` pairs)

#### Returns

The first `(key . value)` pair where key matches (using `eqv?`), or `nil` if not found.

#### Examples

```lisp
(assv 42 '((42 . "answer") (10 . "ten")))       ; => (42 . "answer")
(assv 'key '((key . "value") (x . "y")))        ; => (key . "value")
```

#### Notes

Uses `eqv?` equality (same as `equal?` in this implementation).

#### See Also

- `assoc` - Structural equality (equivalent in this implementation)
- `assq` - Pointer equality

### `alist-get`

Get value for key from association list with optional default.

#### Parameters

- `key` - Key to search for
- `alist` - Association list (list of `(key . value)` pairs)
- `default` - Optional default value (defaults to `nil`)

#### Returns

The value (cdr) of the first matching pair, or `default` if key not found.

#### Examples

```lisp
(alist-get 'name '((name . "Alice") (age . 30)))        ; => "Alice"
(alist-get 'city '((name . "Bob")) "Unknown")          ; => "Unknown"
(alist-get 'missing '((a . 1) (b . 2)))                  ; => nil
```

#### Notes

Returns the **value** directly, not the `(key . value)` pair.

#### See Also

- `assoc` - Get full key-value pair
- `hash-ref` - Hash table lookup

### `member`

Find element in list using structural equality.

#### Parameters

- `item` - Item to search for
- `list` - List to search in

#### Returns

The tail of the list starting from the first match, or `nil` if not found.

#### Examples

```lisp
(member 'b '(a b c d))          ; => (b c d)
(member 'x '(a b c))            ; => nil
(member "foo" '("bar" "foo" "baz"))  ; => ("foo" "baz")
```

#### Notes

Uses deep structural comparison (`equal?`). For pointer equality, use `memq`.

#### See Also

- `memq` - Pointer equality search
- `assoc` - Association list lookup

### `memq`

Find element in list using pointer equality.

#### Parameters

- `item` - Item to search for
- `list` - List to search in

#### Returns

The tail of the list starting from the first match, or `nil` if not found.

#### Examples

```lisp
(memq 'b '(a b c d))            ; => (b c d)
(memq 'x '(a b c))              ; => nil
```

#### Notes

- Uses pointer equality (`eq?`) - fastest but only reliable for symbols
- Symbols are always interned, so `memq` is safe for symbol searches
- NOT reliable for numbers or strings (use `member` instead)

#### See Also

- `member` - Structural equality search
- `assq` - Association list lookup with eq?

### `map`

Apply function to each element of list, return new list of results.

#### Parameters

- `function` - Function to apply to each element
- `list` - List to process

#### Returns

New list containing results of applying `function` to each element.

#### Examples

```lisp
(map (lambda (x) (* x 2)) '(1 2 3 4))    ; => (2 4 6 8)
(map car '((1 . 2) (3 . 4) (5 . 6)))     ; => (1 3 5)
(map string-upcase '("a" "b" "c"))      ; => ("A" "B" "C")
```

### `mapcar`

Apply function to each element of a list. Alias for `map`.

#### Parameters

- `function` - Function to apply to each element
- `list` - List of elements

#### Returns

New list of results from applying `function` to each element.

#### Examples

```lisp
(mapcar (lambda (x) (* x 2)) '(1 2 3))  ; => (2 4 6)
```

#### See Also

- `map` - Same function
- `filter` - Select elements matching predicate

### `filter`

Filter list elements that satisfy a predicate function.

#### Parameters

- `predicate` - Function that returns true/false for each element
- `list` - List to filter

#### Returns

New list containing only elements for which `predicate` returns non-nil.

#### Examples

```lisp
(filter (lambda (x) (> x 0)) '(1 -2 3 -4 5))  ; => (1 3 5)
(filter even? '(1 2 3 4 5 6))                 ; => (2 4 6)
(filter string? '(1 "a" 2 "b"))              ; => ("a" "b")
```

### `apply`

Apply a function to a list of arguments.

#### Parameters

- `function` - Function to call
- `args` - List of arguments to pass to the function

#### Returns

Result of calling `function` with the elements of `args` as arguments.

#### Examples

```lisp
(apply + '(1 2 3 4))        ; => 10
(apply list '(a b c))       ; => (a b c)
(apply max '(3 1 4 1 5))    ; => 5

;; Useful for calling functions with dynamic argument lists
(define args '(1 2 3))
(apply + args)              ; => 6
```

---

## Packages

Package (namespace) and library management.

### `in-package`

Set the current package.

#### Parameters

- `name` - Package name (symbol). Strings also accepted for convenience.

#### Returns

The package name as a symbol.

#### Examples

```lisp
(in-package 'math)
(in-package 'user)
```

### `current-package`

Return the current package name as a symbol.

#### Examples

```lisp
(current-package) ; => user
```

### `package-symbols`

Return an alist of bindings in the named package.

#### Parameters

- `name` - Package name (symbol). Strings also accepted for convenience.

#### Returns

An alist of `(symbol . value)` pairs.

#### Examples

```lisp
(package-symbols 'user) ; => ((x . 42) ...)
```

### `list-packages`

Return a list of distinct package names as symbols.

#### Examples

```lisp
(list-packages) ; => (core user)
```

### `package-save`

Save package bindings to a file as valid Lisp source.

#### Parameters

- `filename` - Path to write (string)
- `package` (optional) - Package name (symbol). Defaults to current package.
- `:defun` (optional) - Extract lambdas into named `defun` forms for readability.
- `:format` (optional) - Pretty-print the output (requires lisp-fmt.lisp to be loaded).

#### Returns

`#t` on success.

#### Examples

```lisp
(define x 42)
(package-save "my-pkg.lisp")
(package-save "math.lisp" 'math)
(package-save "my-pkg.lisp" :defun)
(package-save "math.lisp" 'math :defun)
(package-save "my-pkg.lisp" :defun :format)
```

#### Notes

The output file can be loaded with `(load filename)`. Strings are also accepted
for the package name and converted to symbols internally.

With `:defun`, top-level lambdas emit as `(defun name ...)` and nested lambdas
are extracted into separate defun forms referenced by name.

With `:format`, the output is pretty-printed using `format-file` from lisp-fmt.lisp.
Load lisp-fmt.lisp first: `(load "lisp/lisp-fmt.lisp")`.

### `provide`

Register a feature as loaded by adding it to `*features*`.

Calling `provide` multiple times with the same feature is idempotent —
the feature is only added once.

#### Parameters

- `feature` - Feature name (symbol). Strings also accepted.

#### Returns

The feature symbol.

#### Examples

```lisp
(provide 'mylib)
(provide "mylib") ; string also accepted
```

### `require`

Load a library if it has not already been loaded.

Checks if the feature is already in `*features*`. If so, returns immediately.
Otherwise, searches load paths for the library file, loads it (which should
call `provide`), and verifies the feature was provided.

#### Load Path Search Order

1. Current directory (`name.lisp`, then `name/name.lisp`)
2. Directories in `DITTY_LISP_PATH` (colon-separated on Unix, semicolon on Windows)
3. `$XDG_DATA_HOME/ditty/lisp/` (default: `~/.local/share/ditty/lisp/`)
4. Each dir in `$XDG_DATA_DIRS/ditty/lisp/` (default: `/usr/local/share:/usr/share`)

On Windows: `%APPDATA%\ditty\lisp\` (user) and exe-relative `..\share\ditty\lisp\` (system).

#### Parameters

- `feature` - Feature name (symbol). Strings also accepted.

#### Returns

The feature symbol on success.

#### Errors

- If the library file cannot be found.
- If the file loads but does not call `(provide 'feature)` for the expected feature.

#### Examples

```lisp
(require 'mylib)
(require "mylib") ; string also accepted
```

#### Notes

`require` saves and restores `*package*`, so a library's `in-package` does not
leak to the caller. Transitive dependencies work naturally: if `mylib` calls
`(require 'utils)` at its top, loading `mylib` automatically pulls in `utils`.

### `provided?`

Check if a feature has been provided (is in `*features*`).

#### Parameters

- `feature` - Feature name (symbol). Strings also accepted.

#### Returns

`#t` if the feature is loaded, `nil` otherwise.

#### Examples

```lisp
(provided? 'mylib) ; => #t if (provide 'mylib) or (require 'mylib) was called
(provided? 'nonexistent) ; => nil
```

### `export`

Mark symbols as exported from the current package.

Exported symbols are the public API of a package. Non-exported symbols are
internal and only accessible via `pkg:symbol` qualified access.

If `export` is never called for a package, `use-package` imports all bindings
(default = all exported).

#### Parameters

- `symbol` ... - One or more quoted symbols to export.

#### Returns

`#t` on success.

#### Examples

```lisp
(in-package 'mylib)
(export 'greet 'farewell)
(defun greet (name) (concat "Hello, " name))
(defun farewell (name) (concat "Goodbye, " name))
(defun internal-helper () "secret") ; not exported
(provide 'mylib)
```

### `package-exports`

Return the list of exported symbols for a package.

#### Parameters

- `name` - Package name (symbol). Strings also accepted.

#### Returns

A list of exported symbols, or `nil` if no explicit exports were set.

#### Examples

```lisp
(package-exports 'mylib) ; => (greet farewell)
(package-exports 'core) ; => nil (no explicit exports)
```

#### Notes

Returns `nil` when no `export` has been called for the package. In this case,
`use-package` treats all bindings as exported (Emacs Lisp convention).

### `use-package`

Import exported symbols from a package into the current package.

After `use-package`, exported symbols from the source package are accessible
unqualified in the current package. Non-exported symbols remain accessible
only via `pkg:symbol` qualified syntax.

If the source package has no explicit `export` list, all bindings are imported.

#### Parameters

- `name` - Package name (symbol). Strings also accepted.

#### Returns

`#t` on success.

#### Errors

- If the named package does not exist (has no bindings).

#### Examples

```lisp
(require 'mylib)
(use-package 'mylib)
(greet "World") ; now accessible unqualified
```

### `*features*`

A list of loaded feature symbols. Updated by `provide` and checked by `require`.
Initially `nil`.

```lisp
*features* ; => (mylib utils lisp-fmt)
```

### `*load-path*`

A read-only list of directory strings where `require` searches for libraries.
Populated at startup from `DITTY_LISP_PATH` and XDG data directories.

```lisp
*load-path* ; => ("/home/me/.local/share/ditty/lisp" "/usr/local/share/ditty/lisp" ...)
```

### `*command-line-args*`

A list of command-line arguments passed to the script after the `--` separator.
Only defined when `--` is present on the command line; use `(bound? '*command-line-args*)` to check.

```lisp
; Run: ditty script.lisp -- arg1 arg2 arg3
(if (bound? '*command-line-args*)
    (format #t "Args: ~A~%" *command-line-args*)
    (format #t "No arguments~%"))

; Iterate over arguments
(when (bound? '*command-line-args*)
  (do ((args *command-line-args* (cdr args)))
    ((null? args))
    (format #t "Arg: ~A~%" (car args))))
```

Arguments are strings in the order they appeared. Files before `--` are executed; arguments after `--` are passed to the last script.

### Package Aliases

The following aliases are defined in the standard library for tab-completion discoverability. They resolve to the same function via `defalias`.

- `package-set` - Alias for `in-package`
- `package-current` - Alias for `current-package`
- `package-list` - Alias for `list-packages`

---

## Printing

Output functions (Common Lisp style).

### `princ`

Print object in human-readable form (without quotes on strings).

#### Parameters

- `object` - Any object to print

#### Returns

The object that was printed.

#### Examples

```lisp
(princ "Hello")         ; Prints: Hello (no quotes)
(princ 42)              ; Prints: 42
(princ '(1 2 3))        ; Prints: (1 2 3)
```

#### Notes

Output goes to stdout. Strings print without surrounding quotes.

### `prin1`

Print object in readable representation (with quotes on strings).

#### Parameters

- `object` - Any object to print

#### Returns

The object that was printed.

#### Examples

```lisp
(prin1 "Hello")         ; Prints: "Hello" (with quotes)
(prin1 42)              ; Prints: 42
(prin1 '(1 2 3))        ; Prints: (1 2 3)
```

#### Notes

Output goes to stdout. Strings print with surrounding quotes.

### `print`

Print object like prin1 but with newlines before and after.

#### Parameters

- `object` - Any object to print

#### Returns

The object that was printed.

#### Examples

```lisp
(print "Hello")         ; Prints: \
"Hello"\

(print 42)              ; Prints: \
42\

```

#### Notes

Output goes to stdout. Adds newline before and after output.

### `format`

Formatted output with directives (Common Lisp style).

#### Parameters

- `destination` - `nil` (return string) or `#t` (print to stdout)
- `format-string` - Format string with directives
- `args...` - Arguments to format

#### Returns

Formatted string if `destination` is `nil`, otherwise `nil`.

#### Format Directives

- `~A` or `~a` - Aesthetic (princ-style, no quotes)
- `~S` or `~s` - S-expression (prin1-style, with quotes)
- `~%` - Newline
- `~~` - Literal tilde (~)

#### Examples

```lisp
(format nil "Hello, ~A!" "World")     ; => "Hello, World!"
(format nil "~A + ~A = ~A" 2 3 5)     ; => "2 + 3 = 5"
(format nil "String: ~S" "test")     ; => "String: \"test\""
(format nil "Line 1~%Line 2")         ; => "Line 1\
Line 2"
(format #t "Hello!~%" )               ; Prints: Hello!\

                                       ; => nil
```

### `terpri`

Print newline (terminate print).

#### Returns

`nil`

#### Examples

```lisp
(princ "Hello")
(terpri)                ; Prints newline
(princ "World")
```

#### Notes

Useful for adding newlines between output.

---

## Regular Expressions

PCRE2-based regular expression matching and replacement.

Patterns may be supplied as strings or as compiled regex objects produced by
`regex-compile`. Reuse a compiled regex when matching the same pattern many
times to avoid recompilation overhead.

### `regex-compile`

Compile a PCRE2 pattern into a reusable regex object.

#### Parameters

- `pattern` - PCRE2 regular expression pattern (string)

#### Returns

A compiled regex object usable in any regex operation. Pattern errors are
reported here rather than at match time.

#### Examples

```lisp
(define digits (regex-compile "\\d+"))
(regex-match? digits "abc123")     ; => #t
(regex-find digits "x42y99")       ; => "42"
(regex-find-all digits "a1b22c3")  ; => ("1" "22" "3")
```

#### See Also

- `regex?` - Test if a value is a compiled regex
- `regex-valid?` - Test if a pattern compiles

### `regex?`

Test if a value is a compiled regex object.

#### Parameters

- `value` - Any value

#### Returns

`#t` if `value` was produced by `regex-compile`, `nil` otherwise. Pattern
strings are _not_ regex objects.

#### Examples

```lisp
(regex? (regex-compile "\\d+"))  ; => #t
(regex? "\\d+")                  ; => nil
```

### `regex-match?`

Test if regex pattern matches string.

#### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)
- `string` - String to match against

#### Returns

`#t` if pattern matches anywhere in string, `nil` otherwise.

#### Examples

```lisp
(regex-match? "[0-9]+" "abc123")      ; => #t
(regex-match? "^[0-9]+$" "abc123")    ; => nil (not all digits)
(regex-match? "hello" "hello world")  ; => #t
```

#### See Also

- `regex-find` - Return the matched text
- `regex-find-all` - Find all matches
- `string-match?` - Wildcard pattern matching

### `regex-find`

Find first regex match in string.

#### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)
- `string` - String to search

#### Returns

First matching substring, or `nil` if no match found.

#### Examples

```lisp
(regex-find "[0-9]+" "abc123def456")  ; => "123"
(regex-find "\\d+" "no digits")       ; => nil
(regex-find "h.llo" "hello world")    ; => "hello"
```

#### See Also

- `regex-match?` - Test if pattern matches
- `regex-find-all` - Find all matches
- `regex-extract` - Extract capture groups

### `regex-find-all`

Find all regex matches in string.

#### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)
- `string` - String to search

#### Returns

List of all matching substrings (in order), or empty list if no matches.

#### Examples

```lisp
(regex-find-all "[0-9]+" "a1b22c333")  ; => ("1" "22" "333")
(regex-find-all "\\w+" "hello world")  ; => ("hello" "world")
(regex-find-all "x" "no match")        ; => ()
```

#### See Also

- `regex-find` - Find first match only
- `regex-split` - Split string by pattern

### `regex-extract`

Extract capture groups from regex match.

#### Parameters

- `pattern` - PCRE2 regular expression with capture groups `(...)`
- `string` - String to match against

#### Returns

List of captured substrings (excluding group 0), or `nil` if no match.

#### Examples

```lisp
(regex-extract "(\\d+)-(\\d+)" "12-34")     ; => ("12" "34")
(regex-extract "(\\w+)@(\\w+)" "user@host")  ; => ("user" "host")
(regex-extract "(no)(match)" "text")       ; => nil
```

#### Notes

Only returns capture groups (not the full match). Use parentheses `()` in pattern to define groups.

### `regex-replace`

Replace all regex matches in string.

#### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)
- `string` - String to search and replace in
- `replacement` - Replacement string (can use `$1`, `$2` for captures)

#### Returns

New string with all matches replaced.

#### Examples

```lisp
(regex-replace "[0-9]+" "a1b2c3" "X")        ; => "aXbXcX"
(regex-replace "(\\w+)" "hello" "[$1]")     ; => "[hello]"
(regex-replace "\\s+" "a  b  c" " ")        ; => "a b c"
```

#### Notes

Always replaces ALL occurrences. Use `$1`, `$2`, etc. to reference capture groups.

#### See Also

- `regex-replace-all` - Alias for this function
- `string-replace` - Literal string replacement (no regex)

### `regex-replace-all`

Replace all regex matches in string. Alias for `regex-replace`.

#### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)
- `string` - String to search and replace in
- `replacement` - Replacement string (can use `$1`, `$2` for captures)

#### Returns

New string with all matches replaced.

#### Examples

```lisp
(regex-replace-all "[0-9]+" "a1b2c3" "X")  ; => "aXbXcX"
```

#### See Also

- `regex-replace` - Same function
- `string-replace` - Literal string replacement (no regex)

### `regex-split`

Split string by regex pattern.

#### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)
- `string` - String to split

#### Returns

List of substrings split by pattern matches.

#### Examples

```lisp
(regex-split "[,;]" "a,b;c")        ; => ("a" "b" "c")
(regex-split "\\s+" "a  b   c")    ; => ("a" "b" "c")
(regex-split "\\d+" "a1b2c")       ; => ("a" "b" "c")
```

#### See Also

- `split` - Split by literal string or wildcard
- `regex-find-all` - Find all matches (doesn't split)

### `regex-escape`

Escape special regex characters in string.

#### Parameters

- `string` - String to escape

#### Returns

New string with regex metacharacters escaped.

#### Examples

```lisp
(regex-escape "a.b")         ; => "a\\.b"
(regex-escape "$100")        ; => "\\$100"
(regex-escape "(test)")      ; => "\\(test\\)"
```

#### Notes

Escapes these characters: `. ^ $ * + ? ( ) [ ] { } | \`

#### Use Case

Use this when you need to match user input literally in a regex pattern.

### `regex-valid?`

Test if regex pattern is valid.

#### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)

#### Returns

`#t` if pattern compiles successfully, `nil` if invalid.

#### Examples

```lisp
(regex-valid? "[0-9]+")      ; => #t
(regex-valid? "[0-9")        ; => nil (unclosed bracket)
(regex-valid? "(?P<name>\\w+)")  ; => #t (named capture)
```

#### Use Case

Validate user-provided regex patterns before use.

---

## Special Forms

Core language forms that are evaluated specially rather than as function calls. Special forms control if and when their arguments are evaluated, unlike normal functions which evaluate all arguments first.

### `quote`

Prevent evaluation. Returns the literal datum without evaluating it.

Shorthand syntax: `'expr` is equivalent to `(quote expr)`.

#### Parameters

- `expr` - Any expression (returned unevaluated)

#### Examples

```lisp
(quote (1 2 3))    ; => (1 2 3)
'(1 2 3)           ; => (1 2 3) (shorthand)
'x                 ; => x (symbol, not evaluated)
```

### `quasiquote`

Quote with selective evaluation. Allows embedding evaluated values using `unquote` (`,`) and `unquote-splicing` (`,@`). Essential for writing macros.

Shorthand syntax: `` `expr`` is equivalent to `(quasiquote expr)`.

#### Parameters

- `expr` - Template expression with unquote markers

#### Examples

```lisp
(define x 42)
`(1 ,x 3)              ; => (1 42 3)
`(result is ,(+ 10 20)) ; => (result is 30)

(define lst '(a b c))
`(1 ,@lst 4)           ; => (1 a b c 4)
`(,@lst ,@lst)         ; => (a b c a b c)
```

### `if`

Conditional evaluation. Evaluates `condition`; if truthy, evaluates and returns the `then` branch, otherwise evaluates and returns the `else` branch.

Only `nil` is falsy; all other values (including `0`, `""`, and `()`) are truthy.

#### Parameters

- `condition` - Test expression
- `then` - Expression to evaluate if condition is truthy
- `else` - Expression to evaluate if condition is falsy (optional, defaults to nil)

#### Examples

```lisp
(if (> 5 3) "yes" "no")  ; => "yes"
(if (< 5 3) "yes" "no")  ; => "no"
(if nil "truthy" "falsy") ; => "falsy"
(if 0 "truthy" "falsy")   ; => "truthy" (only nil is falsy)
(if "" "truthy" "falsy")  ; => "truthy" (empty strings are truthy)
```

### `define`

Define or update a variable in the current environment. The symbol name is not evaluated.

When the value is a lambda, the lambda is automatically named after the symbol.

#### Parameters

- `name` - Symbol name (not evaluated)
- `value` - Expression to evaluate and bind to the name

#### Examples

```lisp
(define x 42)             ; => x
x                         ; => 42
(define y (+ x 8))        ; => y
y                         ; => 50
(define double (lambda (x) (* x 2)))
(function-name double)    ; => "double"
```

### `set!`

Mutate an existing variable. The variable must already be defined in the current environment or an enclosing scope.

#### Parameters

- `name` - Symbol name (not evaluated)
- `value` - Expression to evaluate and assign to the name

#### Examples

```lisp
(define count 0)
(set! count 10)
count                     ; => 10

(define increment (lambda ()
  (set! count (+ count 1))
  count))
(increment)               ; => 11
(increment)               ; => 12
```

### `lambda`

Create an anonymous function (closure). Supports required, optional (`&optional`), and rest (`&rest`) parameters. The body has implicit `progn`: all expressions are evaluated, returning the last value with tail recursion optimization.

An optional docstring may be provided as the first body expression.

#### Parameters

- `params` - Parameter list with required, `&optional`, and `&rest` markers
- `body...` - Expressions evaluated sequentially; last value returned

#### Examples

```lisp
((lambda (x) (* x 2)) 5)                        ; => 10
((lambda (a b) (+ a b)) 3 4)                    ; => 7
((lambda (x) (+ x 1) (+ x 2) (* x 3)) 5)        ; => 15 (implicit progn)

((lambda (a &optional b) (list a b)) 1)         ; => (1 nil)
((lambda (a &optional b) (list a b)) 1 2)       ; => (1 2)

((lambda (a &rest more) (list a more)) 1 2 3)   ; => (1 (2 3))
((lambda (&rest all) all) 1 2 3 4)              ; => (1 2 3 4)

(define make-adder (lambda (x) (lambda (y) (+ x y))))
(define add5 (make-adder 5))
(add5 10)                                        ; => 15
```

### `defmacro`

Define a macro that transforms code at evaluation time. Macros receive arguments unevaluated; the macro body produces an expansion which is then evaluated in the caller's context.

Supports rest parameters using dotted parameter list syntax.

#### Parameters

- `name` - Symbol name for the macro (not evaluated)
- `params` - Parameter list (arguments passed unevaluated)
- `body...` - Expressions producing the macro expansion

#### Examples

```lisp
(defmacro double (x) (list '+ x x))
(double 5)          ; => 10
(double (+ 2 3))    ; => 10

(defmacro when (condition body)
  (list 'if condition body nil))
(when #t 42)        ; => 42
(when #f 42)        ; => nil

(defmacro my-progn (first . rest)
  (cons 'progn (cons first rest)))
```

### `let`

Create local variable bindings with parallel evaluation. All init expressions are evaluated before any variable is bound. Body has implicit `progn`.

#### Parameters

- `bindings` - List of `(name init-expr)` pairs
- `body...` - Expressions evaluated in the new scope; last value returned

#### Examples

```lisp
(let ((x 10) (y 20))
  (+ x y))                 ; => 30

(let ((a 1) (b 2))
  (* a b))                 ; => 2
```

### `let*`

Create local variable bindings with sequential evaluation. Each binding can reference previous bindings in the same `let*` form. Body has implicit `progn`.

#### Parameters

- `bindings` - List of `(name init-expr)` pairs, evaluated in order
- `body...` - Expressions evaluated in the new scope; last value returned

#### Examples

```lisp
(let* ((x 5) (y (+ x 3))) y)      ; => 8

(let* ((a 10)
       (b (* a 2))
       (c (+ b 5)))
  c)                              ; => 25

(let ((x 5) (y (+ x 3))) y)      ; ERROR: x undefined in parallel let
```

### `progn`

Evaluate multiple expressions sequentially and return the last value.

#### Parameters

- `body...` - Zero or more expressions to evaluate in order

#### Examples

```lisp
(progn 1 2 3)                    ; => 3
(progn
  (define x 10)
  (define y 20)
  (+ x y))                       ; => 30
(progn)                          ; => nil
```

### `do`

Iteration loop with variable bindings, step expressions, and an exit condition.

#### Syntax

```lisp
(do ((var init step) ...)
    (test result-expr ...)
  body ...)
```

#### Parameters

- `bindings` - List of `(var init step)` triples; `step` is optional (variable stays constant if omitted)
- `test-clause` - `(test result-expr ...)` evaluated at loop start; when truthy, result expressions are evaluated as implicit `progn`
- `body...` - Evaluated each iteration for side effects when test is falsy

#### Examples

```lisp
(do ((i 0 (+ i 1)))
    ((>= i 10) i))               ; => 10

(do ((i n (- i 1))
     (acc 1 (* acc i)))
    ((<= i 0) acc))              ; factorial

(do ((i 1 (+ i 1))
     (sum 0))
    ((> i 10) sum)
  (set! sum (+ sum i)))          ; => 55 (sum 1 to 10)
```

### `cond`

Multi-way conditional. Evaluates test expressions in order; the first truthy test's body expressions are evaluated as implicit `progn` and returned. Use `else` as the final test for a default case.

#### Parameters

- `clauses...` - Each clause is `(test body...)`; `else` as test always matches

#### Examples

```lisp
(cond
  ((>= score 90) "A")
  ((>= score 80) "B")
  ((>= score 70) "C")
  (else "F"))

(cond
  ((> x 0) "positive")
  ((< x 0) "negative")
  (else "zero"))
```

### `case`

Value-based dispatch. Evaluates the key expression once, then matches it against literal value lists using `eq?` semantics.

#### Parameters

- `key` - Expression evaluated once to produce the dispatch value
- `clauses...` - Each clause is `((values...) body...)` or `(else body...)`

#### Examples

```lisp
(case n
  ((1) "Monday")
  ((2) "Tuesday")
  ((6 7) "Weekend")
  (else "Invalid"))

(case #\a
  ((#\a #\e #\i #\o #\u) "vowel")
  (else "consonant"))
```

### `and`

Logical AND with short-circuit evaluation. Returns the last truthy value, or the first falsy value. Stops evaluating as soon as a falsy value is encountered.

#### Parameters

- `expressions...` - Zero or more expressions evaluated left to right

#### Examples

```lisp
(and)                ; => #t (empty and is true)
(and 1 2 3)          ; => 3 (returns last truthy value)
(and 1 nil 3)        ; => nil (short-circuits at nil)
(and #f (/ 1 0))     ; => #f (does not evaluate division)
```

### `or`

Logical OR with short-circuit evaluation. Returns the first truthy value, or the last falsy value if all are falsy. Stops evaluating as soon as a truthy value is encountered.

#### Parameters

- `expressions...` - Zero or more expressions evaluated left to right

#### Examples

```lisp
(or)                 ; => nil (empty or is false)
(or nil nil 5)       ; => 5 (first truthy value)
(or 1 2 3)           ; => 1 (short-circuits at first truthy)
(or #f "foo" (/ 1 0)) ; => "foo" (does not evaluate division)
```

### `condition-case`

Catch and handle errors by type (Emacs Lisp-style exception handling). Evaluates the body form; if an error is signaled, the handlers are checked in order and the first matching handler is executed.

#### Parameters

- `var` - Symbol bound to the error object during handler execution (not evaluated)
- `body` - Expression to evaluate (may signal an error)
- `handlers...` - Each handler is `(error-type body...)`; `error` catches all types

#### Examples

```lisp
(condition-case e
    (/ 10 0)
  (division-by-zero "Caught division by zero"))
; => "Caught division by zero"

(condition-case e
    (signal 'my-error "test message")
  (error (error-message e)))
; => "test message"

(condition-case e
    (some-risky-operation)
  (file-error "File problem")
  (network-error "Network problem")
  (error "Other error"))
```

### `unwind-protect`

Guarantee cleanup code execution regardless of whether the body form succeeds or signals an error. Like `try-finally` in other languages.

#### Parameters

- `body` - Expression to evaluate (may signal an error)
- `cleanup...` - Expressions evaluated after body completes, regardless of success or error

#### Examples

```lisp
(unwind-protect
    (read-line file)
  (close file))  ; Always closes, even on error

(unwind-protect
    (signal 'test-error "boom")
  (define cleaned-up #t))
cleaned-up  ; => #t (cleanup ran despite error)
```

---

## String Ports

Input ports give O(1) sequential character access to strings; output ports accumulate written text into a buffer for O(n) string building.

### `open-input-string`

Create a string port for reading characters from a string.

#### Parameters

- `string` - The source string to read from

#### Returns

A string port that provides O(1) sequential access to characters.

#### Examples

```lisp
(define p (open-input-string "hello"))
(port-peek-char p)  ; => #\h
(port-read-char p)  ; => #\h (advances position)
```

#### See Also

- `port-peek-char` - Peek at current character
- `port-read-char` - Read and advance

### `port-peek-char`

Return current character without advancing position.

#### Parameters

- `port` - A string port

#### Returns

Current character, or `nil` at end of string.

#### Examples

```lisp
(define p (open-input-string "hi"))
(port-peek-char p)  ; => #\h
(port-peek-char p)  ; => #\h (unchanged)
```

#### Notes

O(1) operation — does not walk the string.

### `port-read-char`

Read current character and advance position.

#### Parameters

- `port` - A string port

#### Returns

Current character, or `nil` at end of string.

#### Examples

```lisp
(define p (open-input-string "hi"))
(port-read-char p)  ; => #\h
(port-read-char p)  ; => #\i
(port-read-char p)  ; => nil
```

#### Notes

O(1) operation — does not walk the string.

### `port-position`

Return current character position in the port.

#### Parameters

- `port` - A string port

#### Returns

Integer position (0-based).

#### Examples

```lisp
(define p (open-input-string "hello"))
(port-position p)   ; => 0
(port-read-char p)
(port-position p)   ; => 1
```

### `port-source`

Return the source string of the port.

#### Parameters

- `port` - A string port

#### Returns

The original string the port was created from.

#### Examples

```lisp
(define p (open-input-string "hello"))
(port-source p)     ; => "hello"
```

#### Notes

Useful for substring extraction combined with `port-position`.

### `port-eof?`

Test if port is at end of string.

#### Parameters

- `port` - A string port

#### Returns

`#t` if all characters have been read, `#f` otherwise.

#### Examples

```lisp
(define p (open-input-string "a"))
(port-eof? p)       ; => #f
(port-read-char p)
(port-eof? p)       ; => #t
```

### `open-output-string`

Create an empty output string port that accumulates written text in an internal buffer.

#### Parameters

None.

#### Returns

A new output string port.

#### Examples

```lisp
(define out (open-output-string))
(port-write-string out "hello")
(get-output-string out)  ; => "hello"
```

#### Notes

Building a string by repeatedly writing to an output port is O(n) overall, versus the O(n²) cost of repeated `concat`.

#### See Also

- `port-write-string` - Append a string
- `port-write-char` - Append a character
- `get-output-string` - Retrieve the accumulated string

### `port-write-string`

Append a string to an output string port's buffer.

#### Parameters

- `port` - An output string port
- `string` - The string to append

#### Returns

`nil`.

#### Examples

```lisp
(define out (open-output-string))
(port-write-string out "foo")
(port-write-string out "bar")
(get-output-string out)  ; => "foobar"
```

#### Errors

Returns an error if `port` is not an output string port, or if `string` is not a string.

### `port-write-char`

Append a single character to an output string port's buffer.

#### Parameters

- `port` - An output string port
- `char` - The character to append

#### Returns

`nil`.

#### Examples

```lisp
(define out (open-output-string))
(port-write-char out #\h)
(port-write-char out #\i)
(get-output-string out)  ; => "hi"
```

#### Errors

Returns an error if `port` is not an output string port, or if `char` is not a character.

### `get-output-string`

Return the accumulated contents of an output string port as a string.

#### Parameters

- `port` - An output string port

#### Returns

A string containing everything written to the port so far.

#### Examples

```lisp
(define out (open-output-string))
(port-write-string out "a")
(port-write-char out #\b)
(get-output-string out)  ; => "ab"
```

#### Notes

Does not reset the port; further writes continue appending to the same buffer.

#### Errors

Returns an error if `port` is not an output string port.

### `string-port?`

Test if a value is a string port (input or output).

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is a string port, `#f` otherwise.

#### Examples

```lisp
(string-port? (open-input-string "hi"))  ; => #t
(string-port? "hello")                   ; => #f
```

---

## Strings

String manipulation functions. All operations are UTF-8 aware.

### `concat`

Concatenate strings together.

#### Parameters

- `strings...` - Zero or more strings to concatenate

#### Returns

A new string formed by concatenating all arguments. Returns empty string `""` if no arguments provided.

#### Examples

```lisp
(concat "Hello" " " "World")     ; => "Hello World"
(concat "foo" "bar" "baz")      ; => "foobarbaz"
(concat)                          ; => ""
(concat "test")                   ; => "test"
```

### `substring`

Extract a substring by character indices (UTF-8 aware).

#### Parameters

- `string` - The input string
- `start` - Starting character index (0-based, inclusive)
- `end` - Ending character index (0-based, exclusive)

#### Returns

A new string containing characters from `start` to `end-1`. Returns empty string if `start >= end`.

#### Examples

```lisp
(substring "Hello" 0 5)          ; => "Hello"
(substring "Hello" 1 4)          ; => "ell"
(substring "Hello, 世界!" 7 9)    ; => "世界" (UTF-8 aware)
```

#### Notes

- Indices are **character** positions, not byte positions
- Works correctly with multi-byte UTF-8 characters
- Index out of bounds returns error

### `string-ref`

Get character at index from string (UTF-8 aware).

#### Parameters

- `string` - The input string
- `index` - Character index (0-based)

#### Returns

The character at the specified index (a `LISP_CHAR` value, e.g. `#\h`).

#### Examples

```lisp
(string-ref "hello" 0)          ; => #\h
(string-ref "hello" 4)          ; => #\o
(string-ref "世界" 0)            ; => #\世 (UTF-8)
```

#### Notes

- Index is **character** position, not byte position
- Works correctly with multi-byte UTF-8 characters
- Returns error if index out of bounds

#### See Also

- `substring` - Extract substring by range
- `length` - Get length of sequence (list, string, or vector)

### `split`

Split string by pattern (supports wildcards).

#### Parameters

- `string` - The string to split
- `pattern` - Pattern to split on (supports wildcards: `*`, `?`, `[abc]`)

#### Returns

List of strings split by pattern.

#### Examples

```lisp
(split "a,b,c" ",")               ; => ("a" "b" "c")
(split "foo*bar*baz" "*")         ; => ("foo" "bar" "baz")
(split "hello world" " ")        ; => ("hello" "world")
```

### `join`

Join a list of strings with a separator.

#### Parameters

- `list` - List of strings to join
- `separator` - String to insert between elements

#### Returns

A single string with all elements joined.

#### Examples

```lisp
(join '("a" "b" "c") ",")       ; => "a,b,c"
(join '("hello" "world") " ")  ; => "hello world"
```

### `number->string`

Convert number to string with optional radix.

#### Parameters

- `number` - Integer or float to convert
- `radix` - Optional base (2-36, defaults to 10)

#### Returns

String representation of the number.

#### Examples

```lisp
(number->string 42)          ; => "42"
(number->string 3.14159)     ; => "3.14159"
(number->string 255 16)      ; => "ff" (hexadecimal)
(number->string 8 2)         ; => "1000" (binary)
```

#### Notes

- Floats only supported in base 10
- For other bases, number is converted to integer first

#### See Also

- `string->number` - Convert string to number

### `string->number`

Convert string to number with optional radix.

#### Parameters

- `string` - String to parse
- `radix` - Optional base (2-36, defaults to 10)

#### Returns

Number parsed from string, or `nil` if invalid.

#### Examples

```lisp
(string->number "42")           ; => 42
(string->number "3.14")         ; => 3.14
(string->number "ff" 16)        ; => 255 (hexadecimal)
(string->number "#xff")         ; => 255 (auto-detect hex prefix)
(string->number "invalid")      ; => nil
```

#### Notes

- Supports prefixes: `#b` (binary), `#o` (octal), `#d` (decimal), `#x` (hex)
- Floats only supported in base 10
- Ignores leading/trailing whitespace

#### See Also

- `number->string` - Convert number to string

### `string-contains?`

Test if string contains substring.

#### Parameters

- `haystack` - String to search in
- `needle` - Substring to find

#### Returns

`#t` if `needle` is found anywhere in `haystack`, `nil` otherwise.

#### Examples

```lisp
(string-contains? "hello world" "world")   ; => #t
(string-contains? "hello world" "planet")  ; => nil
(string-contains? "test" "")               ; => #t (empty string)
```

#### See Also

- `string-index` - Find position of substring
- `string-prefix?` - Test if string starts with prefix

### `string-index`

Find character index of first occurrence of substring.

#### Parameters

- `haystack` - String to search in
- `needle` - Substring to find

#### Returns

Character index (0-based) of first occurrence, or `nil` if not found.

#### Examples

```lisp
(string-index "hello world" "world")    ; => 6
(string-index "hello" "l")              ; => 2 (first 'l')
(string-index "test" "xyz")             ; => nil
```

#### Notes

Returns character index, not byte offset (UTF-8 aware).

#### See Also

- `string-contains?` - Test for substring presence

### `string-match?`

Test if string matches wildcard pattern.

#### Parameters

- `string` - String to test
- `pattern` - Wildcard pattern

#### Returns

`#t` if string matches pattern, `nil` otherwise.

#### Pattern Syntax

- `*` - Match zero or more characters
- `?` - Match exactly one character
- `[abc]` - Match any character in set
- `[a-z]` - Match any character in range
- `[!abc]` - Match any character NOT in set

#### Examples

```lisp
(string-match? "hello" "h*o")        ; => #t
(string-match? "test.txt" "*.txt")   ; => #t
(string-match? "file" "f?le")        ; => #t
(string-match? "abc" "[a-z]*")       ; => #t
```

#### See Also

- `regex-match?` - Full regex pattern matching
- `string-prefix?` - Test for exact prefix

### `string-prefix?`

Test if string starts with prefix.

#### Parameters

- `prefix` - Prefix to test for
- `string` - String to test

#### Returns

`#t` if `string` starts with `prefix`, `nil` otherwise.

#### Examples

```lisp
(string-prefix? "hello" "hello world")   ; => #t
(string-prefix? "world" "hello world")   ; => nil
(string-prefix? "" "anything")           ; => #t (empty prefix)
```

#### See Also

- `string-contains?` - Test for substring anywhere
- `string-match?` - Wildcard pattern matching

### `string-replace`

Replace all occurrences of substring in string.

#### Parameters

- `string` - The input string
- `old` - Substring to find
- `new` - Substring to replace with

#### Returns

New string with all occurrences of `old` replaced by `new`.

#### Examples

```lisp
(string-replace "hello world" "world" "universe")
  ; => "hello universe"
(string-replace "hello" "l" "L")
  ; => "heLLo" (all occurrences)
```

### `string-upcase`

Convert string to uppercase (UTF-8 aware, ASCII only).

#### Parameters

- `string` - The input string

#### Returns

New string with all ASCII letters converted to uppercase. Non-ASCII UTF-8 characters are preserved unchanged.

#### Examples

```lisp
(string-upcase "hello world")    ; => "HELLO WORLD"
(string-upcase "Hello123")       ; => "HELLO123"
```

#### Notes

Only converts ASCII letters (a-z). Non-ASCII characters are preserved (UTF-8 safe) but not case-converted.

### `string-downcase`

Convert string to lowercase (UTF-8 aware, ASCII only).

#### Parameters

- `string` - The input string

#### Returns

New string with all ASCII letters converted to lowercase. Non-ASCII UTF-8 characters are preserved unchanged.

#### Examples

```lisp
(string-downcase "HELLO WORLD")  ; => "hello world"
(string-downcase "Hello123")     ; => "hello123"
```

#### Notes

Only converts ASCII letters (A-Z). Non-ASCII characters are preserved (UTF-8 safe) but not case-converted.

### `string-trim`

Remove leading and trailing whitespace from a string.

#### Parameters

- `string` - The input string

#### Returns

New string with leading and trailing whitespace removed.

#### Examples

```lisp
(string-trim "  hello  ")      ; => "hello"
(string-trim "\thello\
")      ; => "hello"
(string-trim "hello")           ; => "hello"
(string-trim "   ")             ; => ""
```

#### Notes

Whitespace includes space, tab, newline, carriage return, form feed, vertical tab.

### `string<?`

Test if strings are in lexicographic order (less than).

#### Parameters

- `str1` - First string
- `str2` - Second string

#### Returns

`#t` if `str1` is lexicographically less than `str2`, `nil` otherwise.

#### Examples

```lisp
(string<? "apple" "banana")     ; => #t
(string<? "zebra" "ant")        ; => nil
(string<? "abc" "abc")          ; => nil (equal)
```

#### Notes

Comparison is byte-by-byte (ASCII/UTF-8 code point order).

#### See Also

- `string>?` - Greater than
- `string=?` - Equality

### `string>?`

Test if strings are in reverse lexicographic order (greater than).

#### Parameters

- `str1` - First string
- `str2` - Second string

#### Returns

`#t` if `str1` is lexicographically greater than `str2`, `nil` otherwise.

#### Examples

```lisp
(string>? "banana" "apple")     ; => #t
(string>? "ant" "zebra")        ; => nil
(string>? "abc" "abc")          ; => nil (equal)
```

### `string<=?`

Test if strings are in non-decreasing lexicographic order (less than or equal).

#### Parameters

- `str1` - First string
- `str2` - Second string

#### Returns

`#t` if `str1` is lexicographically less than or equal to `str2`, `nil` otherwise.

#### Examples

```lisp
(string<=? "apple" "banana")    ; => #t
(string<=? "abc" "abc")         ; => #t (equal)
(string<=? "zebra" "ant")       ; => nil
```

### `string>=?`

Test if strings are in non-increasing lexicographic order (greater than or equal).

#### Parameters

- `str1` - First string
- `str2` - Second string

#### Returns

`#t` if `str1` is lexicographically greater than or equal to `str2`, `nil` otherwise.

#### Examples

```lisp
(string>=? "banana" "apple")    ; => #t
(string>=? "abc" "abc")         ; => #t (equal)
(string>=? "ant" "zebra")       ; => nil
```

### Standard Library Aliases

The following aliases are defined in the standard library for naming consistency with other Scheme implementations. They are not separate builtins — they resolve to the same function via `defalias`.

- `string-append` - Alias for `concat`
- `string-split` - Alias for `split`
- `string-join` - Alias for `join`
- `string-length` - Alias for `length` (also works on lists and vectors)

---

## Symbols

Symbol and keyword operations.

See also: `symbol?` in [Type Predicates](doc/type-predicates.md), `documentation` and `set-documentation!` in [Functions](doc/functions.md).

### `symbol->string`

Convert symbol to string.

#### Parameters

- `symbol` - Symbol to convert

#### Returns

String containing the symbol's name.

#### Examples

```lisp
(symbol->string 'hello)      ; => "hello"
(symbol->string '+)           ; => "+"
(symbol->string 'my-var)      ; => "my-var"

; Using with variables
(define x 'test)
(symbol->string x)            ; => "test"
```

#### Errors

Returns error if argument is not a symbol.

#### See Also

- `symbol?` - Test if value is symbol
- `string->symbol` - Convert string to symbol

### `string->symbol`

Convert string to interned symbol.

#### Parameters

- `string` - String containing symbol name

#### Returns

Interned symbol with the given name.

#### Examples

```lisp
(string->symbol "hello")     ; => hello
(string->symbol "+")         ; => +
(string->symbol "my-var")    ; => my-var

; Symbols are interned
(eq? (string->symbol "foo") 'foo)  ; => #t
```

#### Errors

Returns error if argument is not a string.

#### See Also

- `symbol?` - Test if value is symbol
- `symbol->string` - Convert symbol to string

### `keyword-name`

Get the name of a keyword without the leading colon.

#### Parameters

- `keyword` - A keyword value

#### Returns

A string containing the keyword's name without the colon prefix.

#### Examples

```lisp
(keyword-name :foo)      ; => "foo"
(keyword-name :bar-baz)  ; => "bar-baz"
```

#### See Also

- `keyword?` - Check if value is keyword
- `symbol->string` - Convert symbol to string

---

## Time and Profiling

Timing and performance profiling.

### `current-time-ms`

Return current time in milliseconds.

#### Returns

Integer representing current monotonic time in milliseconds since an arbitrary epoch.

#### Examples

```lisp
(current-time-ms)        ; => 1234567890
(let ((start (current-time-ms)))
  (sleep 100)
  (- (current-time-ms) start))  ; => ~100
```

#### Notes

Uses monotonic clock. Intended for measuring elapsed time, not wall-clock time.

### `profile-start`

Start the profiler and clear previous data.

#### Returns

`#t`

#### Examples

```lisp
(profile-start)
(my-expensive-function)
(profile-stop)
(profile-report)         ; => list of (name calls ms)
```

#### Notes

Enables function-level profiling and resets all accumulated data.
While profiling is active, every function call's elapsed time is tracked.

#### See Also

- `profile-stop` - Stop profiling
- `profile-report` - View results

### `profile-stop`

Stop the profiler.

#### Returns

`#t`

#### Notes

Disables profiling. Data is preserved until `profile-start` or `profile-reset`.

#### See Also

- `profile-start` - Start profiling
- `profile-report` - View results

### `profile-report`

Return profiling results as a list.

#### Returns

List of `(name calls ms)` entries sorted by total time (descending):

- `name` - Function name (string)
- `calls` - Number of calls (integer)
- `ms` - Total time in milliseconds (float)

#### Examples

```lisp
(profile-report)
; => (("format-sexp" 12847 1523.45) ("concat" 98234 452.12) ...)
```

#### See Also

- `profile-start` - Start profiling
- `profile-reset` - Clear data

### `profile-reset`

Clear all profiling data.

#### Returns

`#t`

#### Notes

Clears accumulated profiling data without stopping the profiler.

#### See Also

- `profile-start` - Start profiling

---

## Type Predicates

Functions for testing the type of a value.

### `null?`

Check if a value is nil (null/empty).

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is `nil`, `#f` otherwise.

#### Examples

```lisp
(null? nil)          ; => #t
(null? #f)           ; => #t (nil and #f are the same)
(null? '())          ; => #t (empty list is nil)
(null? 0)            ; => #f (0 is truthy)
(null? "")           ; => #f (empty string is truthy)
```

### `atom?`

Check if a value is an atom (not a list).

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is an atom (not a cons cell), `#f` otherwise.

#### Examples

```lisp
(atom? 42)           ; => #t
(atom? "hello")      ; => #t
(atom? nil)          ; => #t
(atom? '(1 2 3))     ; => #f (lists are not atoms)
```

### `pair?`

Test if object is a cons cell (pair).

#### Parameters

- `obj` - Object to test

#### Returns

`#t` if object is a cons cell (pair), `#f` otherwise.

#### Examples

```lisp
(pair? '(1 . 2))        ; => #t
(pair? '(a b c))        ; => #t
(pair? nil)             ; => #f (nil is not a pair)
(pair? 42)              ; => #f
(pair? "string")       ; => #f
```

#### Notes

- A pair is a cons cell (`LISP_CONS` type)
- `nil` is NOT a pair (unlike `list?` which returns true for both `nil` and cons cells)
- Useful for checking if an alist element is a key-value pair

#### See Also

- `list?` - Test if object is a list (nil or cons cell)
- `cons` - Create a cons cell

### `integer?`

Check if a value is an integer.

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is a 64-bit integer, `#f` otherwise.

#### Examples

```lisp
(integer? 42)        ; => #t
(integer? -100)      ; => #t
(integer? 3.14)      ; => #f (float)
(integer? "42")      ; => #f (string)
```

### `boolean?`

Check if a value is a boolean (#t or #f).

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is `#t` or `#f`, `#f` otherwise.

#### Examples

```lisp
(boolean? #t)        ; => #t
(boolean? #f)        ; => #t
(boolean? nil)       ; => #t (nil is #f)
(boolean? 1)         ; => #f
```

### `number?`

Check if a value is a number (integer or float).

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is an integer or float, `#f` otherwise.

#### Examples

```lisp
(number? 42)         ; => #t (integer)
(number? 3.14)       ; => #t (float)
(number? "42")       ; => #f (string)
```

### `vector?`

Check if a value is a vector.

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is a vector, `#f` otherwise.

#### Examples

```lisp
(vector? (make-vector 5))    ; => #t
(vector? #())                ; => #t (empty vector literal)
(vector? '(1 2 3))           ; => #f (list)
```

### `hash-table?`

Test if value is a hash table.

#### Parameters

- `value` - Value to test

#### Returns

`#t` if value is a hash table, `nil` otherwise.

#### Examples

```lisp
(define ht (make-hash-table))
(hash-table? ht)        ; => #t
(hash-table? '(1 2 3))  ; => nil
(hash-table? 42)        ; => nil
```

#### See Also

- `make-hash-table` - Create hash table
- `vector?` - Test if value is vector
- `list?` - Test if value is list

### `string?`

Check if a value is a string.

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is a string, `#f` otherwise.

#### Examples

```lisp
(string? "hello")    ; => #t
(string? "")         ; => #t (empty string)
(string? 'hello)     ; => #f (symbol)
(string? 42)         ; => #f
```

### `symbol?`

Check if a value is a symbol.

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is a symbol, `#f` otherwise.

#### Examples

```lisp
(symbol? 'foo)       ; => #t
(symbol? '+)         ; => #t
(symbol? "foo")      ; => #f (string)
(symbol? 42)         ; => #f
```

### `keyword?`

Check if a value is a keyword.

Keywords are self-evaluating symbols that start with a colon (`:`).

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is a keyword, `nil` otherwise.

#### Examples

```lisp
(keyword? :foo)      ; => #t
(keyword? :bar-baz)  ; => #t
(keyword? 'foo)      ; => nil (symbol, not keyword)
(keyword? "foo")     ; => nil (string)
```

#### See Also

- `keyword-name` - Get keyword name without colon
- `symbol?` - Check for symbols

### `list?`

Check if a value is a list (nil or cons cell).

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is `nil` or a cons cell, `#f` otherwise.

#### Examples

```lisp
(list? '(1 2 3))     ; => #t
(list? nil)          ; => #t
(list? '())          ; => #t
(list? 42)           ; => #f
```

### `function?`

Check if a value is a lambda (user-defined function).

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is a lambda, `#f` otherwise.

#### Examples

```lisp
(function? (lambda (x) x))  ; => #t
(function? +)               ; => #f (builtin)
(function? 42)              ; => #f
```

### `macro?`

Check if a value is a macro.

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is a macro, `#f` otherwise.

#### Examples

```lisp
(macro? when)    ; => #t
(macro? +)       ; => #f
```

### `builtin?`

Check if a value is a builtin function.

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is a builtin, `#f` otherwise.

#### Examples

```lisp
(builtin? +)                  ; => #t
(builtin? (lambda (x) x))    ; => #f
```

### `callable?`

Check if a value is callable (lambda, macro, or builtin).

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is callable, `#f` otherwise.

#### Examples

```lisp
(callable? +)                  ; => #t
(callable? (lambda (x) x))    ; => #t
(callable? when)              ; => #t
(callable? 42)                ; => #f
```

### `regex?`

Check if a value is a compiled regex object (produced by `regex-compile`).

#### Parameters

- `value` - Any value to test

#### Returns

`#t` if the value is a compiled regex object, `#f` otherwise. A pattern string is not a regex object — use `regex-compile` to create one.

#### Examples

```lisp
(define digits (regex-compile "\\d+"))
(regex? digits)              ; => #t
(regex? "\\d+")             ; => #f (pattern string, not compiled regex)
(regex? 42)                 ; => #f
```

#### See Also

- `regex-compile` - Compile a pattern string into a regex object

---

## Vectors

Mutable, indexed array operations.

### `make-vector`

Create a new vector with specified size and optional initial value.

#### Parameters

- `size` - Number of elements (integer)
- `initial-value` - Optional value to initialize all elements (defaults to `nil`)

#### Returns

A new vector with `size` elements, all initialized to `initial-value`.

#### Examples

```lisp
(make-vector 5)          ; => #(nil nil nil nil nil)
(make-vector 3 0)        ; => #(0 0 0)
(make-vector 4 "x")      ; => #("x" "x" "x" "x")
```

### `vector-ref`

Get element at index from vector.

#### Parameters

- `vector` - The vector to access
- `index` - Zero-based index (integer)

#### Returns

The element at the specified index.

#### Examples

```lisp
(define v (make-vector 3 0))
(vector-ref v 0)         ; => 0
(vector-ref v 2)         ; => 0
```

#### Errors

Returns error if index is out of bounds.

### `vector-set!`

Set element at index in vector (mutating operation).

#### Parameters

- `vector` - The vector to modify
- `index` - Zero-based index (integer)
- `value` - Value to store at index

#### Returns

The value that was set.

#### Examples

```lisp
(define v (make-vector 3))
(vector-set! v 0 "a")    ; => "a"
(vector-set! v 1 "b")    ; => "b"
v                        ; => #("a" "b" nil)
```

#### Notes

- Modifies vector in place
- Returns error if index out of bounds

### `vector-push!`

Append element to end of vector (mutating operation).

#### Parameters

- `vector` - The vector to modify
- `value` - Value to append

#### Returns

The value that was pushed.

#### Examples

```lisp
(define v (make-vector 0))
(vector-push! v 10)      ; => 10
(vector-push! v 20)      ; => 20
v                        ; => #(10 20)
```

#### Notes

Vector grows dynamically to accommodate new elements.

### `vector-pop!`

Remove and return last element from vector (mutating operation).

#### Parameters

- `vector` - The vector to modify

#### Returns

The element that was removed from the end.

#### Examples

```lisp
(define v #(10 20 30))
(vector-pop! v)          ; => 30
(vector-pop! v)          ; => 20
v                        ; => #(10)
```

#### Errors

Returns error if vector is empty.
