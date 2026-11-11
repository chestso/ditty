#ifndef BUILTINS_INTERNAL_H
#define BUILTINS_INTERNAL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "../include/file_utils.h"
#include "../include/lisp.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "docstrings.gen.h"

/* ---- Argument validation macros ---- */
#define CHECK_ARGS_1(name) \
    if (args == NIL)       \
    return lisp_raise_error(name " requires 1 argument")
#define CHECK_ARGS_2(name)                    \
    if (args == NIL || lisp_cdr(args) == NIL) \
    return lisp_raise_error(name " requires 2 arguments")
#define CHECK_ARGS_3(name)                      \
    if (args == NIL || lisp_cdr(args) == NIL || \
        lisp_cdr(lisp_cdr(args)) == NIL)        \
    return lisp_raise_error(name " requires 3 arguments")

/* ---- List builder macro ---- */
#define LIST_APPEND(result, tail, item)                  \
    do {                                                 \
        LispObject *_node = lisp_make_cons((item), NIL); \
        if ((result) == NIL) {                           \
            (result) = _node;                            \
            (tail) = _node;                              \
        } else {                                         \
            LISP_CDR(tail) = _node;                      \
            (tail) = _node;                              \
        }                                                \
    } while (0)

/* Helper macro to register a builtin and set its symbol's docstring */
#define REGISTER(name, func)                                                         \
    do {                                                                             \
        LispObject *sym = lisp_intern(name);                                         \
        const char *_doc = lookup_builtin_doc(name);                                 \
        if (_doc != NULL)                                                            \
            LISP_SYM_VAL(sym)->docstring = (char *)_doc;                             \
        env_define(env, LISP_SYM_VAL(sym), lisp_make_builtin(func, name), pkg_core); \
    } while (0)

/* Helper function to get numeric value */
static inline double get_numeric_value(LispObject *obj, int *is_integer)
{
    LispType t = LISP_TYPE(obj);
    if (t == LISP_INTEGER) {
        *is_integer = 1;
        return (double)LISP_INT_VAL(obj);
    } else if (t == LISP_NUMBER) {
        *is_integer = 0;
        return LISP_NUM_VAL(obj);
    }
    return 0.0;
}

/* Convert a double to an integer Lisp object, staying a float when the value
   is too large for long long. Never cast a double outside long long range. */
static inline LispObject *make_integral_result(double v)
{
    if (!isfinite(v))
        return lisp_raise_error("result is not a finite number");
    if (v >= -9.2e18 && v <= 9.2e18)
        return lisp_make_integer((long long)v);
    return lisp_make_number(v);
}

/* Compare a long long with a double exactly. Sets *unordered when D is NaN. */
static inline int compare_int_double(long long i, double d, int *unordered)
{
    if (isnan(d)) {
        *unordered = 1;
        return 0;
    }
    if (d >= 9223372036854775808.0) /* 2^63 */
        return -1;
    if (d < -9223372036854775808.0)
        return 1;

    long long truncated = (long long)d; /* in range, so the cast is exact */
    if (i < truncated)
        return -1;
    if (i > truncated)
        return 1;

    /* Same integral part: the fractional part decides */
    double integral = (double)truncated;
    if (d == integral)
        return 0;
    return d > integral ? -1 : 1;
}

/* Compare two numbers exactly. Sets *unordered when either operand is NaN.
   Returns -1, 0, or 1. Only valid for LISP_INTEGER/LISP_NUMBER operands. */
static inline int number_compare(LispObject *a, LispObject *b, int *unordered)
{
    *unordered = 0;
    int a_is_integer = LISP_TYPE(a) == LISP_INTEGER;
    int b_is_integer = LISP_TYPE(b) == LISP_INTEGER;

    if (a_is_integer && b_is_integer) {
        long long av = LISP_INT_VAL(a);
        long long bv = LISP_INT_VAL(b);
        return (av > bv) - (av < bv);
    }

    if (!a_is_integer && !b_is_integer) {
        double av = LISP_NUM_VAL(a);
        double bv = LISP_NUM_VAL(b);
        if (isnan(av) || isnan(bv)) {
            *unordered = 1;
            return 0;
        }
        return (av > bv) - (av < bv);
    }

    if (a_is_integer)
        return compare_int_double(LISP_INT_VAL(a), LISP_NUM_VAL(b), unordered);
    return -compare_int_double(LISP_INT_VAL(b), LISP_NUM_VAL(a), unordered);
}

/* Cross-file dependency: defined in builtins_lists.c, used by builtins_comparison.c */
int objects_equal_recursive(LispObject *a, LispObject *b);

/* Per-topic registration functions */
void register_arithmetic_builtins(Environment *env);
void register_math_builtins(Environment *env);
void register_comparison_builtins(Environment *env);
void register_strings_builtins(Environment *env);
void register_characters_builtins(Environment *env);
void register_lists_builtins(Environment *env);
void register_vectors_builtins(Environment *env);
void register_hash_tables_builtins(Environment *env);
void register_regex_builtins(Environment *env);
void register_file_io_builtins(Environment *env);
void register_string_ports_builtins(Environment *env);
void register_type_predicates_builtins(Environment *env);
void register_symbols_builtins(Environment *env);
void register_functions_builtins(Environment *env);
void register_printing_builtins(Environment *env);
void register_errors_builtins(Environment *env);
void register_packages_builtins(Environment *env);
void register_filesystem_builtins(Environment *env);
void register_time_profiling_builtins(Environment *env);
void register_tui_builtins(Environment *env);

#endif /* BUILTINS_INTERNAL_H */
