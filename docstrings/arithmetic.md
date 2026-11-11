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

For integer division, use `quotient` instead.

### Examples

```lisp
(/ 2)            ; => 0.5 (reciprocal: 1/2)
(/ 10 2)         ; => 5.0 (always float)
(/ 10 3)         ; => 3.333333...
(/ 100 2 5)      ; => 10.0 (multiple divisions: 100/2/5)
```

## `quotient`

Divide and truncate toward zero.

Use this instead of `/` when you need an integer result.

### Parameters

- `dividend` - The number to be divided (integer or float)
- `divisor` - The number to divide by (integer or float)

### Returns

Integer result of division, truncated toward zero. If either argument is a
float, the division is computed in floating point and then truncated. A
quotient too large for the integer range is returned as a float.

### Examples

```lisp
(quotient 10 3)      ; => 3
(quotient 17 5)      ; => 3
(quotient -17 5)     ; => -3
(quotient 7 2.0)     ; => 3
(quotient 7.5 2)     ; => 3 (float operands, integer result)
(quotient 10.8 3.2)  ; => 3
```

### Errors

Returns an error on division by zero, or if an argument is not a number.

### Notes

Float operands are no longer truncated first: `(quotient 7.5 2)` used to be
computed as `7 / 2` and now truncates the real quotient `3.75`.

### See Also

- `remainder` - Remainder of division (sign follows dividend)
- `/` - Regular division (returns float)

## `remainder`

Remainder of division. The result has the same sign as the dividend.

### Parameters

- `dividend` - The number to be divided (integer or float)
- `divisor` - The number to divide by (integer or float)

### Returns

Remainder after division, with the sign of the dividend. Integer arguments
return an integer; if either argument is a float, the result is a float.

### Examples

```lisp
(remainder 17 5)     ; => 2
(remainder 10 3)     ; => 1
(remainder 20 5)     ; => 0
(remainder -17 5)    ; => -2
(remainder 7.5 2)    ; => 1.5 (float)
(remainder -7.5 2)   ; => -1.5 (float)
```

### Errors

Returns an error on division by zero, or if an argument is not a number.

### Notes

Float operands are no longer truncated first: `(remainder 7.5 2)` used to
return `1` and now returns `1.5`.

### See Also

- `floor` - Floor division (rounds toward negative infinity)
- `modulo` - Modulo operation (result has same sign as divisor)
- `quotient` - Integer division
- `/` - Regular division (returns float)

## `floor`

Floor - round down, or divide and round toward negative infinity.

### Parameters

- `number` - With one argument, rounds that number down to an integer
- `dividend divisor` - With two arguments, divides and rounds the quotient toward negative infinity

### Returns

Integer result, rounded toward negative infinity. With two arguments, a float
quotient is computed first, then rounded. A value too large for the integer
range is returned as a float.

### Examples

```lisp
(floor 7 3)      ; => 2
(floor -7 3)     ; => -3
(floor 7 -3)     ; => -3
(floor -7 -3)    ; => 2
(floor 7.5)      ; => 7 (one argument)
(floor -7.5)     ; => -8
(floor 7.5 2)    ; => 3
(floor -7.5 2)   ; => -4
(floor 6.9 2.5)  ; => 2
(floor 7 2.0)    ; => 3
```

### Errors

Returns an error on division by zero, if the number of arguments is not one or
two, or if an argument is not a number.

### Notes

Two-argument float operands are no longer truncated first: `(floor 6.9 2.5)`
used to be computed as `6 / 2` and now floors the real quotient `2.76`.

### See Also

- `modulo` - Modulo operation (result has same sign as divisor)
- `quotient` - Integer division (truncates toward zero)
- `remainder` - Remainder of division (sign follows dividend)
- `ceiling` - Round up
- `/` - Regular division (returns float)

## `modulo`

Modulo operation - returns the remainder of division with the sign of the divisor.

### Parameters

- `dividend` - The number to be divided (integer or float)
- `divisor` - The number to divide by (integer or float)

### Returns

Remainder after division, with the sign of the divisor. Integer arguments
return an integer; if either argument is a float, the result is a float.

### Examples

```lisp
(modulo 7 3)     ; => 1
(modulo -7 3)    ; => 2
(modulo 7 -3)    ; => -2
(modulo -7 -3)   ; => -1
(modulo 7.5 2)   ; => 1.5 (float)
(modulo -7.5 2)  ; => 0.5 (float)
(modulo 7 2.5)   ; => 2 (float)
(modulo 6.0 2)   ; => 0 (float)
```

### Errors

Returns an error on division by zero, or if an argument is not a number.

### Notes

Float operands are no longer truncated first: `(modulo -7.5 2)` used to return
`1` and now returns `0.5`.

### See Also

