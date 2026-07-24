# String Ports

Input ports give O(1) sequential character access to strings; output ports accumulate written text into a buffer for O(n) string building.

## `open-input-string`

Create a string port for reading characters from a string.

### Parameters

- `string` - The source string to read from

### Returns

A string port that provides O(1) sequential access to characters.

### Examples

```lisp
(define p (open-input-string "hello"))
(port-peek-char p)  ; => #\h
(port-read-char p)  ; => #\h (advances position)
```

### See Also

- `port-peek-char` - Peek at current character
- `port-read-char` - Read and advance

## `port-peek-char`

Return current character without advancing position.

### Parameters

- `port` - A string port

### Returns

Current character, or `nil` at end of string.

### Examples

```lisp
(define p (open-input-string "hi"))
(port-peek-char p)  ; => #\h
(port-peek-char p)  ; => #\h (unchanged)
```

### Notes

O(1) operation — does not walk the string.

## `port-read-char`

Read current character and advance position.

### Parameters

- `port` - A string port

### Returns

Current character, or `nil` at end of string.

### Examples

```lisp
(define p (open-input-string "hi"))
(port-read-char p)  ; => #\h
(port-read-char p)  ; => #\i
(port-read-char p)  ; => nil
```

### Notes

O(1) operation — does not walk the string.

## `port-position`

Return current character position in the port.

### Parameters

- `port` - A string port

### Returns

Integer position (0-based).

### Examples

```lisp
(define p (open-input-string "hello"))
(port-position p)   ; => 0
(port-read-char p)
(port-position p)   ; => 1
```

## `port-source`

Return the source string of the port.

### Parameters

- `port` - A string port

### Returns

The original string the port was created from.

### Examples

```lisp
(define p (open-input-string "hello"))
(port-source p)     ; => "hello"
```

### Notes

Useful for substring extraction combined with `port-position`.

## `port-eof?`

Test if port is at end of string.

### Parameters

- `port` - A string port

### Returns

`#t` if all characters have been read, `#f` otherwise.

### Examples

```lisp
(define p (open-input-string "a"))
(port-eof? p)       ; => #f
(port-read-char p)
(port-eof? p)       ; => #t
```

## `open-output-string`

Create an empty output string port that accumulates written text in an internal buffer.

### Parameters

None.

### Returns

A new output string port.

### Examples

```lisp
(define out (open-output-string))
(port-write-string out "hello")
(get-output-string out)  ; => "hello"
```

### Notes

Building a string by repeatedly writing to an output port is O(n) overall, versus the O(n²) cost of repeated `concat`.

### See Also

- `port-write-string` - Append a string
- `port-write-char` - Append a character
- `get-output-string` - Retrieve the accumulated string

## `port-write-string`

Append a string to an output string port's buffer.

### Parameters

- `port` - An output string port
- `string` - The string to append

### Returns

`nil`.

### Examples

```lisp
(define out (open-output-string))
(port-write-string out "foo")
(port-write-string out "bar")
(get-output-string out)  ; => "foobar"
```

### Errors

Returns an error if `port` is not an output string port, or if `string` is not a string.

## `port-write-char`

Append a single character to an output string port's buffer.

### Parameters

- `port` - An output string port
- `char` - The character to append

### Returns

`nil`.

### Examples

```lisp
(define out (open-output-string))
(port-write-char out #\h)
(port-write-char out #\i)
(get-output-string out)  ; => "hi"
```

### Errors

Returns an error if `port` is not an output string port, or if `char` is not a character.

## `get-output-string`

Return the accumulated contents of an output string port as a string.

### Parameters

- `port` - An output string port

### Returns

A string containing everything written to the port so far.

### Examples

```lisp
(define out (open-output-string))
(port-write-string out "a")
(port-write-char out #\b)
(get-output-string out)  ; => "ab"
```

### Notes

Does not reset the port; further writes continue appending to the same buffer.

### Errors

Returns an error if `port` is not an output string port.

## `string-port?`

Test if a value is a string port (input or output).

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is a string port, `#f` otherwise.

### Examples

```lisp
(string-port? (open-input-string "hi"))  ; => #t
(string-port? "hello")                   ; => #f
```
