/* condition.c
 *
 * Frames, signaling and the protected C entry points. See
 * condition_internal.h for the frame model and include/lisp.h for the
 * public raise/try API.
 */

#include "condition_internal.h"
#include <stdio.h>
#include <stdlib.h>

EvalFrame *g_eval_frame = NULL;

void frame_unwind(EvalFrame *f)
{
    assert(g_eval_frame == f);
    g_eval_frame = f->parent;
    while (g_call_stack != f->call_stack && g_call_stack != NULL)
        pop_call_frame();
    assert(g_call_stack == f->call_stack);
}

/* Capture the call stack of the signal point, once. A re-raised error keeps
 * the trace it already has (that is why there is no "_with_stack"
 * constructor: every error gets the stack of where it was first raised). */
static void capture_trace_if_missing(LispObject *error)
{
    if (LISP_ERROR_STACK_TRACE(error) == NIL || LISP_ERROR_STACK_TRACE(error) == NULL)
        LISP_ERROR_STACK_TRACE(error) = capture_call_stack();
}

/* C code ran Lisp through the raising API with no frame in place: that is a
 * bug in the embedder, not a Lisp error. Say so and die — never return, a
 * silent return here would resume a computation that has already failed. */
static _Noreturn void fatal_unframed_signal(LispObject *error)
{
    const char *type = "error";
    const char *message = "(unknown)";

    if (error != NULL && LISP_TYPE(error) == LISP_ERROR) {
        LispObject *type_obj = LISP_ERROR_TYPE(error);
        if (type_obj != NULL && LISP_TYPE(type_obj) == LISP_SYMBOL && LISP_SYM_VAL(type_obj)->name != NULL)
            type = LISP_SYM_VAL(type_obj)->name;
        if (LISP_ERROR_MESSAGE(error) != NULL)
            message = LISP_ERROR_MESSAGE(error);
    }

    fprintf(stderr,
            "ditty: internal error: Lisp signal raised with no active eval frame\n"
            "ditty:   [%s] %s\n"
            "ditty:   run Lisp from C through lisp_try_* instead of lisp_eval()\n",
            type, message);
    abort();
}

_Noreturn LispObject *lisp_raise(LispObject *error)
{
    EvalFrame *f = g_eval_frame;

    if (error == NULL || LISP_TYPE(error) != LISP_ERROR)
        error = lisp_make_typed_error(sym_simple_error, "attempt to signal a non-error object", NIL);

    if (f == NULL)
        fatal_unframed_signal(error);

    capture_trace_if_missing(error);
    f->error = error;
    EVAL_LONGJMP(f->buf, 1);
}

_Noreturn LispObject *lisp_raise_error(const char *message)
{
    lisp_raise(lisp_make_typed_error(sym_simple_error, message, NIL));
}

_Noreturn LispObject *lisp_raise_typed(LispObject *type, const char *message)
{
    lisp_raise(lisp_make_typed_error(type, message, NIL));
}

_Noreturn LispObject *lisp_raise_typed_data(LispObject *type, const char *message, LispObject *data)
{
    lisp_raise(lisp_make_typed_error(type, message, data));
}

/* ---- Error-type hierarchy ----
 *
 * A static ancestor bitmask per registered type, built once from the
 * ERROR_TYPES X-macro. Matching is pointer equality + a bit test — no string
 * compares, no per-signal allocation. Parents precede children in the macro,
 * so a single forward pass computes every mask. */
enum
{
#define ERR_INDEX(c_name, lisp_name, parent) ERR_IDX_##c_name,
    ERROR_TYPES(ERR_INDEX)
#undef ERR_INDEX
    ERROR_TYPE_COUNT
};

static const int err_parent[ERROR_TYPE_COUNT] = {
#define ERR_PARENT(c_name, lisp_name, parent) ERR_IDX_##parent,
    ERROR_TYPES(ERR_PARENT)
#undef ERR_PARENT
};

static struct
{
    LispObject *sym;
    uint32_t mask; /* self bit | ancestors' bits */
} err_types[ERROR_TYPE_COUNT];