- `floor` - Floor division (rounds toward negative infinity)
- `quotient` - Integer division (truncates toward zero)
- `remainder` - Remainder of division (sign follows dividend)
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

## `max`

Return the largest of the given numbers.

### Parameters

- `numbers...` - One or more numbers to compare

### Returns

The maximum value. Returns integer if all arguments are integers; otherwise returns float.

### Examples

```lisp
(max 1 3 2)      ; => 3 (integer)
(max -5 -2 -10)  ; => -2 (integer)
(max 1 2.5 2)    ; => 2.5 (float)
(max 42)         ; => 42 (single argument)
```

### Errors

Returns error if no arguments are provided or if any argument is not a number.

### See Also

- `min` - Return the smallest number
- `>` - Greater-than comparison

## `min`

Return the smallest of the given numbers.

### Parameters

- `numbers...` - One or more numbers to compare

### Returns

The minimum value. Returns integer if all arguments are integers; otherwise returns float.

### Examples

```lisp
(min 1 3 2)      ; => 1 (integer)
(min -5 -2 -10)  ; => -10 (integer)
(min 1 2.5 2)    ; => 1.0 (float)
(min 42)         ; => 42 (single argument)
```

### Errors

Returns error if no arguments are provided or if any argument is not a number.

### See Also

- `max` - Return the largest number
- `<` - Less-than comparison

## `logand`

Bitwise AND of integers.

### Parameters

- `integers...` - Zero or more integers

### Returns

Bitwise AND of all arguments. With no arguments, returns -1 (all bits set, the identity element).

### Examples

```lisp
(logand 15 7)        ; => 7  (0b1111 & 0b0111)
(logand 12 10)       ; => 8  (0b1100 & 0b1010 = 0b1000)
(logand)             ; => -1 (identity)
(logand 42)          ; => 42 (single argument)
(logand -1 6)        ; => 6  (-1 has all bits set)
```

### Errors

Returns error if any argument is not an integer.

### See Also

- `logior` - Bitwise inclusive OR
- `logxor` - Bitwise exclusive OR
- `lognot` - Bitwise NOT

## `logior`

Bitwise inclusive OR of integers.

### Parameters

- `integers...` - Zero or more integers

### Returns

Bitwise inclusive OR of all arguments. With no arguments, returns 0 (no bits set, the identity element).

### Examples

```lisp
(logior 12 10)       ; => 14 (0b1100 | 0b1010 = 0b1110)
(logior 8 1)         ; => 9  (0b1000 | 0b0001 = 0b1001)
(logior)             ; => 0  (identity)
(logior 1 2 4 8)     ; => 15 (variadic)
```

### Errors

Returns error if any argument is not an integer.

### See Also

- `logand` - Bitwise AND
- `logxor` - Bitwise exclusive OR

## `logxor`

Bitwise exclusive OR (XOR) of integers.

### Parameters

- `integers...` - Zero or more integers

### Returns

Bitwise exclusive OR of all arguments. With no arguments, returns 0 (the identity element).

### Examples

```lisp
(logxor 12 10)       ; => 6  (0b1100 ^ 0b1010 = 0b0110)
(logxor 8 1)         ; => 9  (0b1000 ^ 0b0001 = 0b1001)
(logxor 42 42)       ; => 0  (x ^ x = 0)
(logxor)             ; => 0  (identity)
```

### Errors

Returns error if any argument is not an integer.

### See Also

- `logand` - Bitwise AND
- `logior` - Bitwise inclusive OR

## `lognot`

Bitwise NOT (complement) of an integer.

### Parameters

- `integer` - An integer

### Returns

The bitwise complement of the integer. Equivalent to `-(n+1)` in two's complement.

### Examples

```lisp
(lognot 0)           ; => -1  (all bits flipped to 1)
(lognot -1)          ; => 0   (all bits flipped to 0)
(lognot 5)           ; => -6
(lognot (lognot 42)) ; => 42  (double negation)
```

### Errors

Returns error if the argument is not an integer.

### See Also

- `logand` - Bitwise AND

## `ash`

Arithmetic shift - shift bits left or right.

### Parameters

- `integer` - The integer to shift
- `count` - Number of bits to shift (positive = left, negative = right)

### Returns

The integer shifted by `count` bits. Left shift multiplies by 2^count. Right shift divides by 2^count with sign extension (arithmetic shift).

### Examples

```lisp
(ash 1 3)            ; => 8   (1 << 3 = 8)
(ash 2 3)            ; => 16  (2 << 3 = 16)
(ash 8 -3)           ; => 1   (8 >> 3 = 1)
(ash 7 -1)           ; => 3   (7 >> 1 = 3, truncates)
(ash 42 0)           ; => 42  (no shift)
(ash -4 -1)          ; => -2  (sign-extending right shift)
```

