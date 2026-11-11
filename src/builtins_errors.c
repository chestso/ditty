#include "builtins_internal.h"

/* error? - Test if object is an error */
static LispObject *builtin_error_question(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_make_boolean(0);
    }
    LispObject *obj = lisp_car(args);
    return lisp_make_boolean(LISP_TYPE(obj) == LISP_ERROR);
}

/* error-type - Get error type symbol */
static LispObject *builtin_error_type(LispObject *args, Environment *env)
{
    if (args == NIL) {
        return lisp_raise_typed("wrong-number-of-arguments", "error-type requires 1 argument");
    }
    LispObject *obj = lisp_car(args);
    if (LISP_TYPE(obj) != LISP_ERROR) {
        return lisp_raise_typed("wrong-type-argument", "error-type: argument must be an error");
    }
    /* Return error type (guaranteed to be a symbol) */
    if (LISP_ERROR_TYPE(obj) != NULL) {
        return LISP_ERROR_TYPE(obj);
    }
    /* Fallback to 'error symbol if somehow NULL */
    return sym_error;
}

/* error-message - Get error message string */
static LispObject *builtin_error_message(LispObject *args, Environment *env)
{
    if (args == NIL) {
        return lisp_raise_typed("wrong-number-of-arguments", "error-message requires 1 argument");
    }
    LispObject *obj = lisp_car(args);
    if (LISP_TYPE(obj) != LISP_ERROR) {
        return lisp_raise_typed("wrong-type-argument", "error-message: argument must be an error");
    }
    return lisp_make_string(LISP_ERROR_MESSAGE(obj));
}

/* error-stack - Get error stack trace */
static LispObject *builtin_error_stack(LispObject *args, Environment *env)
{
    if (args == NIL) {
        return lisp_raise_typed("wrong-number-of-arguments", "error-stack requires 1 argument");
    }
    LispObject *obj = lisp_car(args);
    if (LISP_TYPE(obj) != LISP_ERROR) {
        return lisp_raise_typed("wrong-type-argument", "error-stack: argument must be an error");
    }
    LispObject *stack = LISP_ERROR_STACK_TRACE(obj);
    return (stack != NULL) ? stack : NIL;
}

/* error-data - Get error data payload */
static LispObject *builtin_error_data(LispObject *args, Environment *env)
{
    if (args == NIL) {
        return lisp_raise_typed("wrong-number-of-arguments", "error-data requires 1 argument");
    }
    LispObject *obj = lisp_car(args);
    if (LISP_TYPE(obj) != LISP_ERROR) {
        return lisp_raise_typed("wrong-type-argument", "error-data: argument must be an error");
    }
    LispObject *data = LISP_ERROR_DATA(obj);
    return (data != NULL) ? data : NIL;
}

/* signal - Raise a typed error, or re-raise an error object
 * (signal ERROR-SYMBOL DATA)
 * (signal ERROR-OBJECT)
 */
static LispObject *builtin_signal(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_raise_typed("wrong-number-of-arguments", "signal requires at least 1 argument");
    }

    LispObject *error_type = lisp_car(args);

    /* (signal ERR) re-raises the very same object — same type, message and
     * original stack. It is the one way to re-signal from a handler. */
    if (LISP_TYPE(error_type) == LISP_ERROR)
        lisp_raise(error_type);

    if (LISP_TYPE(error_type) != LISP_SYMBOL) {
        return lisp_raise_typed("wrong-type-argument", "signal: first argument must be a symbol or an error");
    }

    /* Get optional data argument */
    LispObject *rest = lisp_cdr(args);
    LispObject *data = (rest != NIL && rest != NULL) ? lisp_car(rest) : NIL;

    /* Build error message from data */
    char message[512];
    if (data != NIL && LISP_TYPE(data) == LISP_STRING) {
        snprintf(message, sizeof(message), "%s", LISP_STR_VAL(data));
    } else if (data != NIL) {
        char *data_str = lisp_print(data);
        snprintf(message, sizeof(message), "%s: %s", LISP_SYM_VAL(error_type)->name, data_str);
    } else {
        snprintf(message, sizeof(message), "%s", LISP_SYM_VAL(error_type)->name);
    }

    return lisp_raise_typed_data(error_type, message, data);
}

/* error - Convenience function to signal generic error
 * (error MESSAGE)
 */
static LispObject *builtin_error(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_raise_typed("wrong-number-of-arguments", "error requires 1 argument");
    }

    LispObject *message_obj = lisp_car(args);
    const char *message;

    if (LISP_TYPE(message_obj) == LISP_STRING) {
        message = LISP_STR_VAL(message_obj);
    } else {
        message = lisp_print(message_obj);
    }

    return lisp_raise_typed_data(sym_error, message, NIL);
}

void register_errors_builtins(Environment *env)
{
    REGISTER("error?", builtin_error_question);
    REGISTER("error-type", builtin_error_type);
    REGISTER("error-message", builtin_error_message);
    REGISTER("error-stack", builtin_error_stack);
    REGISTER("error-data", builtin_error_data);
    REGISTER("signal", builtin_signal);
    REGISTER("error", builtin_error);
}
