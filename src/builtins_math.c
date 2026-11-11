#include "builtins_internal.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

/* ---- Argument helpers ---- */

/* Number of elements in ARGS */
static int arg_count(LispObject *args)
{
    int count = 0;
    while (args != NIL && args != NULL) {
        count++;
        args = lisp_cdr(args);
    }
    return count;
}

/* Raise unless ARGS has exactly EXPECTED elements. */
static void check_exact_args(LispObject *args, int expected, const char *name)
{
    if (arg_count(args) == expected)
        return;

    char message[64];
    snprintf(message, sizeof(message), "%s requires %d argument%s", name, expected, expected == 1 ? "" : "s");
    lisp_raise_error(message);
}

/* Store the numeric value of OBJ in OUT. Returns 0 when OBJ is not a number. */
static int number_value(LispObject *obj, double *out)
{
    LispType type = LISP_TYPE(obj);
    if (type == LISP_INTEGER) {
        *out = (double)LISP_INT_VAL(obj);
        return 1;
    }
    if (type == LISP_NUMBER) {
        *out = LISP_NUM_VAL(obj);
        return 1;
    }
    return 0;
}

static LispObject *requires_a_number(const char *name)
{
    char message[64];
    snprintf(message, sizeof(message), "%s requires a number", name);
    return lisp_raise_error(message);
}

static LispObject *requires_numbers(const char *name)
{
    char message[64];
    snprintf(message, sizeof(message), "%s requires numbers", name);
    return lisp_raise_error(message);
}

static LispObject *not_finite(const char *name)
{
    char message[80];
    snprintf(message, sizeof(message), "%s: result is not a finite number", name);
    return lisp_raise_error(message);
}

/* Wrap a computed float result, rejecting NaN and infinities */
static LispObject *make_float_result(double v, const char *name)
{
    if (!isfinite(v))
        return not_finite(name);
    return lisp_make_number(v);
}

/* Fetch the single number argument of a one-argument builtin. */
static void one_number_arg(LispObject *args, const char *name, double *out)
{
    check_exact_args(args, 1, name);
    if (!number_value(lisp_car(args), out))
        requires_a_number(name);
}

/* ---- Absolute value ---- */

static LispObject *builtin_abs(LispObject *args, Environment *env)
{
    (void)env;
    check_exact_args(args, 1, "abs");

    LispObject *arg = lisp_car(args);
    LispType type = LISP_TYPE(arg);
    if (type == LISP_INTEGER) {
        long long v = LISP_INT_VAL(arg);
        if (v == LLONG_MIN)
            return lisp_make_number(-(double)v);
        return lisp_make_integer(v < 0 ? -v : v);
    }
    if (type == LISP_NUMBER)
        return make_float_result(fabs(LISP_NUM_VAL(arg)), "abs");
    return requires_a_number("abs");
}

/* ---- Powers and roots ---- */

static LispObject *builtin_sqrt(LispObject *args, Environment *env)
{
    (void)env;
    double x;
    one_number_arg(args, "sqrt", &x);

    if (x < 0)
        return lisp_raise_error("sqrt: argument must be non-negative");
    return make_float_result(sqrt(x), "sqrt");
}

/* ---- Trigonometry ---- */

static LispObject *builtin_sin(LispObject *args, Environment *env)
{
    (void)env;
    double x;
    one_number_arg(args, "sin", &x);
    return make_float_result(sin(x), "sin");
}

static LispObject *builtin_cos(LispObject *args, Environment *env)
{
    (void)env;
    double x;
    one_number_arg(args, "cos", &x);
    return make_float_result(cos(x), "cos");
}

static LispObject *builtin_tan(LispObject *args, Environment *env)
{
    (void)env;
    double x;
    one_number_arg(args, "tan", &x);
    return make_float_result(tan(x), "tan");
}