### Errors

Returns error if either argument is not an integer.

### See Also

- `logand` - Bitwise AND

## `logcount`

Count the number of 1-bits in an integer (Hamming weight).

### Parameters

- `integer` - An integer

### Returns

For non-negative integers: the number of 1-bits. For negative integers: the number of 0-bits in two's complement representation.

### Examples

```lisp
(logcount 0)         ; => 0
(logcount 1)         ; => 1
(logcount 7)         ; => 3  (0b111)
(logcount 15)        ; => 4  (0b1111)
(logcount 42)        ; => 3  (0b101010)
```

### Errors

Returns error if the argument is not an integer.

### See Also

- `logand` - Bitwise AND

## `abs`

Absolute value of a number.

### Parameters

- `number` - An integer or float

### Returns

The absolute value. Integer input returns an integer; float input returns a float.

### Examples

```lisp
(abs -3)     ; => 3
(abs 3)      ; => 3
(abs -2.5)   ; => 2.5 (float)
(abs 0)      ; => 0
```

### Errors

Returns an error if the argument is not a number.

### See Also

- `float` - Convert a number to a float

## `sqrt`

Square root.

### Parameters

- `number` - A non-negative number

### Returns

The square root as a float, even for perfect squares.

### Examples

```lisp
(sqrt 16)   ; => 4 (float)
(sqrt 2)    ; => 1.4142135623730951
(sqrt 0)    ; => 0
```

### Errors

Returns an error if the argument is negative or not a number.

### See Also

- `expt` - Raise a number to a power

## `sin`

Sine of an angle in radians.

### Parameters

- `number` - An angle in radians

### Returns

The sine as a float.

### Examples

```lisp
(sin 0)                   ; => 0
(sin 1.5707963267948966)  ; => 1
```

### Errors

Returns an error if the argument is not a number or the result is not finite.

### See Also

- `cos` - Cosine
- `tan` - Tangent

## `cos`

Cosine of an angle in radians.

### Parameters

- `number` - An angle in radians

### Returns

The cosine as a float.

### Examples

```lisp
(cos 0)                   ; => 1
(cos 3.141592653589793)   ; => -1
```

### Errors

Returns an error if the argument is not a number or the result is not finite.

### See Also

- `sin` - Sine
- `tan` - Tangent

## `tan`

Tangent of an angle in radians.

### Parameters

- `number` - An angle in radians

### Returns

The tangent as a float.

### Examples

```lisp
(tan 0)                   ; => 0
(tan 0.7853981633974483)  ; => 1
```

### Errors

Returns an error if the argument is not a number or the result is not finite.

### See Also

- `sin` - Sine
- `cos` - Cosine

## `asin`

Arcsine, in radians.

### Parameters

- `number` - A number between -1 and 1 inclusive

### Returns

The angle in radians as a float, in the range [-pi/2, pi/2].

### Examples

```lisp
(asin 1)   ; => 1.5707963267948966
(asin 0)   ; => 0
```

### Errors

Returns an error if the argument is outside [-1, 1] or not a number.

### See Also

- `acos` - Arccosine
- `sin` - Sine

## `acos`

Arccosine, in radians.

### Parameters

- `number` - A number between -1 and 1 inclusive

### Returns

The angle in radians as a float, in the range [0, pi].

### Examples

```lisp
(acos 1)    ; => 0
(acos -1)   ; => 3.141592653589793
```

### Errors

Returns an error if the argument is outside [-1, 1] or not a number.

### See Also

- `asin` - Arcsine
- `cos` - Cosine

## `atan`

Arctangent, in radians.

### Parameters

- `number` - With one argument, the tangent value
- `y x` - With two arguments, the arctangent of `y / x` using the signs of both arguments (the `atan2` form)

### Returns

The angle in radians as a float. The one-argument form is in the range
[-pi/2, pi/2]; the two-argument form is in the range [-pi, pi].

### Examples

```lisp
(atan 1)      ; => 0.7853981633974483
(atan 1 1)    ; => 0.7853981633974483
(atan 1 -1)   ; => 2.356194490192345
```

### Errors

Returns an error if the number of arguments is not one or two, or if an
argument is not a number.

### See Also

- `atan2` - Two-argument arctangent
- `tan` - Tangent

## `atan2`

Two-argument arctangent.

### Parameters

- `y` - The ordinate
- `x` - The abscissa

### Returns

The angle in radians of the point (`x`, `y`) as a float, in the range
[-pi, pi]. Same as `(atan y x)`.

### Examples

```lisp
(atan2 1 1)     ; => 0.7853981633974483
(atan2 -1 -1)   ; => -2.356194490192345
(atan2 0 -1)    ; => 3.141592653589793
```

### Errors

Returns an error if either argument is not a number.

