#include "builtins_internal.h"

/* Arithmetic loop: validate + accumulate numeric args */
#define ARITH_LOOP(accum, op, name)                                                \
    while (args != NIL && args != NULL) {                                          \
        LispObject *_a = lisp_car(args);                                           \
        int _ai = 0;                                                               \
        double _v = get_numeric_value(_a, &_ai);                                   \
        if (!_ai && LISP_TYPE(_a) != LISP_NUMBER && LISP_TYPE(_a) != LISP_INTEGER) \
            return lisp_make_error(name " requires numbers");                      \
        if (!_ai)                                                                  \
            all_integers = 0;                                                      \
        accum op _v;                                                               \
        args = lisp_cdr(args);                                                     \
    }

/* Return integer if all args were ints, else float */
#define ARITH_RETURN(accum)                                     \
    return all_integers ? lisp_make_integer((long long)(accum)) \
                        : lisp_make_number(accum)

static LispObject *builtin_add(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_integer(0);

    int all_integers = 1;
    double sum = 0;
    ARITH_LOOP(sum, +=, "+");
    ARITH_RETURN(sum);
}

static LispObject *builtin_subtract(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("-");

    LispObject *first = lisp_car(args);
    int first_is_integer = 0;
    double result = get_numeric_value(first, &first_is_integer);
    int all_integers = first_is_integer;
    args = lisp_cdr(args);

    if (args == NIL)
        return first_is_integer ? lisp_make_integer((long long)-result)
                                : lisp_make_number(-result);

    ARITH_LOOP(result, -=, "-");
    ARITH_RETURN(result);
}

static LispObject *builtin_multiply(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_integer(1);

    int all_integers = 1;
    double product = 1;
    ARITH_LOOP(product, *=, "*");
    ARITH_RETURN(product);
}
#undef ARITH_LOOP
#undef ARITH_RETURN

static LispObject *builtin_divide(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_make_error("/ requires at least one argument");
    }

    LispObject *first = lisp_car(args);
    int first_is_integer;
    double result = get_numeric_value(first, &first_is_integer);
    args = lisp_cdr(args);

    if (args == NIL) {
        /* Unary reciprocal */
        if (result == 0) {
            return lisp_make_error("Division by zero");
        }
        return lisp_make_number(1.0 / result); /* Always return float */
    }

    /* Division always returns float */
    while (args != NIL && args != NULL) {
        LispObject *arg = lisp_car(args);
        int arg_is_integer = 0;
        double val = get_numeric_value(arg, &arg_is_integer);
        if (arg_is_integer == 0 && LISP_TYPE(arg) != LISP_NUMBER && LISP_TYPE(arg) != LISP_INTEGER) {
            return lisp_make_error("/ requires numbers");
        }
        if (val == 0) {
            return lisp_make_error("Division by zero");
        }
        result /= val;
        args = lisp_cdr(args);
    }

    return lisp_make_number(result); /* Always return float */
}

static LispObject *builtin_quotient(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("quotient");

    LispObject *first = lisp_car(args);
    LispObject *second = lisp_car(lisp_cdr(args));

    if (LISP_TYPE(first) != LISP_INTEGER && LISP_TYPE(first) != LISP_NUMBER) {
        return lisp_make_error("quotient requires numbers");
    }
    if (LISP_TYPE(second) != LISP_INTEGER && LISP_TYPE(second) != LISP_NUMBER) {
        return lisp_make_error("quotient requires numbers");
    }

    int first_is_integer;
    int second_is_integer;
    double first_val = get_numeric_value(first, &first_is_integer);
    double second_val = get_numeric_value(second, &second_is_integer);

    if (second_val == 0) {
        return lisp_make_error("Division by zero");
    }

    /* Truncate to integer */
    long long result = (long long)(first_val / second_val);
    return lisp_make_integer(result);
}

static LispObject *builtin_remainder(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("remainder");

    LispObject *first = lisp_car(args);
    LispObject *second = lisp_car(lisp_cdr(args));

    if (LISP_TYPE(first) != LISP_INTEGER && LISP_TYPE(first) != LISP_NUMBER) {
        return lisp_make_error("remainder requires numbers");
    }
    if (LISP_TYPE(second) != LISP_INTEGER && LISP_TYPE(second) != LISP_NUMBER) {
        return lisp_make_error("remainder requires numbers");
    }

    int first_is_integer;
    int second_is_integer;
    double first_val = get_numeric_value(first, &first_is_integer);
    double second_val = get_numeric_value(second, &second_is_integer);

    if (second_val == 0) {
        return lisp_make_error("Division by zero");
    }

    long long result = (long long)first_val % (long long)second_val;
    return lisp_make_integer(result);
}

