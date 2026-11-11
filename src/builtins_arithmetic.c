#include "builtins_internal.h"

static LispObject *builtin_add(LispObject *args, Environment *env)
{
    (void)env;
    LispObject *head = args;
    long long integer_sum = 0;
    double float_sum = 0.0;
    int all_integers = 1;
    int overflow = 0;

    while (args != NIL && args != NULL) {
        LispObject *arg = lisp_car(args);
        LispType type = LISP_TYPE(arg);
        if (type == LISP_INTEGER) {
            long long value = LISP_INT_VAL(arg);
            float_sum += (double)value;
            if (!overflow && __builtin_add_overflow(integer_sum, value, &integer_sum))
                overflow = 1;
        } else if (type == LISP_NUMBER) {
            all_integers = 0;
            float_sum += LISP_NUM_VAL(arg);
        } else {
            return lisp_raise_error("+ requires numbers");
        }
        args = lisp_cdr(args);
    }

    if (all_integers && !overflow)
        return lisp_make_integer(integer_sum);

#ifdef __SIZEOF_INT128__
    /* Retry exact integer arithmetic with 128 bits: intermediates may
       overflow long long while the final result still fits. */
    if (all_integers) {
        __int128 wide_sum = 0;
        int wide_overflow = 0;
        for (args = head; args != NIL && args != NULL; args = lisp_cdr(args)) {
            if (__builtin_add_overflow(wide_sum, (__int128)LISP_INT_VAL(lisp_car(args)), &wide_sum)) {
                wide_overflow = 1;
                break;
            }
        }
        if (!wide_overflow && wide_sum >= LLONG_MIN && wide_sum <= LLONG_MAX)
            return lisp_make_integer((long long)wide_sum);
    }
#endif

    return lisp_make_number(float_sum);
}

static LispObject *builtin_subtract(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("-");

    LispObject *first = lisp_car(args);
    LispType first_type = LISP_TYPE(first);
    if (first_type != LISP_INTEGER && first_type != LISP_NUMBER)
        return lisp_raise_error("- requires numbers");

    LispObject *rest = lisp_cdr(args);

    if (rest == NIL) {
        if (first_type == LISP_INTEGER) {
            long long value = LISP_INT_VAL(first);
            if (value == LLONG_MIN)
                return lisp_make_number(-(double)value);
            return lisp_make_integer(-value);
        }
        return lisp_make_number(-LISP_NUM_VAL(first));
    }

    long long integer_result = first_type == LISP_INTEGER ? LISP_INT_VAL(first) : 0;
    double float_result = first_type == LISP_INTEGER ? (double)LISP_INT_VAL(first) : LISP_NUM_VAL(first);
    int all_integers = first_type == LISP_INTEGER;
    int overflow = 0;

    for (args = rest; args != NIL && args != NULL; args = lisp_cdr(args)) {
        LispObject *arg = lisp_car(args);
        LispType type = LISP_TYPE(arg);
        if (type == LISP_INTEGER) {
            long long value = LISP_INT_VAL(arg);
            float_result -= (double)value;
            if (all_integers && !overflow && __builtin_sub_overflow(integer_result, value, &integer_result))
                overflow = 1;
        } else if (type == LISP_NUMBER) {
            all_integers = 0;
            float_result -= LISP_NUM_VAL(arg);
        } else {
            return lisp_raise_error("- requires numbers");
        }
    }

    if (all_integers && !overflow)
        return lisp_make_integer(integer_result);

#ifdef __SIZEOF_INT128__
    if (all_integers) {
        __int128 wide_result = LISP_INT_VAL(first);
        int wide_overflow = 0;
        for (args = rest; args != NIL && args != NULL; args = lisp_cdr(args)) {
            if (__builtin_sub_overflow(wide_result, (__int128)LISP_INT_VAL(lisp_car(args)), &wide_result)) {
                wide_overflow = 1;
                break;
            }
        }
        if (!wide_overflow && wide_result >= LLONG_MIN && wide_result <= LLONG_MAX)
            return lisp_make_integer((long long)wide_result);
    }
#endif

    return lisp_make_number(float_result);
}

