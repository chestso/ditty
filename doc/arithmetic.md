# Arithmetic

Functions for numerical operations.

## Integer literals

Integers can be written in decimal or with a radix prefix.

```lisp
#xFF     ; hexadecimal => 255
#xff     ; lowercase hex => 255
#o10     ; octal => 8
#b11     ; binary => 3
#d42     ; decimal => 42
```

Negative radix literals are written with a leading `-` before the radix prefix:

```lisp
(- #xFF)  ; => -255
```

## `+`

Add numbers together.

### Parameters

- `numbers...` - Zero or more numbers to add

### Returns

Sum of all numbers. Returns `0` if no arguments provided.
If all arguments are integers, returns integer; otherwise returns float.

### Examples

```lisp
(+)              ; => 0
(+ 1 2 3)        ; => 6 (integer)
(+ 10 5)         ; => 15 (integer)
(+ 1.5 2.5)      ; => 4.0 (float)
(+ 1 2.5)        ; => 3.5 (mixed integer/float -> float)
```

## `-`

Subtract numbers or negate a single number.

### Parameters

- `number` - With one argument, returns the negation
- `minuend subtrahends...` - With multiple arguments, subtracts all subsequent numbers from the first

### Returns

Result of subtraction. Returns integer if all arguments are integers; otherwise returns float.

### Examples

```lisp
(- 5)            ; => -5 (unary negation)
(- 10 3)         ; => 7 (integer)
(- 10 3.5)       ; => 6.5 (float - mixed types)
(- 100 20 30)    ; => 50 (multiple subtractions)
```

## `*`

Multiply numbers together.

### Parameters

- `numbers...` - Zero or more numbers to multiply

### Returns

Product of all numbers. Returns `1` if no arguments provided.
If all arguments are integers, returns integer; otherwise returns float.

### Examples

```lisp
(*)              ; => 1
(* 2 3 4)        ; => 24 (integer)
(* 3 4.0)        ; => 12.0 (float - mixed types)
(* 5 -2)         ; => -10
```

## `/`

Divide numbers or compute reciprocal.

### Parameters

- `number` - With one argument, returns the reciprocal (1/number)
- `dividend divisors...` - With multiple arguments, divides the first number by all subsequent numbers

### Returns

Result of division. **Always returns a float**, even for integer arguments.

### Examples

```lisp
(/ 2)            ; => 0.5 (reciprocal: 1/2)
(/ 10 2)         ; => 5.0 (always float)
(/ 10 3)         ; => 3.333333...
(/ 100 2 5)      ; => 10.0 (multiple divisions: 100/2/5)
```

## `quotient`

Integer division - divide and truncate to integer.

### Parameters

- `dividend` - The number to be divided (integer or float)
- `divisor` - The number to divide by (integer or float)

### Returns

Integer result of division, truncated toward zero.

### Examples

```lisp
(quotient 10 3)      ; => 3
(quotient 17 5)      ; => 3
(quotient -17 5)     ; => -3
(quotient 10.8 3.2)  ; => 3
```

### See Also

- `remainder` - Get the remainder of integer division
- `/` - Regular division (returns float)

## `remainder`

Integer remainder (modulo operation).

### Parameters

- `dividend` - The number to be divided (integer or float)
- `divisor` - The number to divide by (integer or float)

### Returns

Integer remainder after division.

### Examples

```lisp
(remainder 17 5)     ; => 2
(remainder 10 3)     ; => 1
(remainder 20 5)     ; => 0
(remainder -17 5)    ; => -2
```

### See Also

- `quotient` - Integer division
- `/` - Regular division (returns float)

## `remainder`

Integer remainder (modulo operation).

### Parameters

- `dividend` - The number to be divided (integer or float)
- `divisor` - The number to divide by (integer or float)

### Returns

Integer remainder after division. The result has the same sign as the dividend.

### Examples

```lisp
(remainder 17 5)     ; => 2
(remainder 10 3)     ; => 1
(remainder 20 5)     ; => 0
(remainder -17 5)    ; => -2
```

### See Also

- `floor` - Floor division (rounds toward negative infinity)
- `modulo` - Modulo operation (result has same sign as divisor)
- `quotient` - Integer division
- `/` - Regular division (returns float)

## `floor`

Floor division - divide and round toward negative infinity.

### Parameters

- `dividend` - The number to be divided (integer or float)
- `divisor` - The number to divide by (integer or float)

### Returns

Integer result of division, rounded toward negative infinity.

### Examples

```lisp
(floor 7 3)      ; => 2
(floor -7 3)     ; => -3
(floor 7 -3)     ; => -3
(floor -7 -3)    ; => 2
```

### See Also

- `modulo` - Modulo operation (result has same sign as divisor)
- `quotient` - Integer division (truncates toward zero)
- `remainder` - Integer remainder (sign follows dividend)
- `/` - Regular division (returns float)

## `modulo`

Modulo operation - returns the remainder of division with the sign of the divisor.

### Parameters

- `dividend` - The number to be divided (integer or float)
- `divisor` - The number to divide by (integer or float)

### Returns

Integer remainder after division. The result has the same sign as the divisor.

### Examples

```lisp
(modulo 7 3)     ; => 1
(modulo -7 3)    ; => 2
(modulo 7 -3)    ; => -2
(modulo -7 -3)   ; => -1
```

### See Also

- `floor` - Floor division (rounds toward negative infinity)
- `quotient` - Integer division (truncates toward zero)
- `remainder` - Integer remainder (sign follows dividend)
- `/` - Regular division (returns float)

## `even?`

Test if number is even.

### Parameters

- `n` - Number to test (integer or float)

### Returns

`#t` if number is even, `#f` if odd.

### Examples

```lisp
(even? 4)    ; => #t
(even? 5)    ; => #f
(even? 0)    ; => #t
(even? -2)   ; => #t
(even? 3.0)  ; => #f (converts to integer)
```

### Notes

Converts floats to integers by truncation before testing.

### See Also

- `odd?` - Test if number is odd
- `quotient` - Integer division
- `remainder` - Integer remainder

## `odd?`

Test if number is odd.

### Parameters

- `n` - Number to test (integer or float)

### Returns

`#t` if number is odd, `#f` if even.

### Examples

```lisp
(odd? 5)     ; => #t
(odd? 4)     ; => #f
(odd? 1)     ; => #t
(odd? -3)    ; => #t
(odd? 2.0)   ; => #f (converts to integer)
```

### Notes

Converts floats to integers by truncation before testing.

### See Also

- `even?` - Test if number is even
- `quotient` - Integer division
- `remainder` - Integer remainder
