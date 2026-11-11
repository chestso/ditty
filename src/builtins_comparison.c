#include "builtins_internal.h"

#include <stdio.h>

/* Number comparisons. All five predicates take two or more arguments and
   check every adjacent pair, comparing exactly (see number_compare). */

enum numeric_compare_mode
{
    NUMERIC_EQ,
    NUMERIC_LT,
    NUMERIC_GT,
    NUMERIC_LE,
    NUMERIC_GE
};

static int numeric_compare_accepts(enum numeric_compare_mode mode, int cmp)
{
    switch (mode) {
    case NUMERIC_EQ:
        return cmp == 0;
    case NUMERIC_LT:
        return cmp < 0;
    case NUMERIC_GT:
        return cmp > 0;
    case NUMERIC_LE:
        return cmp <= 0;
    case NUMERIC_GE:
        return cmp >= 0;
    }
    return 0;
}

static LispObject *numeric_compare_chain(LispObject *args, const char *name, enum numeric_compare_mode mode)
{
    char message[64];

    if (args == NIL || lisp_cdr(args) == NIL) {
        snprintf(message, sizeof(message), "%s requires 2 arguments", name);
        return lisp_raise_error(message);
    }

    LispObject *previous = lisp_car(args);
    LispType previous_type = LISP_TYPE(previous);
    if (previous_type != LISP_INTEGER && previous_type != LISP_NUMBER) {
        snprintf(message, sizeof(message), "%s requires numbers", name);
        return lisp_raise_error(message);
    }

    for (args = lisp_cdr(args); args != NIL && args != NULL; args = lisp_cdr(args)) {
        LispObject *current = lisp_car(args);
        LispType current_type = LISP_TYPE(current);
        if (current_type != LISP_INTEGER && current_type != LISP_NUMBER) {
            snprintf(message, sizeof(message), "%s requires numbers", name);
            return lisp_raise_error(message);
        }

        int unordered = 0;
        int cmp = number_compare(previous, current, &unordered);
        if (unordered || !numeric_compare_accepts(mode, cmp))
            return NIL;

        previous = current;
    }

    return LISP_TRUE;
}

static LispObject *builtin_gt(LispObject *args, Environment *env)
{
    (void)env;
    return numeric_compare_chain(args, ">", NUMERIC_GT);
}

static LispObject *builtin_lt(LispObject *args, Environment *env)
{
    (void)env;
    return numeric_compare_chain(args, "<", NUMERIC_LT);
}

static LispObject *builtin_eq(LispObject *args, Environment *env)
{
    (void)env;
    return numeric_compare_chain(args, "=", NUMERIC_EQ);
}

static LispObject *builtin_gte(LispObject *args, Environment *env)
{
    (void)env;
    return numeric_compare_chain(args, ">=", NUMERIC_GE);
}

static LispObject *builtin_lte(LispObject *args, Environment *env)
{
    (void)env;
    return numeric_compare_chain(args, "<=", NUMERIC_LE);
}

/* Boolean operations */
static LispObject *builtin_not(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("not");

    LispObject *arg = lisp_car(args);
    return lisp_is_truthy(arg) ? NIL : LISP_TRUE;
}

/* Equality predicates */

static LispObject *builtin_eq_question(LispObject *args, Environment *env)
{
    (void)env;
    /* Validate exactly 2 arguments */
    if (args == NIL || LISP_CDR(args) == NIL || LISP_CDR(LISP_CDR(args)) != NIL) {
        return lisp_raise_error("eq? expects exactly 2 arguments");
    }

    LispObject *a = LISP_CAR(args);
    LispObject *b = LISP_CAR(LISP_CDR(args));

    /* Pointer equality - same object in memory */
    return (a == b) ? LISP_TRUE : NIL;
}

static LispObject *builtin_equal_question(LispObject *args, Environment *env)
{
    (void)env;
    /* Validate exactly 2 arguments */
    if (args == NIL || LISP_CDR(args) == NIL || LISP_CDR(LISP_CDR(args)) != NIL) {
        return lisp_raise_error("equal? expects exactly 2 arguments");
    }

    LispObject *a = LISP_CAR(args);
    LispObject *b = LISP_CAR(LISP_CDR(args));

    /* Use recursive structural equality */
    return objects_equal_recursive(a, b) ? LISP_TRUE : NIL;
}

static LispObject *builtin_string_eq_question(LispObject *args, Environment *env)
{
    (void)env;
    /* Validate exactly 2 arguments */
    if (args == NIL || LISP_CDR(args) == NIL || LISP_CDR(LISP_CDR(args)) != NIL) {
        return lisp_raise_error("string=? expects exactly 2 arguments");
    }

    LispObject *a = LISP_CAR(args);
    LispObject *b = LISP_CAR(LISP_CDR(args));

    /* Type validation */
    if (LISP_TYPE(a) != LISP_STRING) {
        return lisp_raise_error("string=?: first argument must be a string");
    }
    if (LISP_TYPE(b) != LISP_STRING) {
        return lisp_raise_error("string=?: second argument must be a string");
    }

    /* String comparison */
    return (strcmp(LISP_STR_VAL(a), LISP_STR_VAL(b)) == 0) ? LISP_TRUE : NIL;
}

void register_comparison_builtins(Environment *env)
{
    REGISTER(">", builtin_gt);
    REGISTER("<", builtin_lt);
    REGISTER("=", builtin_eq);
    REGISTER(">=", builtin_gte);
    REGISTER("<=", builtin_lte);
    REGISTER("not", builtin_not);
    REGISTER("eq?", builtin_eq_question);
    REGISTER("equal?", builtin_equal_question);
    REGISTER("string=?", builtin_string_eq_question);
}