static LispObject *inverse_trig(LispObject *args, const char *name, double (*fn)(double))
{
    double x;
    one_number_arg(args, name, &x);

    /* Also catches NaN */
    if (!(x >= -1.0 && x <= 1.0)) {
        char message[64];
        snprintf(message, sizeof(message), "%s: argument out of range", name);
        return lisp_raise_error(message);
    }
    return make_float_result(fn(x), name);
}

static LispObject *builtin_asin(LispObject *args, Environment *env)
{
    (void)env;
    return inverse_trig(args, "asin", asin);
}

static LispObject *builtin_acos(LispObject *args, Environment *env)
{
    (void)env;
    return inverse_trig(args, "acos", acos);
}

static LispObject *builtin_atan(LispObject *args, Environment *env)
{
    (void)env;
    int count = arg_count(args);
    if (count != 1 && count != 2)
        return lisp_raise_error("atan requires 1 or 2 arguments");

    double y;
    if (!number_value(lisp_car(args), &y))
        return count == 1 ? requires_a_number("atan") : requires_numbers("atan");

    if (count == 1)
        return make_float_result(atan(y), "atan");

    double x;
    if (!number_value(lisp_car(lisp_cdr(args)), &x))
        return requires_numbers("atan");
    return make_float_result(atan2(y, x), "atan");
}

static LispObject *builtin_atan2(LispObject *args, Environment *env)
{
    (void)env;
    check_exact_args(args, 2, "atan2");

    double y;
    double x;
    if (!number_value(lisp_car(args), &y) || !number_value(lisp_car(lisp_cdr(args)), &x))
        return requires_numbers("atan2");
    return make_float_result(atan2(y, x), "atan2");
}

/* ---- Exponentials and logarithms ---- */

static LispObject *builtin_exp(LispObject *args, Environment *env)
{
    (void)env;
    double x;
    one_number_arg(args, "exp", &x);
    return make_float_result(exp(x), "exp");
}

static LispObject *builtin_log(LispObject *args, Environment *env)
{
    (void)env;
    double x;
    one_number_arg(args, "log", &x);

    if (x <= 0)
        return lisp_raise_error("log: argument must be positive");
    return make_float_result(log(x), "log");
}

/* Exact integer exponentiation by repeated squaring. Returns 0 on overflow. */
static int integer_power(long long base, long long exponent, long long *out)
{
    long long result = 1;
    long long factor = base;

    while (exponent > 0) {
        if (exponent & 1) {
            if (__builtin_mul_overflow(result, factor, &result))
                return 0;
        }
        exponent >>= 1;
        if (exponent > 0) {
            if (__builtin_mul_overflow(factor, factor, &factor))
                return 0;
        }
    }

    *out = result;
    return 1;
}

static LispObject *builtin_expt(LispObject *args, Environment *env)
{
    (void)env;
    check_exact_args(args, 2, "expt");

    LispObject *base_obj = lisp_car(args);
    LispObject *exponent_obj = lisp_car(lisp_cdr(args));

    double base;
    double exponent;
    if (!number_value(base_obj, &base) || !number_value(exponent_obj, &exponent))
        return requires_numbers("expt");

    if (LISP_TYPE(base_obj) == LISP_INTEGER && LISP_TYPE(exponent_obj) == LISP_INTEGER) {
        long long base_int = LISP_INT_VAL(base_obj);
        long long exponent_int = LISP_INT_VAL(exponent_obj);

        if (exponent_int < 0 && base_int == 0)
            return lisp_raise_error("Division by zero");

        if (exponent_int >= 0) {
            long long result;
            if (integer_power(base_int, exponent_int, &result))
                return lisp_make_integer(result);
            /* Overflow: fall back to the float calculation below */
        }
    }

    return make_float_result(pow(base, exponent), "expt");
}

/* ---- Rounding ---- */

static LispObject *builtin_float(LispObject *args, Environment *env)
{
    (void)env;
    double x;
    one_number_arg(args, "float", &x);
    return make_float_result(x, "float");
}