static LispObject *builtin_floor(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("floor");

    LispObject *first = lisp_car(args);
    LispObject *second = lisp_car(lisp_cdr(args));

    if (LISP_TYPE(first) != LISP_INTEGER && LISP_TYPE(first) != LISP_NUMBER) {
        return lisp_make_error("floor requires numbers");
    }
    if (LISP_TYPE(second) != LISP_INTEGER && LISP_TYPE(second) != LISP_NUMBER) {
        return lisp_make_error("floor requires numbers");
    }

    int first_is_integer;
    int second_is_integer;
    long long first_val = (long long)get_numeric_value(first, &first_is_integer);
    long long second_val = (long long)get_numeric_value(second, &second_is_integer);

    if (second_val == 0) {
        return lisp_make_error("Division by zero");
    }

    long long result = first_val / second_val;

    // Round toward negative infinity if signs differ and there's a remainder
    if ((first_val < 0) != (second_val < 0) && first_val % second_val != 0) {
        result--;
    }

    return lisp_make_integer(result);
}

static LispObject *builtin_modulo(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("modulo");

    LispObject *first = lisp_car(args);
    LispObject *second = lisp_car(lisp_cdr(args));

    if (LISP_TYPE(first) != LISP_INTEGER && LISP_TYPE(first) != LISP_NUMBER) {
        return lisp_make_error("modulo requires numbers");
    }
    if (LISP_TYPE(second) != LISP_INTEGER && LISP_TYPE(second) != LISP_NUMBER) {
        return lisp_make_error("modulo requires numbers");
    }

    int first_is_integer;
    int second_is_integer;
    long long first_val = (long long)get_numeric_value(first, &first_is_integer);
    long long second_val = (long long)get_numeric_value(second, &second_is_integer);

    if (second_val == 0) {
        return lisp_make_error("Division by zero");
    }

    long long result = first_val % second_val;

    // Ensure result has same sign as divisor
    if ((result > 0 && second_val < 0) || (result < 0 && second_val > 0)) {
        result += second_val;
    }

    return lisp_make_integer(result);
}

static LispObject *builtin_even_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("even?");

    LispObject *arg = lisp_car(args);
    int arg_is_integer;
    double arg_val = get_numeric_value(arg, &arg_is_integer);

    if (LISP_TYPE(arg) != LISP_INTEGER && LISP_TYPE(arg) != LISP_NUMBER) {
        return lisp_make_error("even? requires a number");
    }

    long long val = (long long)arg_val;
    if ((val & 1) == 0) {
        return lisp_make_boolean(1);
    }
    return lisp_make_boolean(0);
}

static LispObject *builtin_odd_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("odd?");

    LispObject *arg = lisp_car(args);
    int arg_is_integer;
    double arg_val = get_numeric_value(arg, &arg_is_integer);

    if (LISP_TYPE(arg) != LISP_INTEGER && LISP_TYPE(arg) != LISP_NUMBER) {
        return lisp_make_error("odd? requires a number");
    }

    long long val = (long long)arg_val;
    if ((val & 1) == 1) {
        return lisp_make_boolean(1);
    }
    return lisp_make_boolean(0);
}

static LispObject *builtin_max(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("max requires at least one argument");

    int all_integers = 1;
    LispObject *first = lisp_car(args);
    int first_is_integer = 0;
    double result = get_numeric_value(first, &first_is_integer);
    if (!first_is_integer && LISP_TYPE(first) != LISP_NUMBER && LISP_TYPE(first) != LISP_INTEGER)
        return lisp_make_error("max requires numbers");
    if (!first_is_integer)
        all_integers = 0;

    args = lisp_cdr(args);
    while (args != NIL && args != NULL) {
        LispObject *arg = lisp_car(args);
        int arg_is_integer = 0;
        double val = get_numeric_value(arg, &arg_is_integer);
        if (!arg_is_integer && LISP_TYPE(arg) != LISP_NUMBER && LISP_TYPE(arg) != LISP_INTEGER)
            return lisp_make_error("max requires numbers");
        if (!arg_is_integer)
            all_integers = 0;
        if (val > result)
            result = val;
        args = lisp_cdr(args);
    }

    return all_integers ? lisp_make_integer((long long)result)
                        : lisp_make_number(result);
}