void condition_init_types(void)
{
    uint32_t masks[ERROR_TYPE_COUNT];
    for (int i = 0; i < ERROR_TYPE_COUNT; i++)
        masks[i] = (1u << i) | (err_parent[i] == i ? 0 : masks[err_parent[i]]);
#define ERR_SET(c_name, lisp_name, parent)    \
    err_types[ERR_IDX_##c_name].sym = c_name; \
    err_types[ERR_IDX_##c_name].mask = masks[ERR_IDX_##c_name];
    ERROR_TYPES(ERR_SET)
#undef ERR_SET
}

static uint32_t error_type_mask(LispObject *type)
{
    for (int i = 0; i < ERROR_TYPE_COUNT; i++)
        if (err_types[i].sym == type)
            return err_types[i].mask;
    return 0;
}

static uint32_t error_type_bit(LispObject *type)
{
    for (int i = 0; i < ERROR_TYPE_COUNT; i++)
        if (err_types[i].sym == type)
            return 1u << i;
    return 0;
}

/* Error types are package-agnostic: a qualified symbol (core:error) matches
 * the same type as its unqualified name. Normalize qualified symbols to the
 * canonical interned symbol for matching only — the error keeps its original
 * type object. Cold path (once per signal), so the intern lookup is free. */
static LispObject *canonical_type(LispObject *type)
{
    if (type != NULL && LISP_TYPE(type) == LISP_SYMBOL && LISP_SYM_VAL(type)->namespace != NULL)
        return lisp_intern(LISP_SYM_VAL(type)->name);
    return type;
}

/* The only handler matcher. Clauses are tried in order; the first whose type
 * is the signal's type or one of its ancestors wins (CL/Elisp semantics).
 * `error` is the root, so it catches every registered type; an unregistered
 * signal type is treated as a direct child of `error`, and an unregistered
 * handler name matches only that exact type. */
LispObject *condition_find_handler(LispObject *handlers, LispObject *error)
{
    LispObject *error_type = canonical_type(LISP_ERROR_TYPE(error));
    uint32_t mask = error_type_mask(error_type);
    if (mask == 0)
        mask = 1u; /* unregistered type: direct child of error (root bit) */

    for (LispObject *h = handlers; h != NIL && LISP_TYPE(h) == LISP_CONS; h = lisp_cdr(h)) {
        LispObject *handler = lisp_car(h);
        if (handler == NIL || LISP_TYPE(handler) != LISP_CONS)
            continue;
        LispObject *handler_type = canonical_type(lisp_car(handler));
        if (handler_type == NULL || LISP_TYPE(handler_type) != LISP_SYMBOL)
            continue;

        if (handler_type == error_type || (mask & error_type_bit(handler_type)))
            return handler;
    }

    return NIL;
}

/* ---- Protected API ---- */

LispStatus lisp_try(LispThunk fn, void *data, LispObject **out)
{
    EvalFrame frame;

    frame_push(&frame);

    if (EVAL_SETJMP(frame.buf) == 0) {
        LispObject *result = fn(data);
        frame_pop(&frame);
        if (out != NULL)
            *out = result;
        return LISP_OK;
    }

    frame_unwind(&frame);
    if (out != NULL)
        *out = frame.error;
    return LISP_SIGNALED;
}

typedef struct
{
    LispObject *expr;
    Environment *env;
} EvalThunk;

static LispObject *eval_thunk(void *data)
{
    EvalThunk *t = data;
    return lisp_eval(t->expr, t->env);
}

LispStatus lisp_try_eval(LispObject *expr, Environment *env, LispObject **out)
{
    EvalThunk t = { expr, env };
    return lisp_try(eval_thunk, &t, out);
}

typedef struct
{
    LispObject *func;
    LispObject *args;
    Environment *env;
} ApplyThunk;

static LispObject *apply_thunk(void *data)
{
    ApplyThunk *t = data;
    return lisp_apply(t->func, t->args, t->env);
}

LispStatus lisp_try_apply(LispObject *func, LispObject *args, Environment *env, LispObject **out)
{
    ApplyThunk t = { func, args, env };
    return lisp_try(apply_thunk, &t, out);
}

typedef struct
{
    const char *filename;
    Environment *env;
} LoadThunk;

static LispObject *load_thunk(void *data)
{
    LoadThunk *t = data;
    return lisp_load_file(t->filename, t->env);
}

LispStatus lisp_try_load_file(const char *filename, Environment *env, LispObject **out)
{
    LoadThunk t = { filename, env };
    return lisp_try(load_thunk, &t, out);
}

LispStatus lisp_try_eval_string(const char *code, Environment *env, LispObject **out)
{
    const char *input = code;
    LispObject *expr;

    if (env == NULL)
        env = lisp_init();

    expr = lisp_read(&input);

    if (expr == NULL) {
        if (out != NULL)
            *out = NIL;
        return LISP_OK;
    }

    /* The reader returns parse failures as values; at this boundary a parse
     * failure is a signal like any other. */
    if (LISP_TYPE(expr) == LISP_ERROR) {
        if (out != NULL)
            *out = expr;
        return LISP_SIGNALED;
    }

    return lisp_try_eval(expr, env, out);
}
