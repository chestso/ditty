# Ditty Lisp Language Guide

A conceptual guide to Ditty Lisp, covering the evaluation model, special forms, macros, naming conventions, error handling, the package/library system, and tail recursion optimization.

For per-function documentation with parameters, return values, and examples, see [BUILTIN_REFERENCE.md](BUILTIN_REFERENCE.md).

## Table of Contents

- [Understanding Lisp Evaluation](#understanding-lisp-evaluation)
- [Data Types](#data-types)
- [Special Forms](#special-forms)
- [Built-in Functions](#built-in-functions)
- [Naming Conventions](#naming-conventions)
- [Truthy/Falsy Values](#truthyfalsy-values)
- [Pattern Matching](#pattern-matching)
- [Error Handling](#error-handling)
- [Package System](#package-system)
- [Library System](#library-system)
- [Tail Recursion Optimization](#tail-recursion-optimization)
- [Quick Examples](#quick-examples)

## Understanding Lisp Evaluation

Before diving into the language details, it helps to understand how Lisp evaluates code. When you write `(foo x y)`, Lisp evaluates this as a function call—but _how_ that call works depends on what `foo` is.

### What Is a "Form"?

Lisp documentation uses the word **form** a lot. In Lisp, a form is simply any expression that can be evaluated.

The term originates not in Lisp itself but in **Alonzo Church's lambda calculus** (1930s). Church distinguished between **functions** (which can be applied to arguments) and **forms** — expressions with free variables, like `y² + x`, that are not yet functions until those variables are bound. McCarthy borrowed this distinction directly in his 1960 paper ("Recursive Functions of Symbolic Expressions and Their Computation by Machine"), crediting Church in Section 2e: "This distinction and a notation for describing it, from which we deviate trivially, is given by Church."

When McCarthy designed Lisp, every M-expression "form" (a function application like `cons[car[x]; cdr[x]]`) was translated into an **S-expression** (a parenthesized list like `(CONS (CAR X) (CDR X))`). Since all Lisp code is represented as S-expressions, and `eval` recurses through them, "form" naturally came to mean "an S-expression that can be evaluated." The Common Lisp HyperSpec formalizes this: a form is "any object meant to be evaluated."

You'll see "form" used throughout this guide and in error messages:

- `(+ 1 2)` is a form (a list form, specifically a function call)
- `42` is a form (a self-evaluating literal)
- `x` is a form (a symbol form, evaluated by looking up the variable)
- `'foo` is a form (a quoted form, evaluating to the symbol `foo`)

When we say **special form**, we mean a form whose evaluation rules are special — the interpreter handles it differently from a normal function call. **Macro forms** are forms whose head symbol is bound to a macro. The word "form" is just Lisp terminology for "an expression ready for evaluation."

There are four main categories of forms:

### M-Expressions and S-Expressions

The parenthesized syntax you write in Ditty Lisp — and in every Lisp since the early 1960s — was not McCarthy's original design. It was an accident of history that became one of the most powerful ideas in programming.

**M-expressions** (meta-expressions) were McCarthy's intended notation for writing Lisp programs. They looked like algebra: function applications used brackets and semicolons, and lower-case function names:

```text
cons[car[x]; cdr[x]]
conditional[p₁ → e₁; p₂ → e₂]
```

M-expressions were meant to be the **programmer-facing language** — concise and mathematical. They would be compiled into a machine-friendly internal representation called **S-expressions** (symbolic expressions), which were nested parenthesized lists using upper-case atoms:

```text
(CONS (CAR X) (CDR X))
(COND (P1 E1) (P2 E2))
```

S-expressions were the **data format**: a simple recursive structure of atoms and lists, easy for a machine to parse and manipulate. McCarthy expected programmers to write M-expressions and let the system translate them into S-expressions behind the scenes.

**The accident:** McCarthy needed a way to evaluate S-expressions directly, so he implemented `eval` — a universal function that could evaluate any S-expression. Once `eval` existed, it became clear that you could skip the M-expression layer entirely and write programs directly as S-expressions. This collapsed the two-level design into one: **code and data became the same thing**. The parenthesized syntax that every Lisp programmer writes today is the S-expression notation that McCarthy originally intended only for machines.

This unification is the source of Lisp's most distinctive features:

- **Homoiconicity:** Programs are data structures (nested lists), not text. A Lisp program can be read, manipulated, and rewritten as Lisp data — this is what makes macros possible.
- **Quote:** `'(+ 1 2)` gives you the list `( + 1 2)` as data instead of evaluating it. This only makes sense because code and data share the same representation.
- **`eval`:** You can evaluate arbitrary code at runtime with `(eval expr)`. The program can treat its own code as data and vice versa.
- **Macros:** A macro receives unevaluated code (S-expressions) as arguments and returns new code (S-expressions). This is just list manipulation — trivial because code is lists.

S-expressions are either **atoms** (symbols, numbers, strings, etc.) or **lists** of S-expressions. The syntax is minimal: `(` and `)` delimit lists, whitespace separates elements, and `'` quotes the next expression. Everything else — function calls, variable references, conditionals, lambda definitions — is just different arrangements of atoms and lists.

As McCarthy later reflected, the survival of S-expressions and the disappearance of M-expressions was not planned: "The project of defining M-expressions precisely and compiling them or at least translating them into S-expressions was neither finalized nor explicitly abandoned. It just receded into the indefinite future."

### Special Forms

Special forms are **hard-coded in the interpreter** (in C). They have magical evaluation rules that can't be replicated by user code.

**Why special?** Normal function calls evaluate all arguments first, then call the function. Special forms break this rule—they control _if_ and _when_ their arguments get evaluated.

**Examples:**

- `if` - Only evaluates one branch (the true or false part, not both)
- `define` - The first argument is a symbol name, not evaluated
- `quote` - Returns its argument completely unevaluated
- `lambda` - The body isn't evaluated until the function is called
- `and`/`or` - Short-circuit: stop evaluating after finding the answer

```lisp
(if #t (print "yes") (launch-missiles))  ; Only prints "yes", missiles safe!
(define x 10)                            ; x is not evaluated, it's the name
'(+ 1 2)                                 ; => (+ 1 2), not 3
```

**You can't write `if` as a function** because both branches would be evaluated before the function even runs.

### Built-in Functions

Built-in functions are **implemented in C** for performance, but follow normal evaluation rules—all arguments are evaluated before the function runs.

**Why C?** Speed and access to system capabilities (file I/O, regex, etc.).

**Examples:**

- Arithmetic: `+`, `-`, `*`, `/`, `remainder`
- Lists: `car`, `cdr`, `cons`, `list`, `append`
- Strings: `string-append`, `string-length`, `substring`
- I/O: `print`, `read-file`, `write-file`

```lisp
(+ 1 2 3)              ; All args evaluated, then summed
(string-append "a" "b") ; Both strings evaluated, then concatenated
```

### Macros

Macros are **code transformers written in Lisp**. They receive their arguments **unevaluated**, transform them into new code, and that new code gets evaluated.

**Why macros?** To extend the language syntax without modifying the interpreter. They're like special forms you can write yourself.

**Examples:**

- `when`/`unless` - Conditional execution (defined as macros expanding to `if`)
- `defun` - Named function definition (expands to `define` + `lambda`)
- `defvar` - Variable definition with optional docstring

```lisp
; This macro...
(when (> x 0)
  (print "positive")
  (+ x 1))

; ...expands to this code, which then runs:
(if (> x 0)
    (progn
      (print "positive")
      (+ x 1))
    nil)
```

**Macros vs special forms:** Both control evaluation, but macros are written in Lisp and work by code transformation. Special forms are primitive—they're the building blocks macros use.

### User-Defined Functions (Lambdas)

Regular functions written in Lisp using `lambda` or `defun`. Arguments are evaluated before the function runs (normal evaluation).

```lisp
(define square (lambda (x) (* x x)))
(defun cube (x) (* x x x))

(square (+ 2 3))  ; (+ 2 3) evaluated to 5, then squared => 25
```

### Comparison Table

| Type              | Implemented In  | Args Evaluated?    | Can Control Evaluation?  | Extensible? |
| ----------------- | --------------- | ------------------ | ------------------------ | ----------- |
| Special Form      | C (interpreter) | Depends on form    | Yes                      | No          |
| Built-in Function | C               | Yes, all           | No                       | No          |
| Macro             | Lisp            | No (receives code) | Yes (via transformation) | Yes         |
| User Function     | Lisp            | Yes, all           | No                       | Yes         |

### When to Use What

- **Need to control evaluation?** → Macro (or use existing special forms)
- **Performance-critical operation?** → Built-in (already in C)
- **Normal computation?** → User function
- **Extending syntax?** → Macro

Most code you write will be user functions. Macros are powerful but should be used sparingly—prefer functions when possible, since they're simpler to understand and debug.

## Data Types

- **Numbers**: Double precision floating-point numbers
- **Integers**: 64-bit signed integers
- **Booleans**: True (#t) and false (#f)
- **Strings**: Unicode strings with full UTF-8 support
- **Characters**: Unicode characters (codepoints) with `#\a` reader syntax
- **Lists**: Cons cells for linked lists
- **Vectors**: Dynamic arrays with `#(elem ...)` literal syntax (grow/shrink)
- **Hash Tables**: Key-value mappings
- **Symbols**: Interned names with optional docstrings (used for variables, functions)
- **Keywords**: Self-evaluating interned symbols starting with `:` (e.g., `:foo`, `:key`)
- **Lambda Functions**: User-defined closures with lexical scoping

## Special Forms

- `quote` - Quote expressions (also supported via `'expr` syntax)
- `quasiquote` - Quote with selective evaluation (also supported via `` `expr`` syntax)
- `unquote` - Evaluate within quasiquote (also supported via `,expr` syntax)
- `unquote-splicing` - Evaluate and splice list within quasiquote (also supported via `,@expr` syntax)
- `if` - Conditional evaluation
- `define` - Define variables and functions
- `set!` - Mutate existing variables (works inside lambdas/hooks)
- `lambda` - Create anonymous functions with required, optional (&optional), and rest (&rest) parameters (body has implicit progn: evaluates all expressions, returns last with tail recursion optimization)
- `defmacro` - Define macros for code transformation at evaluation time
- `let` - Local variable bindings (parallel evaluation, body has implicit progn)
- `let*` - Local variable bindings (sequential evaluation, can reference previous bindings, body has implicit progn)
- `progn` - Evaluate multiple expressions sequentially and return last value
- `do` - Iteration loop with variable bindings, step expressions, and exit condition (result expressions have implicit progn)
- `cond` - Multi-way conditional with test clauses
- `case` - Pattern matching with value-based dispatch
- `and` - Logical AND with short-circuit evaluation (returns last truthy value or first falsy value)
- `or` - Logical OR with short-circuit evaluation (returns first truthy value or last falsy value)
- `condition-case` - Catch and handle errors by type (Emacs Lisp-style exception handling)
- `unwind-protect` - Guarantee cleanup code execution (like try-finally)
- `package-ref` - Resolve a symbol in a specific package (emitted by `pkg:sym` reader syntax)

### Macros

Macros are special functions that receive unevaluated arguments and return code to be evaluated. They enable powerful code transformations at evaluation time.

#### Quasiquote (Backquote)

Quasiquote (`` ` ``) is like quote (`'`) but allows selective evaluation using unquote (`,`) and unquote-splicing (`,@`). This is essential for writing macros that construct code with computed values.

**Syntax:**

- `` `expr`` - Quasiquote (same as `(quasiquote expr)`)
- `,expr` - Unquote: evaluate expr and insert result (same as `(unquote expr)`)
- `,@expr` - Unquote-splicing: evaluate expr (must be a list) and splice elements (same as `(unquote-splicing expr)`)

**Examples:**

```lisp
; Simple quasiquote (same as quote)
`(1 2 3)  ; => (1 2 3)

; Unquote to insert computed values
(define x 42)
`(1 ,x 3)  ; => (1 42 3)
`(result is ,(+ 10 20))  ; => (result is 30)

; Nested lists
`(a (b ,x c) d)  ; => (a (b 42 c) d)

; Unquote-splicing to splice lists
(define lst '(a b c))
`(1 ,@lst 4)  ; => (1 a b c 4)
`(start ,x middle ,@lst end)  ; => (start 42 middle a b c end)

; Multiple splicing
`(,@lst ,@lst)  ; => (a b c a b c)

; Empty list splicing
`(1 ,@'() 2)  ; => (1 2)
```

**Common pattern in macros:**

```lisp
; Without quasiquote (cumbersome)
(defmacro when (condition body)
  (list 'if condition body nil))

; With quasiquote (clean)
(defmacro when (condition body)
  `(if ,condition ,body nil))
```

#### defmacro

Define a macro that transforms code before evaluation.

**Syntax:**

```lisp
(defmacro name (params...) body...)
(defmacro name (param1 param2 . rest-params) body...)
```

**Features:**

- Macros receive arguments unevaluated
- The macro body is evaluated to produce an expansion
- The expansion is then evaluated in the caller's context
- Supports rest parameters using dotted parameter list syntax

**Examples:**

```lisp
; Simple macro that returns a quoted expression
(defmacro simple () '(+ 1 2))
(simple)  ; => 3

; Macro with parameters
(defmacro double (x) (list '+ x x))
(double 5)          ; => 10
(double (+ 2 3))    ; => 10

; Conditional macro (like 'when')
(defmacro when (condition body)
  (list 'if condition body nil))
(when #t 42)   ; => 42
(when #f 42)   ; => nil

; Macro with rest parameters
(defmacro my-progn (first . rest)
  (cons 'progn (cons first rest)))
```

#### when and unless

`when` and `unless` are conditional macros that execute their body only when a condition is true (or false).

**Syntax:**

```lisp
(when condition body...)
(unless condition body...)
```

**Examples:**

```lisp
; when - execute body when condition is true
(when (> 5 3)
  (+ 1 2)
  (* 3 4))  ; => 12 (returns last expression)

(when nil
  (+ 1 2))  ; => nil

; unless - execute body when condition is false
(unless (< 5 3)
  "condition was false")  ; => "condition was false"

(unless #t
  "won't run")  ; => nil
```

**Implementation:**

```lisp
(defmacro when (condition . body)
  "Execute BODY when CONDITION is true."
  `(if ,condition (progn ,@body) nil))

(defmacro unless (condition . body)
  "Execute BODY when CONDITION is false."
  `(if ,condition nil (progn ,@body)))
```

#### defun

`defun` is a built-in macro that provides a convenient syntax for defining named functions. It's implemented using quasiquote:

**Syntax:**

```lisp
(defun name (params...) body...)
```

**Definition:**

```lisp
(defmacro defun (name params . body)
  `(define ,name (lambda ,params ,@body)))
```

**Expands to:**

```lisp
(define name (lambda (params...) body...))
```

**Examples:**

```lisp
; Simple function
(defun add-one (x) (+ x 1))
(add-one 5)  ; => 6

; Multiple parameters
(defun add-three (a b c)
  (+ a b c))
(add-three 1 2 3)  ; => 6

; Multiple body expressions
(defun test-multi (x)
  (+ x 1)
  (+ x 2)
  (+ x 3))
(test-multi 10)  ; => 13

; Recursive function
(defun factorial (n)
  (if (= n 0)
      1
      (* n (factorial (- n 1)))))
(factorial 5)  ; => 120
```

#### lambda

Create anonymous functions with optional and rest parameters. Lambda functions support lexical scoping and capture their environment (closures).

**Syntax:**

```lisp
(lambda (params...) body...)
(lambda (params...) "docstring" body...)
(lambda (required... &optional optional... &rest rest-param) body...)
```

**Parameter Types:**

- **Required parameters**: Must be provided, bound in order
- **Optional parameters** (after `&optional`): Default to `nil` if not provided
- **Rest parameter** (after `&rest`): Collects remaining arguments as a list

**Features:**

- Body has implicit `progn`: evaluates all expressions, returns last value
- Tail recursion optimization for last expression
- Lexical scoping: captures environment variables
- Optional docstring as first body expression (only when followed by more expressions)
- Automatically named when assigned via `define` (e.g., `(define foo (lambda ...))` names the lambda "foo" for stack traces)

**Examples:**

```lisp
; Basic lambda
((lambda (x) (* x 2)) 5)  ; => 10

; Multiple parameters
((lambda (a b) (+ a b)) 3 4)  ; => 7

; Multiple body expressions (implicit progn)
((lambda (x)
   (+ x 1)
   (+ x 2)
   (* x 3)) 5)  ; => 15 (only last expression returned)

; Optional parameters (default to nil)
((lambda (a &optional b) (list a b)) 1)     ; => (1 nil)
((lambda (a &optional b) (list a b)) 1 2)   ; => (1 2)

; Multiple optional parameters
((lambda (a &optional b c d) (list a b c d)) 1 2)  ; => (1 2 nil nil)

; Only optional parameters (no required)
((lambda (&optional a b) (list a b)))      ; => (nil nil)
((lambda (&optional a b) (list a b)) 10)   ; => (10 nil)

; Default values via (or param default)
(define greet
  (lambda (name &optional greeting)
    (let ((g (or greeting "Hello")))
      (concat g ", " name "!"))))
(greet "Alice")      ; => "Hello, Alice!"
(greet "Bob" "Hi")   ; => "Hi, Bob!"

; Rest parameter (collects remaining args)
((lambda (a &rest more) (list a more)) 1 2 3)  ; => (1 (2 3))
((lambda (&rest all) all) 1 2 3 4)             ; => (1 2 3 4)

; Combined optional and rest
((lambda (a &optional b &rest more) (list a b more)) 1)        ; => (1 nil ())
((lambda (a &optional b &rest more) (list a b more)) 1 2 3 4)  ; => (1 2 (3 4))

; Variadic function with rest parameter
(define sum
  (lambda (initial &rest numbers)
    (let ((total initial))
      (do ((nums numbers (cdr nums)))
        ((null? nums) total)
        (set! total (+ total (car nums)))))))
(sum 1)           ; => 1
(sum 1 2 3 4 5)   ; => 15

; Closures (capture environment)
(define make-adder
  (lambda (x)
    (lambda (y) (+ x y))))
(define add5 (make-adder 5))
(add5 10)  ; => 15

; Lambda with docstring (must have body after docstring)
(define square
  (lambda (x)
    "Return X squared."
    (* x x)))
(documentation 'square)  ; => "Return X squared."

; Lambdas in let bindings can also have docstrings
(let ((double (lambda (x)
                "Double the value X."
                (* x 2))))
  (double 5))  ; => 10

; Named functions (via define)
(define factorial
  (lambda (n)
    (if (<= n 1) 1 (* n (factorial (- n 1))))))
(factorial 5)  ; => 120
```

**Parameter Order Rules:**

1. Required parameters come first
2. `&optional` marker introduces optional parameters (all default to `nil`)
3. `&rest` marker must be last, followed by exactly one parameter name
4. `&optional` cannot appear after `&rest`

**Arity Checking:**

- Too few arguments (missing required): Error
- Too many arguments (without `&rest`): Error
- With `&rest`: No maximum argument limit

### Docstrings

Documentation strings (docstrings) follow Emacs Lisp conventions. Functions and macros get docstrings from the first string in their body. Variables use `defvar`, `defconst`, or `set-documentation!`. Docstrings use CommonMark (Markdown) format.

**Syntax for Functions/Macros:**

```lisp
(lambda (params...)
  "Docstring here (optional)"
  body...)

(defmacro name (params...)
  "Docstring here (optional)"
  body...)
```

**Syntax for Variables:**

```lisp
(defvar name value "Docstring here (optional)")
(defconst name value "Docstring here (optional)")
(set-documentation! 'name "Docstring here")
```

**Requirements:**

- Docstring must be a string literal
- For functions/macros: must be first expression after parameters, followed by at least one more expression
- Single-expression functions cannot have docstrings (prevents ambiguity)
- Docstrings use CommonMark (Markdown) format

**Introspection Functions:**

- `(documentation symbol)` - Get docstring for a symbol. First checks the symbol's own docstring (set via `set-documentation!` or copied from lambda/macro on define), then falls back to the value's docstring if bound to a lambda/macro/builtin.
- `(doc symbol)` - Shorthand for `documentation`
- `(bound? symbol)` - Check if symbol is bound in the environment
- `(set-documentation! symbol docstring)` - Set docstring directly on the interned symbol. Since symbols are interned (globally shared), this sets the docstring globally.
- `(doc-set! symbol docstring)` - Shorthand for `set-documentation!`

Returns `nil` if no docstring exists.

**Note:** Docstrings are stored on symbols (like Emacs Lisp), not on bindings. This means docstrings are global per symbol name.

#### Variable Definition Macros

**defvar** - Define a variable (only sets value if unbound):

```lisp
(defvar name)                    ; Define with nil value
(defvar name value)              ; Define with value
(defvar name value "Docstring")  ; Define with value and docstring
```

`defvar` only sets the value if the variable is not already bound. The docstring is always set if provided.

**defconst** - Define a constant (always sets value):

```lisp
(defconst name value)              ; Define constant
(defconst name value "Docstring")  ; Define with docstring
```

`defconst` always sets the value (unlike `defvar`). Note: constants are a convention, not enforced (Emacs Lisp style - hackable).

**defalias** - Create a function alias:

```lisp
(defalias alias target)              ; Create alias
(defalias alias target "Docstring")  ; Create alias with docstring
```

**Examples:**

```lisp
; Variable with docstring
(defvar my-toggle nil "Non-nil means the feature is enabled.")
(documentation 'my-toggle)  ; => "Non-nil means the feature is enabled."

; defvar doesn't rebind if already bound
(defvar my-toggle #t "New docstring")
my-toggle  ; => nil (unchanged)

; Constant with docstring
(defconst pi 3.14159 "The ratio of circumference to diameter.")
(documentation 'pi)  ; => "The ratio of circumference to diameter."

; defconst always rebinds
(defconst pi 3.14 "Truncated pi")
pi  ; => 3.14 (updated)

; Function alias
(defalias my-add + "Alias for the + function.")
(my-add 1 2 3)  ; => 6
(doc 'my-add)   ; => "Alias for the + function."

; Check if symbol is bound
(bound? 'car)                    ; => #t
(bound? 'nonexistent-symbol)     ; => #f

; Set documentation on a symbol (works whether bound or not)
(define my-var 42)
(set-documentation! 'my-var "The answer to everything.")
(documentation 'my-var)  ; => "The answer to everything."

; Can also set docstring on unbound symbols
(set-documentation! 'future-var "Will be defined later.")
(documentation 'future-var)  ; => "Will be defined later."
```

#### Function Docstrings

````lisp
; Function with docstring
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
; => "Calculate the area of a rectangle.\n\n    ## Parameters..."

; Macro with docstring
(defmacro when (condition . body)
  "Execute BODY when CONDITION is true."
  `(if ,condition (progn ,@body) nil))

(documentation 'when)
; => "Execute BODY when CONDITION is true."

; Single-expression lambda (NO docstring - string is return value)
(define return-msg (lambda () "Hello"))
(documentation 'return-msg)  ; => nil
(return-msg)                 ; => "Hello"

; Multi-line CommonMark docstring
(define process-data
  (lambda (data)
    "Process data using various transformations.

    ## Description
    This function applies a series of transformations:
    1. Validate input
    2. Transform data
    3. Return result

    ## Example
    ```lisp
    (process-data '(1 2 3))  ; => '(2 4 6)
    ```

    **Note**: Input must be a list."
    (map (lambda (x) (* x 2)) data)))

; Closures preserve docstrings
(define make-multiplier
  (lambda (factor)
    "Create a function that multiplies by FACTOR."
    (lambda (x)
      "Multiply X by the captured factor."
      (* x factor))))
````

**Edge Cases:**

```lisp
; Lambda without docstring
(define double (lambda (x) (* x 2)))
(documentation 'double)  ; => nil

; String-only body is NOT a docstring
(define msg (lambda () "Just a message"))
(documentation 'msg)  ; => nil (only one expression)
(msg)  ; => "Just a message" (returns the string)
```

#### Standard Library Aliases

The standard library provides convenient aliases:

- `doc` - Alias for `documentation`
- `doc-set!` - Alias for `set-documentation!`
- `string-append` - Alias for `concat`

## Built-in Functions

For a complete listing of all built-in functions, macros, and variables with
detailed parameter descriptions, return values, and examples, see
[BUILTIN_REFERENCE.md](BUILTIN_REFERENCE.md).

The same documentation is available at runtime via `(documentation 'symbol)`:

```lisp
(documentation '+)       ; => "Add numbers together."
(documentation 'car)     ; => "Return the first element of a list."
(doc 'pair?)             ; => shorthand for (documentation ...)
```

## Naming Conventions

Ditty Lisp follows modern Lisp naming conventions:

- **Predicates** use `?` suffix: `null?`, `vector?`, `integer?`, `boolean?`, `string?`
- **Mutating functions** use `!` suffix: `set!`, `vector-set!`, `vector-push!`, `hash-set!`
- **Non-mutating functions** have no special suffix: `car`, `cdr`, `vector-ref`, `hash-ref`

## Truthy/Falsy Values

Following traditional Lisp semantics:

- **Falsy**: Only `nil` (and `#f`, which is identical to `nil`)
- **Truthy**: Everything else, including:
  - Numbers: `0`, `0.0`, `42`, `-1`
  - Strings: `""`, `"hello"`
  - Collections: Empty vectors `#()`, empty hash tables, non-empty collections
  - Booleans: `#t`

**Note**: Unlike some Lisps, both integer `0` and float `0.0` are **truthy**. Only `nil` is false.

## Pattern Matching

The `split` and `string-match?` functions support enhanced wildcard patterns:

- `*` - Matches zero or more characters
- `?` - Matches exactly one character
- `[abc]` - Matches any character in set
- `[a-z]` - Matches character ranges
- `[!abc]` - Matches anything NOT in set

Examples:

```lisp
(string-match? "hello" "h*o")         ; => 1
(string-match? "hello" "h?llo")       ; => 1
(string-match? "hello" "h??lo")       ; => 1
(string-match? "hello" "h[aeiou]llo") ; => 1
(string-match? "hello" "h[a-z]llo")   ; => 1
```

## Error Handling

Ditty Lisp implements an Emacs Lisp-style condition system with typed errors, catch/handle, and guaranteed cleanup. The system provides:

- **Typed errors**: Errors have symbol-based types (e.g., `'division-by-zero`, `'file-error`)
- **Error catching**: `condition-case` catches and handles specific error types
- **Guaranteed cleanup**: `unwind-protect` ensures cleanup code always runs
- **Error introspection**: Full access to error type, message, data, and call stack
- **Automatic propagation**: Uncaught errors propagate with call stack traces

### Signal and Error Creation

**`signal`** - Raise a typed error:

```lisp
(signal 'division-by-zero "Cannot divide by zero")
; => ERROR: [division-by-zero] Cannot divide by zero

(signal 'file-error "Cannot open file")
; => ERROR: [file-error] Cannot open file

; With data payload
(signal 'file-error '("cannot open" "file.txt" 404))
; => ERROR: [file-error] ("cannot open" "file.txt" 404)
```

**`error`** - Convenience function for generic errors:

```lisp
(error "Something went wrong")
; => ERROR: [error] Something went wrong
```

### Condition-Case (Error Catching)

**Syntax:** `(condition-case VAR BODYFORM HANDLERS...)`

Catch and handle specific error types:

```lisp
; Basic error catching
(condition-case e
    (/ 10 0)
  (division-by-zero "Caught division by zero"))
; => "Caught division by zero"

; Multiple handlers
(condition-case e
    (some-risky-operation)
  (file-error "File problem")
  (network-error "Network problem")
  (error "Other error"))  ; 'error catches everything

; Access the error object
(condition-case err
    (signal 'my-error "test message")
  (error (error-message err)))
; => "test message"

; Handler with multiple expressions (implicit progn)
(condition-case e
    (signal 'test-error "boom")
  (test-error
    (define handled #t)
    (define result "recovered")
    result))
; => "recovered"
```

**Handler matching:**

- Handlers are checked in order
- First matching handler executes
- `'error` symbol catches all error types
- More specific handlers take precedence

### Unwind-Protect (Guaranteed Cleanup)

**Syntax:** `(unwind-protect BODYFORM CLEANUP...)`

Guarantee cleanup code execution (like try-finally):

```lisp
; Cleanup always runs
(define file (open "data.txt" "r"))
(unwind-protect
    (read-line file)
  (close file))  ; Always closes, even on error

; Multiple cleanup forms
(define cleanup-count 0)
(unwind-protect
    (signal 'test-error "boom")
  (define cleanup-count (+ cleanup-count 1))
  (define cleanup-count (+ cleanup-count 10)))
cleanup-count  ; => 11 (cleanup ran despite error)

; Returns body result even if error
(condition-case e
    (unwind-protect
        (signal 'test-error "boom")
      (define cleaned-up #t))
  (error "caught"))
cleaned-up  ; => #t
```

### Error Introspection

Caught errors can be inspected:

```lisp
(define my-err nil)
(condition-case e
    (signal 'custom-error "test message")
  (error (define my-err e)))

(error? my-err)           ; => #t
(error-type my-err)       ; => custom-error
(error-message my-err)    ; => "test message"
(error-data my-err)       ; => "test message"
(error-stack my-err)      ; => ("signal")
```

### Practical Examples

**Safe division:**

```lisp
(defun safe-divide (a b)
  (condition-case err
      (if (= b 0)
          (signal 'division-by-zero "cannot divide by zero")
          (/ a b))
    (division-by-zero "Error: division by zero")
    (error (concat "Unexpected error: " (error-message err)))))

(safe-divide 10 2)  ; => 5
(safe-divide 10 0)  ; => "Error: division by zero"
```

**Resource management:**

```lisp
(defun process-file (filename)
  (let ((file (open filename "r")))
    (unwind-protect
        (let ((content (read-line file)))
          (if (null? content)
              (signal 'empty-file "File is empty")
              content))
      (close file))))  ; Always closes
```

**Nested error handling:**

```lisp
(condition-case outer
    (condition-case inner
        (signal 'inner-error "from inner")
      (other-error "inner handler"))
  (inner-error "outer caught it"))
; => "outer caught it"
```

### Error Propagation and Call Stacks

Uncaught errors automatically propagate with call stack traces:

```lisp
(define inner (lambda (x) (/ x 0)))
(define middle (lambda (x) (inner x)))
(define outer (lambda (x) (middle x)))
(outer 10)
; => ERROR: Division by zero
;    Call stack:
;      at "/"
;      at "inner"
;      at "middle"
;      at "outer"
```

### Common Error Types

Standard error types (user-extensible):

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

```lisp
(signal 'my-custom-error "Custom error message")
```

## Package System

Ditty Lisp has a package system for organizing bindings into namespaces. Each binding is tagged with an owning package. The default packages are `core` (builtins and stdlib) and `user` (user-defined bindings).

See [Package Functions](#package-functions) for the full function reference. All package functions have `package-` prefixed aliases (e.g., `package-set` for `in-package`).

### Current Package

The `*package*` variable controls where `define` creates new bindings. It defaults to `user`. Packages are referenced by symbol.

- `current-package` (alias: `package-current`) - Return the current package name as a symbol
- `in-package` (alias: `package-set`) - Set the current package (symbol; strings also accepted for convenience)

```lisp
(current-package)       ; => user
(in-package 'math)
(current-package)       ; => math
(in-package 'user)
```

### Qualified Symbol Access

Use `pkg:symbol` syntax to access a binding in a specific package:

```lisp
(in-package 'math)
(define pi 3.14159)
(define add (lambda (a b) (+ a b)))

(in-package 'user)
math:pi                 ; => 3.14159
(math:add 2 3)          ; => 5
(core:+ 10 20)          ; => 30 (access builtins explicitly)
```

The reader translates `pkg:sym` into the `(package-ref "pkg" sym)` special form.

### Package Introspection

- `package-symbols` - Return an alist of `(symbol . value)` pairs for a named package (symbol)
- `list-packages` (alias: `package-list`) - Return a list of all package names as symbols

```lisp
(in-package 'math)
(define x 42)
(in-package 'user)

(package-symbols 'math)   ; => ((x . 42) ...)
(list-packages)            ; => (core user math)
```

### Saving Package Bindings

- `package-save` - Save a package's bindings to a file as loadable Lisp source

```lisp
(package-save "math-lib.lisp" 'math)  ; save specific package
(package-save "session.lisp")          ; save current package
(load "math-lib.lisp")                 ; restore later
```

## Library System

Ditty Lisp has a library system built on top of the package system, using `require`/`provide` for load-once semantics (similar to Emacs Lisp). This is orthogonal to packages (namespaces) — a library file defines bindings in a package and calls `provide` to register itself as loaded.

### `require` / `provide`

`provide` adds a feature symbol to `*features*`. `require` checks if a feature is already loaded; if not, it searches load paths for the library file, loads it, and verifies `provide` was called.

```lisp
;; mylib.lisp — a library file
(in-package 'mylib)
(export 'greet 'farewell)
(defun greet (name) (concat "Hello, " name))
(provide 'mylib)

;; main.lisp — using the library
(require 'mylib)
(mylib:greet "World")          ; qualified access
(use-package 'mylib)           ; or import for unqualified access
(greet "World")
```

Both `require` and `provide` accept a string or a symbol. The name may contain slashes for relative paths (e.g., `(require "mylib/foo")` looks for `<dir>/mylib/foo.lisp`).

```lisp
(require "mylib/foo")   ; string with relative path
(require 'mylib)        ; symbol (bare name)
(provide "mylib/foo")   ; must match the name used with require
```

`require` saves and restores `*package*`, so a library's `in-package` does not leak to the caller. Transitive dependencies work naturally — if `mylib` calls `(require 'utils)` at its top, loading `mylib` automatically pulls in `utils`.

### Library Search Path

`require` searches these locations in order (first match wins):

1. Current directory (`name.lisp`, then `name/name.lisp`)
2. Directories in `DITTY_LISP_PATH` (colon-separated on Unix, semicolon on Windows)
3. `$XDG_DATA_HOME/ditty/lisp/` (default: `~/.local/share/ditty/lisp/`)
4. Each dir in `$XDG_DATA_DIRS/ditty/lisp/` (default: `/usr/local/share:/usr/share`)

On Windows: `%APPDATA%\ditty\lisp\` (user) and exe-relative `..\share\ditty\lisp\` (system).

`DITTY_LISP_PATH` is analogous to Emacs' `load-path` and Common Lisp's `CL_SOURCE_REGISTRY`.

### Export and use-package

`export` marks which symbols in a package are public. Non-exported symbols remain accessible only via `pkg:symbol` qualified syntax. If `export` is never called for a package, `use-package` imports all bindings (default = all exported, Emacs Lisp convention).

```lisp
(in-package 'mylib)
(export 'greet 'farewell)
(defun greet (name) ...)
(defun internal-helper () ...)  ; not exported

(in-package 'user)
(use-package 'mylib)            ; imports greet, farewell (not internal-helper)
(greet "World")                 ; accessible unqualified
mylib:internal-helper            ; still accessible via qualified access
```

### Introspection

- `*features*` — list of loaded feature symbols
- `*load-path*` — list of library search directory strings
- `provided?` — check if a feature is loaded
- `package-exports` — return a package's exported symbols

## Tail Recursion Optimization

Ditty Lisp implements **tail call optimization** (TCO) using a trampoline-based approach. This allows recursive functions to execute without growing the call stack, enabling efficient recursive algorithms that would otherwise cause stack overflow.

### What is Tail Position?

An expression is in **tail position** if it is the last operation performed before returning from a function. The result of a tail-positioned expression becomes the return value of the enclosing function without any further computation.

**Tail position locations:**

- Last expression in a lambda body
- Last expression in `progn`, `let`, `let*`
- Both branches of `if` (then and else)
- Return expressions in `cond` and `case` clauses
- Body of `do` loop (result expression)

**NOT in tail position:**

- Arguments to function calls: `(+ 1 (factorial 5))` - factorial call is NOT in tail position
- Non-final expressions in sequence
- Operands of arithmetic or comparison operators

### How It Works

When a function call appears in tail position, instead of executing it immediately and growing the stack, the interpreter:

1. Creates a tail call object with the function and its evaluated arguments
2. Returns this object to the trampoline loop
3. The trampoline unwraps and executes the call iteratively

This converts deep recursion into iteration at the interpreter level.

### Examples

**Tail-recursive factorial:**

```lisp
;; Tail-recursive with accumulator
(define factorial-tail
  (lambda (n acc)
    (if (<= n 1)
        acc
        (factorial-tail (- n 1) (* n acc)))))

(define factorial
  (lambda (n)
    (factorial-tail n 1)))

(factorial 10)      ; => 3628800
(factorial 1000)    ; Works without stack overflow!
```

**Tail-recursive fibonacci:**

```lisp
(define fib-iter
  (lambda (n a b)
    (if (= n 0)
        a
        (fib-iter (- n 1) b (+ a b)))))

(define fibonacci
  (lambda (n)
    (fib-iter n 0 1)))

(fibonacci 20)      ; => 6765
(fibonacci 100)     ; => 354224848179262000000
```

**Mutually recursive functions:**

```lisp
(define even?
  (lambda (n)
    (if (= n 0)
        #t
        (odd? (- n 1)))))

(define odd?
  (lambda (n)
    (if (= n 0)
        #f
        (even? (- n 1)))))

(even? 1000)        ; => #t (works without stack overflow)
```

**Helper function in tail position:**

```lisp
(define format-result
  (lambda (x)
    (string-append "Result: " (number->string x))))

(define compute-and-format
  (lambda (x y)
    (if (> x y)
        (format-result (* x y))    ; Tail call
        (format-result (+ x y))))) ; Tail call

(compute-and-format 10 5)  ; => "Result: 50"
```

### Non-Tail Recursion (Still Works)

Non-tail recursive functions still work but use stack space:

```lisp
;; NOT tail-recursive (multiplication happens after recursive call)
(define factorial-normal
  (lambda (n)
    (if (<= n 1)
        1
        (* n (factorial-normal (- n 1))))))  ; NOT in tail position

(factorial-normal 10)   ; => 3628800 (works but uses stack)
```

### Benefits

- **No stack overflow**: Recursive algorithms can run indefinitely
- **Constant memory**: Stack size remains constant regardless of recursion depth
- **Performance**: Recursion is as efficient as iteration
- **Functional style**: Write clean recursive code without worrying about stack limits

### When to Use

Use tail recursion when:

- Implementing recursive algorithms (factorial, fibonacci, tree traversal)
- Processing lists recursively
- State machines with recursive state transitions
- Any algorithm that would naturally use iteration

Convert to tail recursion by:

- Adding accumulator parameters
- Passing partial results down instead of building up on return
- Ensuring recursive call is the last operation

## Quick Examples

### Variables

```lisp
(define x 42)
(define y 10)
(+ x y)            ; => 52
```

### Functions

```lisp
; Define a function
(define square (lambda (n) (* n n)))
(square 5)         ; => 25

; Define a function with multiple operations
(define abs (lambda (n)
  (if (< n 0)
    (- n)
    n)))
(abs -5)           ; => 5
```

### Conditionals

```lisp
(if (> 5 3) "yes" "no")              ; => "yes"
(if (< 5 3) "yes" "no")              ; => "no"
(if nil "truthy" "falsy")            ; => "falsy"
(if 0 "truthy" "falsy")              ; => "truthy" (only nil is falsy!)
(if "" "truthy" "falsy")             ; => "truthy" (empty strings are truthy!)
```

### Boolean Operations

`and` and `or` are special forms with short-circuit evaluation:

```lisp
; and - returns last value if all truthy, or first falsy value
(and)                                ; => 1 (empty and is true)
(and 1 2 3)                          ; => 3 (returns last truthy value)
(and 1 nil 3)                        ; => nil (short-circuits at nil)
(and #f (/ 1 0))                     ; => #f (doesn't evaluate division)

; or - returns first truthy value, or last value if all falsy
(or)                                 ; => nil (empty or is false)
(or nil nil 5)                       ; => 5 (first truthy value)
(or 1 2 3)                           ; => 1 (short-circuits at first truthy)
(or #f "foo" (/ 1 0))                ; => "foo" (doesn't evaluate division)

; not - simple negation
(not nil)                            ; => 1
(not 5)                              ; => nil

; list? - check if value is a list (nil or cons)
(list? '(1 2 3))                     ; => 1
(list? nil)                          ; => 1
(list? 42)                           ; => nil

; pair? - check if value is a cons cell
(pair? '(1 . 2))                     ; => 1 (dotted pair)
(pair? '(1 2 3))                     ; => 1 (lists are chains of pairs)
(pair? (cons 1 2))                   ; => 1
(pair? nil)                          ; => nil (nil is NOT a pair)
(pair? 42)                           ; => nil
```

### Number Comparisons

```lisp
(> 5 3)                              ; => 1
(< 5 3)                              ; => nil
(= 5 5)                              ; => 1
(>= 5 5)                             ; => 1
(<= 3 5)                             ; => 1
```

### Quotes

```lisp
(quote (1 2 3))                      ; => (1 2 3)
'(1 2 3)                             ; => (1 2 3) (shorthand)
'x                                   ; => x (symbol, not evaluated)
```

### Let Bindings

```lisp
(let ((x 10) (y 20))
  (+ x y))                           ; => 30
```

### Let\* Bindings (Sequential)

```lisp
; let* evaluates bindings sequentially
(let* ((x 5) (y (+ x 3))) y)         ; => 8

; Works with multiple dependent bindings
(let* ((a 10)
       (b (* a 2))
       (c (+ b 5)))
  c)                                  ; => 25

; Difference from let (which evaluates in parallel)
(let ((x 5) (y (+ x 3))) y)          ; ERROR: Undefined symbol: x
(let* ((x 5) (y (+ x 3))) y)         ; => 8 (works!)

; let* body behaves like progn - evaluates all expressions and returns last
(let* ((x 10))
  (+ x 5)
  (* x 2)
  (- x 3))                            ; => 7 (returns last expression)
```

### Progn (Sequential Evaluation)

```lisp
; progn evaluates expressions sequentially and returns the last value
(progn 1 2 3)                        ; => 3

; Used for executing multiple operations
(progn
  (define x 10)
  (define y 20)
  (+ x y))                           ; => 30

; With conditional logic
(progn
  (define a 5)
  (define b (* a 2))
  b)                                 ; => 10

; Empty progn returns NIL
(progn)                              ; => NIL
```

### Do Loop (Iteration)

**Syntax:**

```lisp
(do ((var init step) ...)
    (test result-expr ...)
  body ...)
```

**Components:**

- **Variable bindings** `((var init step) ...)` — Each binding declares a loop variable with an initial value and an optional step expression. On each iteration, all step expressions are evaluated (using the current values), then all variables are updated simultaneously.
- **Test clause** `(test result-expr ...)` — The test is evaluated at the start of each iteration. When it becomes truthy, the result expressions are evaluated sequentially like `progn` and the value of the last one is returned. If there are no result expressions, `nil` is returned.
- **Body** `body ...` — Evaluated on each iteration (for side effects) when the test is falsy.

**Examples:**

```lisp
; Simple counter from 0 to 9
(do ((i 0 (+ i 1)))
    ((>= i 10) i))                   ; => 10

; No result expression returns nil
(do ((i 0 (+ i 1)))
    ((= i 5)))                       ; => nil

; Multiple result expressions (like progn, returns last)
(do ((i 0 (+ i 1)))
    ((= i 3) 1 2 3))                ; => 3

; Result expressions evaluated for side effects
(define tracker nil)
(do ((i 0 (+ i 1)))
    ((= i 2) (set! tracker "done") 42))  ; => 42
tracker                                   ; => "done"

; Factorial using do loop
(define factorial-do
  (lambda (n)
    (do ((i n (- i 1))
         (acc 1 (* acc i)))
        ((<= i 0) acc))))

(factorial-do 5)                     ; => 120

; Count down with side effects in body
(do ((i 10 (- i 1)))
    ((<= i 0) "blastoff")
  i)                                  ; => "blastoff"

; Multiple variables
(do ((i 1 (+ i 1))
     (sum 0))
    ((> i 10) sum)
  (set! sum (+ sum i)))              ; => 55 (sum 1 to 10)

; Variable without step expression (constant through loop)
(do ((x 5))
    ((= x 5) x))                    ; => 5 (immediate return)
```

### Cond and Case (Multi-way Conditionals)

```lisp
; Grade calculator using cond
(define grade
  (lambda (score)
    (cond
      ((>= score 90) "A")
      ((>= score 80) "B")
      ((>= score 70) "C")
      ((>= score 60) "D")
      (else "F"))))

(grade 95)  ; => "A"
(grade 75)  ; => "C"

; Day of week using case
(define day-name
  (lambda (n)
    (case n
      ((1) "Monday")
      ((2) "Tuesday")
      ((6 7) "Weekend")
      (else "Invalid"))))

(day-name 1)   ; => "Monday"
(day-name 6)   ; => "Weekend"
(day-name 99)  ; => "Invalid"
```

### Global State Management

```lisp
; Variables persist across the REPL session and can be updated
(define counter 0)                   ; Define initial value
counter                              ; => 0
(define counter 1)                   ; Update global state
counter                              ; => 1
(define counter (+ counter 5))       ; Update using previous value
counter                              ; => 6

; Use progn for multi-step state updates
(define balance 100)
(progn
  (define balance (+ balance 50))
  (define balance (- balance 30))
  balance)                           ; => 120

; Use set! to mutate from inside lambdas/hooks
(define counter 0)
(define increment (lambda ()
  (set! counter (+ counter 1))
  counter))
(increment)                         ; => 1
(increment)                         ; => 2
counter                             ; => 2 (globally updated!)

; Variables can be reassigned with different types
(define x 42)
x                                   ; => 42
(define x "hello")
x                                   ; => "hello"
```

### Set! for Mutating State

```lisp
; set! updates existing variables (variable must exist)
(define msg_count 0)
(set! msg_count 10)
msg_count                            ; => 10

; Works inside lambdas for stateful functions
(define counter 0)
(define increment (lambda ()
  (set! counter (+ counter 1))
  counter))
(increment)                         ; => 1
(increment)                         ; => 2
counter                             ; => 2

; Example: Stateful counter function
(define message_count 0)
(define count_messages (lambda ()
  (set! message_count (+ message_count 1))
  message_count))                    ; Return current count

(count_messages)                     ; => 1
(count_messages)                     ; => 2
message_count                        ; => 2
```

vec ; => #(10 99 30)
(length vec) ; => 3

; Pop elements
(vector-pop! vec) ; => 30
vec ; => #(10 99)

````

### Complex Example

```lisp
; Factorial function
(define factorial
  (lambda (n)
    (if (<= n 1)
      1
      (* n (factorial (- n 1))))))

(factorial 5)                        ; => 120

; String processing
(define greet
  (lambda (name)
    (concat "Hello, " name "!")))

(greet "World")                      ; => "Hello, World!"
````

## See Also

- [BUILTIN_REFERENCE.md](BUILTIN_REFERENCE.md) - Auto-generated reference for all built-in functions, macros, and variables
- [README.md](README.md) - Project overview, building, embedding, and C API reference
- [tests/](tests/) - Test files with more examples
- [doc/](doc/) - Per-function documentation source files