static LispObject *builtin_min(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("min requires at least one argument");

    int all_integers = 1;
    LispObject *first = lisp_car(args);
    int first_is_integer = 0;
    double result = get_numeric_value(first, &first_is_integer);
    if (!first_is_integer && LISP_TYPE(first) != LISP_NUMBER && LISP_TYPE(first) != LISP_INTEGER)
        return lisp_make_error("min requires numbers");
    if (!first_is_integer)
        all_integers = 0;

    args = lisp_cdr(args);
    while (args != NIL && args != NULL) {
        LispObject *arg = lisp_car(args);
        int arg_is_integer = 0;
        double val = get_numeric_value(arg, &arg_is_integer);
        if (!arg_is_integer && LISP_TYPE(arg) != LISP_NUMBER && LISP_TYPE(arg) != LISP_INTEGER)
            return lisp_make_error("min requires numbers");
        if (!arg_is_integer)
            all_integers = 0;
        if (val < result)
            result = val;
        args = lisp_cdr(args);
    }

    return all_integers ? lisp_make_integer((long long)result)
                        : lisp_make_number(result);
}

/* ---- Bitwise Operations ---- */

static LispObject *builtin_logand(LispObject *args, Environment *env)
{
    (void)env;
    long long result = -1; /* Identity: all bits set */

    while (args != NIL && args != NULL) {
        LispObject *arg = lisp_car(args);
        if (LISP_TYPE(arg) != LISP_INTEGER)
            return lisp_make_error("logand requires integers");
        result &= LISP_INT_VAL(arg);
        args = lisp_cdr(args);
    }

    return lisp_make_integer(result);
}

static LispObject *builtin_logior(LispObject *args, Environment *env)
{
    (void)env;
    long long result = 0; /* Identity: no bits set */

    while (args != NIL && args != NULL) {
        LispObject *arg = lisp_car(args);
        if (LISP_TYPE(arg) != LISP_INTEGER)
            return lisp_make_error("logior requires integers");
        result |= LISP_INT_VAL(arg);
        args = lisp_cdr(args);
    }

    return lisp_make_integer(result);
}

static LispObject *builtin_logxor(LispObject *args, Environment *env)
{
    (void)env;
    long long result = 0; /* Identity: no bits set */

    while (args != NIL && args != NULL) {
        LispObject *arg = lisp_car(args);
        if (LISP_TYPE(arg) != LISP_INTEGER)
            return lisp_make_error("logxor requires integers");
        result ^= LISP_INT_VAL(arg);
        args = lisp_cdr(args);
    }

    return lisp_make_integer(result);
}

static LispObject *builtin_lognot(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("lognot");

    LispObject *arg = lisp_car(args);
    if (LISP_TYPE(arg) != LISP_INTEGER)
        return lisp_make_error("lognot requires an integer");

    return lisp_make_integer(~LISP_INT_VAL(arg));
}

static LispObject *builtin_ash(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("ash");

    LispObject *value_obj = lisp_car(args);
    LispObject *count_obj = lisp_car(lisp_cdr(args));

    if (LISP_TYPE(value_obj) != LISP_INTEGER)
        return lisp_make_error("ash requires an integer value");
    if (LISP_TYPE(count_obj) != LISP_INTEGER)
        return lisp_make_error("ash requires an integer count");

    long long value = LISP_INT_VAL(value_obj);
    long long count = LISP_INT_VAL(count_obj);

    if (count >= 0)
        return lisp_make_integer(value << count);
    else
        return lisp_make_integer(value >> (-count));
}

static LispObject *builtin_logcount(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("logcount");

    LispObject *arg = lisp_car(args);
    if (LISP_TYPE(arg) != LISP_INTEGER)
        return lisp_make_error("logcount requires an integer");

    long long value = LISP_INT_VAL(arg);
    int count = 0;

    /* Count 1-bits using two's complement semantics */
    if (value >= 0) {
        while (value) {
            count += value & 1;
            value >>= 1;
        }
    } else {
        /* For negative numbers, count 0-bits in two's complement */
        value = ~value;
        while (value) {
            count += value & 1;
            value >>= 1;
        }
    }

    return lisp_make_integer(count);
}

void register_arithmetic_builtins(Environment *env)
{
    REGISTER("+", builtin_add);
    REGISTER("-", builtin_subtract);
    REGISTER("*", builtin_multiply);
    REGISTER("/", builtin_divide);
    REGISTER("quotient", builtin_quotient);
    REGISTER("remainder", builtin_remainder);
    REGISTER("floor", builtin_floor);
    REGISTER("modulo", builtin_modulo);
    REGISTER("even?", builtin_even_question);
    REGISTER("odd?", builtin_odd_question);
    REGISTER("max", builtin_max);
    REGISTER("min", builtin_min);
    REGISTER("logand", builtin_logand);
    REGISTER("logior", builtin_logior);
    REGISTER("logxor", builtin_logxor);
    REGISTER("lognot", builtin_lognot);
    REGISTER("ash", builtin_ash);
    REGISTER("logcount", builtin_logcount);
}