### See Also

- `atan` - Arctangent

## `exp`

Exponential function, e raised to a power.

### Parameters

- `number` - The exponent

### Returns

e**number as a float.

### Examples

```lisp
(exp 0)   ; => 1
(exp 1)   ; => 2.718281828459045
```

### Errors

Returns an error if the argument is not a number or the result is not finite.

### See Also

- `log` - Natural logarithm
- `expt` - Raise a number to a power

## `log`

Natural logarithm.

### Parameters

- `number` - A positive number

### Returns

The natural logarithm (base e) as a float.

### Examples

```lisp
(log 1)                   ; => 0
(log 2.718281828459045)   ; => 1
```

### Errors

Returns an error if the argument is not positive or not a number.

### See Also

- `exp` - Exponential function
- `expt` - Raise a number to a power

## `expt`

Raise a number to a power.

### Parameters

- `base` - The base (integer or float)
- `exponent` - The exponent (integer or float)

### Returns

`base` raised to `exponent`. When both arguments are integers and the exponent
is non-negative, the result is computed exactly and is an integer unless it
overflows, in which case a float is returned. All other cases use floating
point and return a float.

### Examples

```lisp
(expt 2 10)    ; => 1024 (integer)
(expt 2 0)     ; => 1
(expt 0 0)     ; => 1
(expt -2 3)    ; => -8
(expt 2 -1)    ; => 0.5 (float)
(expt 2.0 3)   ; => 8 (float)
(expt 4 0.5)   ; => 2 (float)
(expt 3 39)    ; => 4052555153018976267 (exact integer)
```

### Errors

Returns an error on division by zero (`(expt 0 -1)`), if an argument is not a
number, or if the result is not finite.

### See Also

- `exp` - e raised to a power
- `sqrt` - Square root

## `float`

Convert a number to a float.

### Parameters

- `number` - An integer or float

### Returns

The value as a float.

### Examples

```lisp
(float 3)     ; => 3 (float)
(float 2.5)   ; => 2.5
```

### Errors

Returns an error if the argument is not a number.

### See Also

- `abs` - Absolute value

## `truncate`

Round a number toward zero.

### Parameters

- `number` - An integer or float

### Returns

The number rounded toward zero, as an integer. A value too large for the
integer range is returned as a float.

### Examples

```lisp
(truncate 1.9)    ; => 1
(truncate -1.9)   ; => -1
(truncate 3)      ; => 3
```

### Errors

Returns an error if the argument is not a number.

### See Also

- `floor` - Round down
- `ceiling` - Round up
- `round` - Round to nearest

## `ceiling`

Round a number up toward positive infinity.

### Parameters

- `number` - An integer or float

### Returns

The number rounded up, as an integer. A value too large for the integer range
is returned as a float.

### Examples

```lisp
(ceiling 1.2)    ; => 2
(ceiling -1.2)   ; => -1
(ceiling 3)      ; => 3
```

### Errors

Returns an error if the argument is not a number.

### See Also

- `floor` - Round down
- `truncate` - Round toward zero
- `round` - Round to nearest

## `round`

Round a number to the nearest integer, with ties going to even.

### Parameters

- `number` - An integer or float

### Returns

The nearest integer. Exact halves round to the nearest even integer. A value
too large for the integer range is returned as a float.

### Examples

```lisp
(round 2.5)    ; => 2
(round 3.5)    ; => 4
(round -2.5)   ; => -2
(round 1.4)    ; => 1
(round 1.6)    ; => 2
(round 5)      ; => 5
```

### Errors

Returns an error if the argument is not a number.

### See Also

- `floor` - Round down
- `ceiling` - Round up
- `truncate` - Round toward zero

## `random`

Uniform pseudo-random number.

### Parameters

- `number` - A positive integer bound, or a positive float bound

### Returns

With a positive integer `n`, a uniformly distributed integer in `[0, n)`.
With a positive float `x`, a uniformly distributed float in `[0, x)`.

### Examples

```lisp
(random 10)   ; => an integer from 0 to 9
(random 1.0)  ; => a float from 0 up to (but not including) 1
(random 1)    ; => 0
```

### Errors

Returns an error if the argument is not a number or not positive.

### Notes

The generator is seeded from the clock on first use. Use `random-seed` to
reproduce a sequence.

### See Also

- `random-seed` - Reseed the generator

## `random-seed`

Reseed the pseudo-random number generator.

### Parameters

- `seed` - An integer seed

### Returns

`nil`.

### Examples

```lisp
(random-seed 42)
(define a (random 1000))
(random-seed 42)
(define b (random 1000))
(= a b)   ; => #t (the same seed reproduces the sequence)
```

### Errors

Returns an error if the seed is not an integer.

### See Also

- `random` - Uniform pseudo-random number
