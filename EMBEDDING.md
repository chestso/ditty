# Embedding Ditty in Your Application

Ditty is a self-contained C library (`libditty.a`) that can be integrated into any C project. This document covers the embedder-facing API, the tagged-pointer object representation, and the Boehm GC rules you must follow when holding live `LispObject *` values in your own structs.

For the Lisp language itself, see [LANGUAGE_GUIDE.md](LANGUAGE_GUIDE.md) (concepts) and [BUILTIN_REFERENCE.md](BUILTIN_REFERENCE.md) (function reference). For build instructions, see [README.md](README.md).

## Minimal example

```c
#include <ditty/lisp.h>

int main() {
    Environment* env = lisp_init();

    LispObject* result = NULL;
    if (lisp_try_eval_string("(+ 1 2 3)", env, &result) == LISP_OK) {
        char* output = lisp_print(result);
        printf("%s\n", output);  // Prints: 6
    } else {
        fprintf(stderr, "%s\n", lisp_error_report(result));
    }

    lisp_cleanup();
    return 0;
}
```

Memory is managed by Boehm GC. Call `lisp_cleanup()` once at program exit.

> **Signals versus data.** Lisp signaling is a non-local exit (`longjmp` to the
> innermost `condition-case`), not an error return value. Run Lisp from C
> through the **protected** API — `lisp_try`, `lisp_try_eval`,
> `lisp_try_apply`, `lisp_try_eval_string`, `lisp_try_load_file` — which
> returns a `LispStatus` (`LISP_OK` / `LISP_SIGNALED`) plus an out-parameter.
> On `LISP_OK` the out-parameter holds the value, which may itself be an error
> object (that is data, not a failure). On `LISP_SIGNALED` it holds the
> signaled error.
>
> `lisp_eval`, `lisp_apply`, `lisp_call_*` and `lisp_load_file` **raise** and
> are only valid while an eval frame is active — from inside a builtin or a
> `lisp_try_*` call. Calling them with no frame aborts with an internal-error
> diagnostic.

