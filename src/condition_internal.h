/* condition_internal.h
 *
 * The unwinding machinery behind `condition-case`, `unwind-protect` and the
 * lisp_try_* API. A signal is a non-local control transfer: lisp_raise()
 * longjmps to the innermost EvalFrame, and the code that follows that
 * frame's setjmp decides what happens next.
 *
 * A frame carries nothing but:
 *   - the jump target,
 *   - its parent (so pop/unwind is LIFO),
 *   - the call-stack snapshot to restore on a non-local exit,
 *   - the error slot written by lisp_raise just before the jump.
 *
 * There is no kind tag and no global "pending error": what a frame does is
 * the code after its setjmp, and the error lives on that frame (on the C
 * stack, where Boehm scans it).
 */

#ifndef CONDITION_INTERNAL_H
#define CONDITION_INTERNAL_H

#include "../include/lisp.h"
#include <assert.h>
#include <setjmp.h>

typedef struct EvalFrame
{
    jmp_buf buf;
    struct EvalFrame *parent;
    CallStackFrame *call_stack; /* g_call_stack when pushed; restored on catch */
    LispObject *error;          /* written by lisp_raise just before the jump */
} EvalFrame;

extern EvalFrame *g_eval_frame;      /* innermost live frame, NULL when none */
extern CallStackFrame *g_call_stack; /* innermost live call frame (env.c) */

/* Pop the call frames an abandoned evaluation left behind and detach the
 * frame. Catch path only — call it before running handlers or cleanups. */
void frame_unwind(EvalFrame *f);

/* The one condition-case handler matcher (condition.c). Returns the matched
 * handler form `(TYPE BODY...)`, or NIL when nothing matches. */
LispObject *condition_find_handler(LispObject *handlers, LispObject *error);

static inline void frame_push(EvalFrame *f)
{
    f->parent = g_eval_frame;
    f->call_stack = g_call_stack;
    f->error = NULL;
    g_eval_frame = f;
}

/* Normal exit. The assertions make a leaked frame or an unbalanced call
 * stack abort immediately (the default build is ASan+UBSan). */
static inline void frame_pop(EvalFrame *f)
{
    assert(g_eval_frame == f);
    assert(g_call_stack == f->call_stack);
    g_eval_frame = f->parent;
}

/* _setjmp/_longjmp skip the signal-mask dance that plain setjmp/longjmp do
 * on POSIX. Windows (UCRT64/MinGW) has no underscore variants. */
#ifdef _WIN32
#define EVAL_SETJMP(b)     setjmp(b)
#define EVAL_LONGJMP(b, v) longjmp(b, v)
#else
#define EVAL_SETJMP(b)     _setjmp(b)
#define EVAL_LONGJMP(b, v) _longjmp(b, v)
#endif

/* Evaluate BODY into RESULT, then run CLEANUP — on the normal path and on
 * the non-local path (after which the original error is re-raised).
 *
 * CLEANUP must not raise: it is what keeps the C world consistent, and a
 * raise from inside it would replace the pending error.
 *
 * Must be expanded in the function that owns the frame (setjmp cannot
 * return into a helper). If -Wclobbered fires on RESULT, declare it
 * volatile in the caller.
 *
 * Rule for builtins: anything that holds a FILE *, a malloc'd buffer, a
 * PCRE2 handle or dynamic state across a call that can reach Lisp
 * (lisp_eval, lisp_apply, lisp_call_*, lisp_load_file) must use this, or an
 * eager raise will leak it. */
#define LISP_UNWIND_PROTECT(RESULT, BODY, CLEANUP) \
    do {                                           \
        EvalFrame _uf;                             \
        frame_push(&_uf);                          \
        if (EVAL_SETJMP(_uf.buf) == 0) {           \
            (RESULT) = (BODY);                     \
            frame_pop(&_uf);                       \
            CLEANUP;                               \
        } else {                                   \
            frame_unwind(&_uf);                    \
            CLEANUP;                               \
            lisp_raise(_uf.error);                 \
        }                                          \
    } while (0)

#endif /* CONDITION_INTERNAL_H */