static LispObject *builtin_multiply(LispObject *args, Environment *env)
{
    (void)env;
    LispObject *head = args;
    long long integer_product = 1;
    double float_product = 1.0;
    int all_integers = 1;
    int overflow = 0;

    while (args != NIL && args != NULL) {
        LispObject *arg = lisp_car(args);
        LispType type = LISP_TYPE(arg);
        if (type == LISP_INTEGER) {
            long long value = LISP_INT_VAL(arg);
            float_product *= (double)value;
            if (!overflow && __builtin_mul_overflow(integer_product, value, &integer_product))
                overflow = 1;
        } else if (type == LISP_NUMBER) {
            all_integers = 0;
            float_product *= LISP_NUM_VAL(arg);
        } else {
            return lisp_raise_error("* requires numbers");
        }
        args = lisp_cdr(args);
    }

    if (all_integers && !overflow)
        return lisp_make_integer(integer_product);

#ifdef __SIZEOF_INT128__
    if (all_integers) {
        __int128 wide_product = 1;
        int wide_overflow = 0;
        for (args = head; args != NIL && args != NULL; args = lisp_cdr(args)) {
            if (__builtin_mul_overflow(wide_product, (__int128)LISP_INT_VAL(lisp_car(args)), &wide_product)) {
                wide_overflow = 1;
                break;
            }
        }
        if (!wide_overflow && wide_product >= LLONG_MIN && wide_product <= LLONG_MAX)
            return lisp_make_integer((long long)wide_product);
    }
#endif

    return lisp_make_number(float_product);
}

static LispObject *builtin_divide(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_raise_error("/ requires at least one argument");
    }

    LispObject *first = lisp_car(args);
    int first_is_integer;
    double result = get_numeric_value(first, &first_is_integer);
    args = lisp_cdr(args);

    if (args == NIL) {
        /* Unary reciprocal */
        if (result == 0) {
            return lisp_raise_error("Division by zero");
        }
        return lisp_make_number(1.0 / result); /* Always return float */
    }

    /* Division always returns float */
    while (args != NIL && args != NULL) {
        LispObject *arg = lisp_car(args);
        int arg_is_integer = 0;
        double val = get_numeric_value(arg, &arg_is_integer);
        if (arg_is_integer == 0 && LISP_TYPE(arg) != LISP_NUMBER && LISP_TYPE(arg) != LISP_INTEGER) {
            return lisp_raise_error("/ requires numbers");
        }
        if (val == 0) {
            return lisp_raise_error("Division by zero");
        }
        result /= val;
        args = lisp_cdr(args);
    }

    return lisp_make_number(result); /* Always return float */
}

static int arg_count(LispObject *args)
{
    int count = 0;
    while (args != NIL && args != NULL) {
        count++;
        args = lisp_cdr(args);
    }
    return count;
}

static int is_number_object(LispObject *obj)
{
    LispType type = LISP_TYPE(obj);
    return type == LISP_INTEGER || type == LISP_NUMBER;
}

/* Numeric value of a NUMBER or INTEGER object, as a double */
static double numeric_double(LispObject *obj)
{
    if (LISP_TYPE(obj) == LISP_INTEGER)
        return (double)LISP_INT_VAL(obj);
    return LISP_NUM_VAL(obj);
}

static LispObject *builtin_quotient(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("quotient");

    LispObject *first = lisp_car(args);
    LispObject *second = lisp_car(lisp_cdr(args));

    if (!is_number_object(first) || !is_number_object(second))
        return lisp_raise_error("quotient requires numbers");

    if (LISP_TYPE(first) == LISP_INTEGER && LISP_TYPE(second) == LISP_INTEGER) {
        long long first_val = LISP_INT_VAL(first);
        long long second_val = LISP_INT_VAL(second);

        if (second_val == 0)
            return lisp_raise_error("Division by zero");
        /* LLONG_MIN / -1 is undefined behavior */
        if (second_val == -1)
            return make_integral_result(-(double)first_val);
        return lisp_make_integer(first_val / second_val);
    }

    double first_val = numeric_double(first);
    double second_val = numeric_double(second);
    if (second_val == 0)
        return lisp_raise_error("Division by zero");

    double result = trunc(first_val / second_val);
    if (!isfinite(result))
        return lisp_raise_error("quotient: result is not a finite number");
    return make_integral_result(result);
}