> **Embedder contract.** A `LispObject *` is **not** always a real pointer — it may be a tagged immediate value (small integer, char, `NIL`, `LISP_TRUE`). Never read `obj->type` or `obj->value.X` directly; always go through the `LISP_*` accessor macros (`LISP_TYPE`, `LISP_INT_VAL`, `LISP_CAR`, `LISP_CDR`, `LISP_LAMBDA_NAME`, ...). The macros are defined in `<ditty/lisp_value.h>` and decode the tag bits and follow boxed-out pointers transparently. See [Object representation & GC](#object-representation--gc) below.

## Integration options

**pkg-config (after `make install`):**

```bash
# ditty (interpreter + syntax highlighting)
cc myapp.c $(pkg-config --cflags --libs ditty) -o myapp
```

**Copy into your project** and link directly:

```bash
cc -I./libs/ditty/include \
   myapp.c \
   ./libs/ditty/src/libditty.a \
   $(pkg-config --libs bdw-gc libpcre2-8) -lm \
   -o myapp
```

**Autotools subproject:**

```bash
# In your configure.ac
AC_CONFIG_SUBDIRS([libs/ditty])
```

```makefile
# In your Makefile.am
SUBDIRS = libs/ditty
myapp_LDADD = libs/ditty/src/libditty.a
```

## C API reference

### Core functions

| Function                                  | Description                                                |
| ----------------------------------------- | ---------------------------------------------------------- |
| `lisp_init()`                             | Initialize interpreter, return environment                 |
| `lisp_try_eval_string(code, env, &out)`   | Protected: parse and evaluate first expression in string   |
| `lisp_try_load_file(filename, env, &out)` | Protected: load and evaluate a file (all expressions)      |
| `lisp_try_eval(expr, env, &out)`          | Protected: evaluate a parsed expression                    |
| `lisp_try_apply(func, args, env, &out)`   | Protected: call a function with an evaluated argument list |
| `lisp_try(thunk, data, &out)`             | Protected: run an arbitrary C thunk with a frame installed |
| `lisp_load_file(filename, env)`           | Raising: load and evaluate a file                          |
| `lisp_cleanup()`                          | Free global resources                                      |

### Parsing and evaluation

| Function               | Description                                              |
| ---------------------- | -------------------------------------------------------- |
| `lisp_read(input)`     | Parse input into AST (`const char **`, advances pointer) |
| `lisp_eval(expr, env)` | Evaluate an expression                                   |
| `lisp_print(obj)`      | Convert object to string (GC-managed)                    |
| `lisp_princ(obj)`      | Print without quotes (human-readable)                    |
| `lisp_prin1(obj)`      | Print with quotes (machine-readable)                     |
| `lisp_print_cl(obj)`   | Print Emacs Lisp–style                                   |

### Function invocation

| Function                             | Description                             |
| ------------------------------------ | --------------------------------------- |
| `lisp_apply(func, args, env)`        | Call a function with evaluated arg list |
| `lisp_call_0(func, env)`             | Call a function with 0 arguments        |
| `lisp_call_1(func, arg1, env)`       | Call a function with 1 argument         |
| `lisp_call_2(func, arg1, arg2, env)` | Call a function with 2 arguments        |
| `lisp_call_3(func, a1, a2, a3, env)` | Call a function with 3 arguments        |

### Object creation

| Function                                        | Description                                  |
| ----------------------------------------------- | -------------------------------------------- |
| `lisp_make_number(value)`                       | Create floating-point number                 |
| `lisp_make_integer(value)`                      | Create integer (tagged fixnum or heap)       |
| `lisp_make_char(codepoint)`                     | Create character (tagged immediate)          |
| `lisp_make_string(value)`                       | Create string                                |
| `lisp_intern(name)`                             | Intern a symbol (reuse existing if interned) |
| `lisp_make_symbol(name)`                        | Alias for `lisp_intern`                      |
| `lisp_make_keyword(name)`                       | Create keyword (separate interning table)    |
| `lisp_make_boolean(value)`                      | Create boolean                               |
| `lisp_make_cons(car, cdr)`                      | Create cons cell                             |
| `lisp_make_lambda(params, body, closure, name)` | Create lambda (simple)                       |
| `lisp_make_lambda_ext(...)`                     | Create lambda with optional/rest params      |
| `lisp_make_macro(params, body, closure, name)`  | Create macro                                 |
| `lisp_make_vector(capacity)`                    | Create vector                                |
| `lisp_make_hash_table()`                        | Create hash table                            |
| `lisp_make_typed_error(type, msg, data)`        | Pure error constructor (no raise, no trace)  |
| `lisp_make_builtin(func, name)`                 | Create builtin function                      |
| `lisp_make_string_port(str)`                    | Create input string port                     |
| `lisp_make_output_string_port()`                | Create output string port                    |
| `lisp_make_regex(code)`                         | Wrap compiled PCRE2 pattern (GC finalizer)   |
| `lisp_make_file_stream(file)`                   | Wrap `FILE*` as file stream                  |

### Error handling

| Function                                 | Description                                                            |
| ---------------------------------------- | ---------------------------------------------------------------------- |
| `lisp_make_typed_error(type, msg, data)` | Build an error object. Does not signal or capture a trace              |
| `lisp_raise(error)`                      | Signal an existing error object to the innermost frame (never returns) |
| `lisp_raise_error(msg)`                  | Signal a new `'error` (never returns)                                  |
| `lisp_raise_typed(type_name, msg)`       | Signal a new typed error from a C string (never returns)               |
| `lisp_raise_typed_data(type, msg, data)` | Signal a new typed error with a data payload (never returns)           |
| `lisp_error_report(error)`               | Multi-line diagnostic for a signaled error (`ERROR: [...] ...`)        |

The stack trace is captured by `lisp_raise` at the signal point, so every
error that is signaled carries the stack where it was raised. A returned
`LISP_ERROR` object is data: printing it yields `#<error TYPE "MESSAGE">`.

### Semantic classification

| Function            | Description                                   |
| ------------------- | --------------------------------------------- |
| `lisp_sf_kind(sym)` | Return `SfKind` for special form, or `-1`     |
| `lisp_sf_count()`   | Return number of special forms (currently 17) |

### Object inspection

| Function                 | Description                     |
| ------------------------ | ------------------------------- |
| `lisp_is_truthy(obj)`    | Test if object is truthy        |
| `lisp_is_list(obj)`      | Test if object is a proper list |
| `lisp_is_callable(obj)`  | Test if object is callable      |
| `lisp_list_length(list)` | Return length of a list         |

### List access

| Function           | Description           |
| ------------------ | --------------------- |
| `lisp_car(obj)`    | First element of cons |
| `lisp_cdr(obj)`    | Rest of cons          |
| `lisp_cadr(obj)`   | Second element        |
| `lisp_caddr(obj)`  | Third element         |
| `lisp_cadddr(obj)` | Fourth element        |
| `lisp_cdddr(obj)`  | Rest after three      |

> Mutating `set-car!` and `set-cdr!` are available as Lisp builtins, not as C API functions. Use `LISP_CAR(v)` and `LISP_CDR(v)` as lvalues to mutate cons cells from C.

### Value accessors (read these instead of `obj->value.X`)

Defined in `<ditty/lisp_value.h>`. Required for correctness — see [Object representation & GC](#object-representation--gc).

| Macro                              | What it returns                                        |
| ---------------------------------- | ------------------------------------------------------ |
| `LISP_TYPE(v)`                     | The `LispType` of `v` (decodes tag bits transparently) |
| `LISP_INT_VAL(v)`                  | `long long` value of a fixnum (tagged or heap)         |
| `LISP_NUM_VAL(v)`                  | `double` value of a heap-allocated number              |
| `LISP_CHAR_VAL(v)`                 | `uint32_t` Unicode codepoint                           |
| `LISP_BOOL_VAL(v)`                 | `int` — `1` if `v == LISP_TRUE`, else `0`              |
| `LISP_STR_VAL(v)`                  | `char *` for `LISP_STRING`                             |
| `LISP_SYM_VAL(v)`                  | `Symbol *` for `LISP_SYMBOL`/`LISP_KEYWORD`            |
| `LISP_CAR(v)`, `LISP_CDR(v)`       | Cons fields (lvalues — assignable)                     |
| `LISP_LAMBDA_NAME(v)`, etc.        | Lambda fields (follows the boxed-out `LambdaInfo *`)   |
| `LISP_ERROR_MESSAGE(v)`, etc.      | Error fields (follows the boxed-out `ErrorInfo *`)     |
| `LISP_VECTOR_ITEMS(v)`, etc.       | Vector fields                                          |
| `LISP_HT_BUCKETS(v)`, etc.         | Hash-table fields                                      |
| `LISP_REGEX_CODE(v)`               | `pcre2_code *` for `LISP_REGEX`                        |
| `LISP_STRING_PORT_BUFFER(v)`, etc. | String port fields                                     |

There's one of these per (type, field) pair for boxed-out variants and one per scalar/struct member otherwise. Field names match the union member names.

### Hash table operations

| Function                                | Description             |
| --------------------------------------- | ----------------------- |
| `hash_table_get_entry(table, key)`      | Look up entry by key    |
| `hash_table_set_entry(table, key, val)` | Insert or update entry  |
| `hash_table_remove_entry(table, key)`   | Remove entry by key     |
| `hash_table_clear(table)`               | Remove all entries      |
| `hash_keys_equal(a, b)`                 | Key equality comparison |

### Environment management

| Function                                   | Description                          |
| ------------------------------------------ | ------------------------------------ |
| `env_create(parent)`                       | Create child environment frame       |
| `env_define(env, sym, value, package)`     | Define variable in package           |
| `env_lookup(env, sym)`                     | Look up variable                     |
| `env_lookup_in_package(env, sym, package)` | Look up variable in specific package |
| `env_set(env, sym, value)`                 | Update existing variable             |
| `env_current_package(env)`                 | Get current package symbol           |

## Object representation & GC

`sizeof(LispObject) == 24 bytes`. Every value passed around the C API is either a real pointer to a 24-byte heap object or a tagged immediate encoded directly in the `LispObject *` value itself. Both kinds coexist with Boehm GC's conservative scanner without violating its invariants.

### Tag scheme

The low 3 bits of the `LispObject *` value act as a tag:

| Low 3 bits | Meaning      | Encoding                                        |
| ---------- | ------------ | ----------------------------------------------- |
| `000`      | heap pointer | 8-byte-aligned `LispObject *` (real heap obj)   |
| `001`      | fixnum       | `(int64 << 3) \| 1` — 61-bit signed integer     |
| `010`      | char         | `(uint32 << 3) \| 2` — 21-bit Unicode codepoint |
| `011`      | singleton    | `NIL = 0x3`, `LISP_TRUE = 0xB`                  |
| `100..111` | reserved     | (future tags)                                   |

Fixnums up to ±2<sup>60</sup>, all chars, `NIL`, and `LISP_TRUE` need no allocation at all — they're encoded in the pointer value. Bigger integers, doubles, strings, symbols, conses, lambdas, etc. live on the heap with the low 3 bits zero.

### How this stays Boehm-GC-safe

Boehm GC scans memory looking for bit patterns that match real allocated heap pointers. Two failure modes to consider:

1. **A real heap pointer being missed by GC.** This would cause use-after-free. We avoid it by keeping every heap pointer as a real, untouched 8-byte-aligned address — Boehm sees those as pointers exactly as it always has.

2. **A tagged immediate being mistaken for a pointer.** A fixnum or char with low bits `001`/`010` could occasionally land on a value that falls inside a Boehm-managed heap region. When that happens GC conservatively keeps the pointed-at block alive longer than necessary. This is **false retention**, not corruption — the wrong object isn't freed, it's the _right_ object that lingers an extra cycle. Stress tests show the overhead is well under 5%.

### Boxed-out info structs

Lambda, macro, error, string-port, vector, and hash-table objects don't fit comfortably in 24 bytes. They live as separately-allocated `*Info` structs (`LambdaInfo`, `MacroInfo`, `ErrorInfo`, `StringPortInfo`, `VectorInfo`, `HashTableInfo`) with the wrapping `LispObject` holding only an 8-byte pointer to them. The `LISP_LAMBDA_NAME(v)`, `LISP_ERROR_MESSAGE(v)`, etc. macros follow that indirection for you.

### What this means for embedders

- **`obj->type` is illegal.** Use `LISP_TYPE(obj)`. It checks tag bits before dereferencing, so it's correct on tagged immediates too.
- **`obj->value.integer` is illegal for tagged fixnums.** Use `LISP_INT_VAL(obj)` — for tagged values it shifts the tag off; for big-int heap objects it reads the field.
- **`obj == NIL` and `obj == LISP_TRUE` are fine.** Both are compile-time constants; pointer comparison just compares against `0x3` / `0xB`.
- **Constructing heap objects from C** (e.g. building a custom builtin that returns a `LispObject`): use `lisp_make_*` constructors, never hand-roll a `LispObject` on the stack — Boehm needs to see the allocation.
- **Holding a `LispObject *` across a possible GC trigger.** Per the [Boehm GC allocation rule](#boehm-gc-allocation-rule-internal-data-structures), any C struct that holds a `LispObject *` must itself be allocated with `GC_malloc`, otherwise GC can't see the pointer. Locals on the stack are scanned automatically — but declare short-lived locals `volatile` if the optimizer might elide them.

### Boehm GC allocation rule (internal data structures)

When you write internal C structs that hold `LispObject *` or `Environment *` fields:

- Allocate the struct with `GC_malloc`, not `malloc`/`calloc`. Boehm only scans GC-allocated memory; a `malloc`-ed struct holding a `LispObject *` is invisible to it, and GC may collect what looks like an unreferenced object.
- Use `GC_strdup` (not `strdup`) for any string field that lives inside a GC-visible struct.
- Plain `malloc` is still fine for buffers that don't contain `LispObject *` (e.g. a temporary character buffer).

Symptoms of violation: intermittent "Undefined symbol" errors for builtins, or content-dependent random failures.