static LispObject *integral_math(LispObject *args, const char *name, double (*fn)(double))
{
    double x;
    one_number_arg(args, name, &x);

    /* An integer already is its own rounding */
    if (LISP_TYPE(lisp_car(args)) == LISP_INTEGER)
        return lisp_car(args);

    double result = fn(x);
    if (!isfinite(result))
        return not_finite(name);
    return make_integral_result(result);
}

static LispObject *builtin_truncate(LispObject *args, Environment *env)
{
    (void)env;
    return integral_math(args, "truncate", trunc);
}

static LispObject *builtin_ceiling(LispObject *args, Environment *env)
{
    (void)env;
    return integral_math(args, "ceiling", ceil);
}

static LispObject *builtin_round(LispObject *args, Environment *env)
{
    (void)env;
    return integral_math(args, "round", nearbyint);
}

/* ---- Pseudo-random numbers ---- */

/* xorshift64* state, seeded through splitmix64. No Lisp pointers live here,
   so plain static storage is fine. */
static uint64_t random_state = 0;

static uint64_t splitmix64_next(uint64_t *state)
{
    uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static void random_ensure_seeded(void)
{
    if (random_state == 0) {
        uint64_t seed = (uint64_t)time(NULL) ^ ((uint64_t)clock() << 32);
        random_state = splitmix64_next(&seed);
        if (random_state == 0)
            random_state = 0x9E3779B97F4A7C15ULL;
    }
}

static uint64_t random_next(void)
{
    uint64_t x = random_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    random_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static LispObject *builtin_random(LispObject *args, Environment *env)
{
    (void)env;
    check_exact_args(args, 1, "random");

    LispObject *arg = lisp_car(args);
    random_ensure_seeded();

    if (LISP_TYPE(arg) == LISP_INTEGER) {
        long long n = LISP_INT_VAL(arg);
        if (n <= 0)
            return lisp_raise_error("random: argument must be positive");

        /* Rejection sampling avoids modulo bias */
        uint64_t range = (uint64_t)n;
        uint64_t limit = UINT64_MAX - (UINT64_MAX % range);
        uint64_t r;
        do {
            r = random_next();
        } while (r >= limit);
        return lisp_make_integer((long long)(r % range));
    }

    if (LISP_TYPE(arg) == LISP_NUMBER) {
        double x = LISP_NUM_VAL(arg);
        if (!(x > 0))
            return lisp_raise_error("random: argument must be positive");
        return lisp_make_number((double)(random_next() >> 11) * (1.0 / 9007199254740992.0) * x);
    }

    return requires_a_number("random");
}

static LispObject *builtin_random_seed(LispObject *args, Environment *env)
{
    (void)env;
    check_exact_args(args, 1, "random-seed");

    LispObject *arg = lisp_car(args);
    if (LISP_TYPE(arg) != LISP_INTEGER)
        return lisp_raise_error("random-seed requires an integer");

    uint64_t seed = (uint64_t)LISP_INT_VAL(arg);
    random_state = splitmix64_next(&seed);
    if (random_state == 0)
        random_state = 0x9E3779B97F4A7C15ULL;
    return NIL;
}

void register_math_builtins(Environment *env)
{
    REGISTER("abs", builtin_abs);
    REGISTER("sqrt", builtin_sqrt);
    REGISTER("sin", builtin_sin);
    REGISTER("cos", builtin_cos);
    REGISTER("tan", builtin_tan);
    REGISTER("asin", builtin_asin);
    REGISTER("acos", builtin_acos);
    REGISTER("atan", builtin_atan);
    REGISTER("atan2", builtin_atan2);
    REGISTER("exp", builtin_exp);
    REGISTER("log", builtin_log);
    REGISTER("expt", builtin_expt);
    REGISTER("float", builtin_float);
    REGISTER("truncate", builtin_truncate);
    REGISTER("ceiling", builtin_ceiling);
    REGISTER("round", builtin_round);
    REGISTER("random", builtin_random);
    REGISTER("random-seed", builtin_random_seed);
}