static LispObject *builtin_remainder(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("remainder");

    LispObject *first = lisp_car(args);
    LispObject *second = lisp_car(lisp_cdr(args));

    if (!is_number_object(first) || !is_number_object(second))
        return lisp_raise_error("remainder requires numbers");

    if (LISP_TYPE(first) == LISP_INTEGER && LISP_TYPE(second) == LISP_INTEGER) {
        long long first_val = LISP_INT_VAL(first);
        long long second_val = LISP_INT_VAL(second);

        if (second_val == 0)
            return lisp_raise_error("Division by zero");
        /* LLONG_MIN % -1 is undefined behavior; the remainder is 0 */
        if (second_val == -1)
            return lisp_make_integer(0);
        return lisp_make_integer(first_val % second_val);
    }

    double first_val = numeric_double(first);
    double second_val = numeric_double(second);
    if (second_val == 0)
        return lisp_raise_error("Division by zero");

    double result = fmod(first_val, second_val);
    if (!isfinite(result))
        return lisp_raise_error("remainder: result is not a finite number");
    return lisp_make_number(result);
}

static LispObject *builtin_floor(LispObject *args, Environment *env)
{
    (void)env;
    int count = arg_count(args);
    if (count != 1 && count != 2)
        return lisp_raise_error("floor requires 1 or 2 arguments");

    LispObject *first = lisp_car(args);
    if (!is_number_object(first))
        return lisp_raise_error("floor requires numbers");

    if (count == 1) {
        if (LISP_TYPE(first) == LISP_INTEGER)
            return first;

        double result = floor(LISP_NUM_VAL(first));
        if (!isfinite(result))
            return lisp_raise_error("floor: result is not a finite number");
        return make_integral_result(result);
    }

    LispObject *second = lisp_car(lisp_cdr(args));
    if (!is_number_object(second))
        return lisp_raise_error("floor requires numbers");

    if (LISP_TYPE(first) == LISP_INTEGER && LISP_TYPE(second) == LISP_INTEGER) {
        long long first_val = LISP_INT_VAL(first);
        long long second_val = LISP_INT_VAL(second);

        if (second_val == 0)
            return lisp_raise_error("Division by zero");
        /* LLONG_MIN / -1 is undefined behavior */
        if (second_val == -1)
            return make_integral_result(-(double)first_val);

        long long result = first_val / second_val;
        /* Round toward negative infinity if signs differ and there's a remainder */
        if ((first_val < 0) != (second_val < 0) && first_val % second_val != 0)
            result--;
        return lisp_make_integer(result);
    }

    double first_val = numeric_double(first);
    double second_val = numeric_double(second);
    if (second_val == 0)
        return lisp_raise_error("Division by zero");

    double result = floor(first_val / second_val);
    if (!isfinite(result))
        return lisp_raise_error("floor: result is not a finite number");
    return make_integral_result(result);
}

static LispObject *builtin_modulo(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("modulo");

    LispObject *first = lisp_car(args);
    LispObject *second = lisp_car(lisp_cdr(args));

    if (!is_number_object(first) || !is_number_object(second))
        return lisp_raise_error("modulo requires numbers");

    if (LISP_TYPE(first) == LISP_INTEGER && LISP_TYPE(second) == LISP_INTEGER) {
        long long first_val = LISP_INT_VAL(first);
        long long second_val = LISP_INT_VAL(second);

        if (second_val == 0)
            return lisp_raise_error("Division by zero");
        /* LLONG_MIN % -1 is undefined behavior; the remainder is 0 */
        if (second_val == -1)
            return lisp_make_integer(0);

        long long result = first_val % second_val;
        /* Ensure result has same sign as divisor */
        if ((result > 0 && second_val < 0) || (result < 0 && second_val > 0))
            result += second_val;
        return lisp_make_integer(result);
    }

    double first_val = numeric_double(first);
    double second_val = numeric_double(second);
    if (second_val == 0)
        return lisp_raise_error("Division by zero");

    double result = fmod(first_val, second_val);
    if (result != 0 && ((result < 0) != (second_val < 0)))
        result += second_val;
    if (!isfinite(result))
        return lisp_raise_error("modulo: result is not a finite number");
    return lisp_make_number(result);
}

static LispObject *builtin_even_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("even?");

    LispObject *arg = lisp_car(args);
    LispType type = LISP_TYPE(arg);

    if (type == LISP_INTEGER)
        return (LISP_INT_VAL(arg) & 1) == 0 ? LISP_TRUE : NIL;

    if (type == LISP_NUMBER) {
        double value = LISP_NUM_VAL(arg);
        if (!isfinite(value))
            return lisp_raise_error("even?: argument must be finite");
        return fmod(trunc(value), 2.0) == 0 ? LISP_TRUE : NIL;
    }

    return lisp_raise_error("even? requires a number");
}

