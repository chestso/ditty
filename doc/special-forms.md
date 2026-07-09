# Special Forms

Core language forms that are evaluated specially rather than as function calls. Special forms control if and when their arguments are evaluated, unlike normal functions which evaluate all arguments first.

## `quote`

Prevent evaluation. Returns the literal datum without evaluating it.

Shorthand syntax: `'expr` is equivalent to `(quote expr)`.

### Parameters

- `expr` - Any expression (returned unevaluated)

### Examples

```lisp
(quote (1 2 3))    ; => (1 2 3)
'(1 2 3)           ; => (1 2 3) (shorthand)
'x                 ; => x (symbol, not evaluated)
```

## `quasiquote`

Quote with selective evaluation. Allows embedding evaluated values using `unquote` (`,`) and `unquote-splicing` (`,@`). Essential for writing macros.

Shorthand syntax: `` `expr`` is equivalent to `(quasiquote expr)`.

### Parameters

- `expr` - Template expression with unquote markers

### Examples

```lisp
(define x 42)
`(1 ,x 3)              ; => (1 42 3)
`(result is ,(+ 10 20)) ; => (result is 30)

(define lst '(a b c))
`(1 ,@lst 4)           ; => (1 a b c 4)
`(,@lst ,@lst)         ; => (a b c a b c)
```

## `if`

Conditional evaluation. Evaluates `condition`; if truthy, evaluates and returns the `then` branch, otherwise evaluates and returns the `else` branch.

Only `nil` is falsy; all other values (including `0`, `""`, and `()`) are truthy.

### Parameters

- `condition` - Test expression
- `then` - Expression to evaluate if condition is truthy
- `else` - Expression to evaluate if condition is falsy (optional, defaults to nil)

### Examples

```lisp
(if (> 5 3) "yes" "no")  ; => "yes"
(if (< 5 3) "yes" "no")  ; => "no"
(if nil "truthy" "falsy") ; => "falsy"
(if 0 "truthy" "falsy")   ; => "truthy" (only nil is falsy)
(if "" "truthy" "falsy")  ; => "truthy" (empty strings are truthy)
```

## `define`

Define or update a variable in the current environment. The symbol name is not evaluated.

When the value is a lambda, the lambda is automatically named after the symbol.

### Parameters

- `name` - Symbol name (not evaluated)
- `value` - Expression to evaluate and bind to the name

### Examples

```lisp
(define x 42)             ; => x
x                         ; => 42
(define y (+ x 8))        ; => y
y                         ; => 50
(define double (lambda (x) (* x 2)))
(function-name double)    ; => "double"
```

## `set!`

Mutate an existing variable. The variable must already be defined in the current environment or an enclosing scope.

### Parameters

- `name` - Symbol name (not evaluated)
- `value` - Expression to evaluate and assign to the name

### Examples

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

## `lambda`

Create an anonymous function (closure). Supports required, optional (`&optional`), and rest (`&rest`) parameters. The body has implicit `progn`: all expressions are evaluated, returning the last value with tail recursion optimization.

An optional docstring may be provided as the first body expression.

### Parameters

- `params` - Parameter list with required, `&optional`, and `&rest` markers
- `body...` - Expressions evaluated sequentially; last value returned

### Examples

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

## `defmacro`

Define a macro that transforms code at evaluation time. Macros receive arguments unevaluated; the macro body produces an expansion which is then evaluated in the caller's context.

Supports rest parameters using dotted parameter list syntax.

### Parameters

- `name` - Symbol name for the macro (not evaluated)
- `params` - Parameter list (arguments passed unevaluated)
- `body...` - Expressions producing the macro expansion

### Examples

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

## `let`

Create local variable bindings with parallel evaluation. All init expressions are evaluated before any variable is bound. Body has implicit `progn`.

### Parameters

- `bindings` - List of `(name init-expr)` pairs
- `body...` - Expressions evaluated in the new scope; last value returned

### Examples

```lisp
(let ((x 10) (y 20))
  (+ x y))                 ; => 30

(let ((a 1) (b 2))
  (* a b))                 ; => 2
```

## `let*`

Create local variable bindings with sequential evaluation. Each binding can reference previous bindings in the same `let*` form. Body has implicit `progn`.

### Parameters

- `bindings` - List of `(name init-expr)` pairs, evaluated in order
- `body...` - Expressions evaluated in the new scope; last value returned

### Examples

```lisp
(let* ((x 5) (y (+ x 3))) y)      ; => 8

(let* ((a 10)
       (b (* a 2))
       (c (+ b 5)))
  c)                              ; => 25

(let ((x 5) (y (+ x 3))) y)      ; ERROR: x undefined in parallel let
```

## `progn`

Evaluate multiple expressions sequentially and return the last value.

### Parameters

- `body...` - Zero or more expressions to evaluate in order

### Examples

```lisp
(progn 1 2 3)                    ; => 3
(progn
  (define x 10)
  (define y 20)
  (+ x y))                       ; => 30
(progn)                          ; => nil
```

## `do`

Iteration loop with variable bindings, step expressions, and an exit condition.

### Syntax

```lisp
(do ((var init step) ...)
    (test result-expr ...)
  body ...)
```

### Parameters

- `bindings` - List of `(var init step)` triples; `step` is optional (variable stays constant if omitted)
- `test-clause` - `(test result-expr ...)` evaluated at loop start; when truthy, result expressions are evaluated as implicit `progn`
- `body...` - Evaluated each iteration for side effects when test is falsy

### Examples

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

## `cond`

Multi-way conditional. Evaluates test expressions in order; the first truthy test's body expressions are evaluated as implicit `progn` and returned. Use `else` as the final test for a default case.

### Parameters

- `clauses...` - Each clause is `(test body...)`; `else` as test always matches

### Examples

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

## `case`

Value-based dispatch. Evaluates the key expression once, then matches it against literal value lists using `eq?` semantics.

### Parameters

- `key` - Expression evaluated once to produce the dispatch value
- `clauses...` - Each clause is `((values...) body...)` or `(else body...)`

### Examples

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

## `and`

Logical AND with short-circuit evaluation. Returns the last truthy value, or the first falsy value. Stops evaluating as soon as a falsy value is encountered.

### Parameters

- `expressions...` - Zero or more expressions evaluated left to right

### Examples

```lisp
(and)                ; => #t (empty and is true)
(and 1 2 3)          ; => 3 (returns last truthy value)
(and 1 nil 3)        ; => nil (short-circuits at nil)
(and #f (/ 1 0))     ; => #f (does not evaluate division)
```

## `or`

Logical OR with short-circuit evaluation. Returns the first truthy value, or the last falsy value if all are falsy. Stops evaluating as soon as a truthy value is encountered.

### Parameters

- `expressions...` - Zero or more expressions evaluated left to right

### Examples

```lisp
(or)                 ; => nil (empty or is false)
(or nil nil 5)       ; => 5 (first truthy value)
(or 1 2 3)           ; => 1 (short-circuits at first truthy)
(or #f "foo" (/ 1 0)) ; => "foo" (does not evaluate division)
```

## `condition-case`

Catch and handle errors by type (Emacs Lisp-style exception handling). Evaluates the body form; if an error is signaled, the handlers are checked in order and the first matching handler is executed.

### Parameters

- `var` - Symbol bound to the error object during handler execution (not evaluated)
- `body` - Expression to evaluate (may signal an error)
- `handlers...` - Each handler is `(error-type body...)`; `error` catches all types

### Examples

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

## `unwind-protect`

Guarantee cleanup code execution regardless of whether the body form succeeds or signals an error. Like `try-finally` in other languages.

### Parameters

- `body` - Expression to evaluate (may signal an error)
- `cleanup...` - Expressions evaluated after body completes, regardless of success or error

### Examples

```lisp
(unwind-protect
    (read-line file)
  (close file))  ; Always closes, even on error

(unwind-protect
    (signal 'test-error "boom")
  (define cleaned-up #t))
cleaned-up  ; => #t (cleanup ran despite error)
```