static LispObject *builtin_odd_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("odd?");

    LispObject *arg = lisp_car(args);
    LispType type = LISP_TYPE(arg);

    if (type == LISP_INTEGER)
        return (LISP_INT_VAL(arg) & 1) != 0 ? LISP_TRUE : NIL;

    if (type == LISP_NUMBER) {
        double value = LISP_NUM_VAL(arg);
        if (!isfinite(value))
            return lisp_raise_error("odd?: argument must be finite");
        return fmod(trunc(value), 2.0) != 0 ? LISP_TRUE : NIL;
    }

    return lisp_raise_error("odd? requires a number");
}

static LispObject *builtin_max(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_raise_error("max requires at least one argument");

    LispObject *best = lisp_car(args);
    LispType best_type = LISP_TYPE(best);
    if (best_type != LISP_INTEGER && best_type != LISP_NUMBER)
        return lisp_raise_error("max requires numbers");

    for (args = lisp_cdr(args); args != NIL && args != NULL; args = lisp_cdr(args)) {
        LispObject *arg = lisp_car(args);
        LispType type = LISP_TYPE(arg);
        if (type != LISP_INTEGER && type != LISP_NUMBER)
            return lisp_raise_error("max requires numbers");

        int unordered = 0;
        if (number_compare(arg, best, &unordered) > 0)
            best = arg;
    }

    return best;
}

static LispObject *builtin_min(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_raise_error("min requires at least one argument");

    LispObject *best = lisp_car(args);
    LispType best_type = LISP_TYPE(best);
    if (best_type != LISP_INTEGER && best_type != LISP_NUMBER)
        return lisp_raise_error("min requires numbers");

    for (args = lisp_cdr(args); args != NIL && args != NULL; args = lisp_cdr(args)) {
        LispObject *arg = lisp_car(args);
        LispType type = LISP_TYPE(arg);
        if (type != LISP_INTEGER && type != LISP_NUMBER)
            return lisp_raise_error("min requires numbers");

        int unordered = 0;
        if (number_compare(arg, best, &unordered) < 0)
            best = arg;
    }

    return best;
}

/* ---- Bitwise Operations ---- */

static LispObject *builtin_logand(LispObject *args, Environment *env)
{
    (void)env;
    long long result = -1; /* Identity: all bits set */

    while (args != NIL && args != NULL) {
        LispObject *arg = lisp_car(args);
        if (LISP_TYPE(arg) != LISP_INTEGER)
            return lisp_raise_error("logand requires integers");
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
            return lisp_raise_error("logior requires integers");
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
            return lisp_raise_error("logxor requires integers");
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
        return lisp_raise_error("lognot requires an integer");

    return lisp_make_integer(~LISP_INT_VAL(arg));
}

static LispObject *builtin_ash(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("ash");

    LispObject *value_obj = lisp_car(args);
    LispObject *count_obj = lisp_car(lisp_cdr(args));

    if (LISP_TYPE(value_obj) != LISP_INTEGER)
        return lisp_raise_error("ash requires an integer value");
    if (LISP_TYPE(count_obj) != LISP_INTEGER)
        return lisp_raise_error("ash requires an integer count");

    long long value = LISP_INT_VAL(value_obj);
    long long count = LISP_INT_VAL(count_obj);

    if (count < 0) {
        /* Arithmetic right shift; by 63 or more only the sign survives */
        if (count <= -63)
            return lisp_make_integer(value < 0 ? -1 : 0);
        return lisp_make_integer(value >> (unsigned int)(-count));
    }

    if (value == 0)
        return lisp_make_integer(0);

    /* Exact left shift while the result still fits in long long */
#ifdef __SIZEOF_INT128__
    if (count < 127) {
        __int128 wide;
        if (!__builtin_mul_overflow((__int128)value, (__int128)1 << count, &wide) &&
            wide >= LLONG_MIN && wide <= LLONG_MAX)
            return lisp_make_integer((long long)wide);
    }
#else
    if (count < 63) {
        long long shifted;
        if (!__builtin_mul_overflow(value, 1LL << count, &shifted))
            return lisp_make_integer(shifted);
    }
#endif

    /* Beyond the integer range: fall back to a float */
    if (count > 1024)
        return lisp_raise_error("ash: result is not a finite number");

    double result = ldexp((double)value, (int)count);
    if (!isfinite(result))
        return lisp_raise_error("ash: result is not a finite number");
    return lisp_make_number(result);
}

static LispObject *builtin_logcount(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("logcount");

    LispObject *arg = lisp_car(args);
    if (LISP_TYPE(arg) != LISP_INTEGER)
        return lisp_raise_error("logcount requires an integer");

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
