/* flare_lexer_ditty_classifier.c - Shared ditty atom classifier
 *
 * Provides runtime-based classification for ditty atoms. Used by both the
 * v1 push-based lexer and the v3 streaming lexer.
 */

#include "ditty/flare_lexer_ditty_classifier.h"
#include "ditty/highlight.h"
#include "lisp.h"
#include <ctype.h>
#include <string.h>

int ditty_is_delimiter(char c)
{
    return c == '\0' || isspace((unsigned char)c) ||
           c == '(' || c == ')' || c == ';' ||
           c == '"' || c == '\'' || c == '`' || c == ',';
}

/* Map ditty's SfKind to ditty-flare's token subcategory */
static FlareTokenType sf_kind_to_type(SfKind kind)
{
    switch (kind) {
    case SF_KIND_DEFINE:
        return HL_KEYWORD_DEFINE;
    case SF_KIND_CONTROL:
        return HL_KEYWORD_CONTROL;
    case SF_KIND_MACRO_DEF:
        return HL_KEYWORD_MACRO_DEF;
    default:
        return HL_KEYWORD_SPECIAL_FORM;
    }
}

FlareTokenType ditty_classify_atom(const char *text, size_t len, Environment *env)
{
    if (len == 0 || !text)
        return HL_TEXT;

    /* Keyword argument */
    if (text[0] == ':' && len > 1)
        return HL_NAME_KEYWORD_ARG;

    /* Boolean / nil literals */
    if (len == 3 && strncmp(text, "nil", 3) == 0)
        return HL_LITERAL_NIL;
    if (len == 2 && strncmp(text, "#f", 2) == 0)
        return HL_LITERAL_NIL;
    if (len == 2 && strncmp(text, "#t", 2) == 0)
        return HL_LITERAL_BOOLEAN;

    /* Number check */
    {
        const char *p = text;
        int is_num = 1, has_dot = 0;
        size_t remain = len;

        if (*p == '-' || *p == '+') {
            p++;
            remain--;
        }
        if (remain == 0)
            is_num = 0;
        while (remain > 0 && is_num) {
            if (*p == '.') {
                if (has_dot) {
                    is_num = 0;
                    break;
                }
                has_dot = 1;
            } else if (!isdigit((unsigned char)*p)) {
                is_num = 0;
            }
            p++;
            remain--;
        }
        if (is_num && len > 0)
            return HL_LITERAL_NUMBER;
    }

    /* Symbol classification via runtime. Make a NUL-terminated copy on
     * the stack for the intern helpers. */
    char buf[256];
    if (len >= sizeof(buf))
        return HL_NAME_VARIABLE;

    memcpy(buf, text, len);
    buf[len] = '\0';

    char *colon = strchr(buf, ':');
    LispObject *sym;
    if (colon && colon != buf && colon[1] != '\0') {
        *colon = '\0';
        sym = lisp_intern_qualified(buf, colon + 1);
    } else {
        sym = lisp_intern(buf);
    }

    /* Special forms */
    int kind = (int)lisp_sf_kind(sym);
    if (kind >= 0)
        return sf_kind_to_type(kind);

    /* Environment lookup: builtin, function, macro, or variable. */
    Symbol *symval = LISP_SYM_VAL(sym);
    LispObject *val = env_lookup(env, symval);
    if (val == NULL && symval->namespace != NULL) {
        val = env_lookup_in_package(env,
                                    LISP_SYM_VAL(lisp_intern(symval->name)),
                                    LISP_SYM_VAL(lisp_intern(symval->namespace)));
    }
    if (val) {
        LispType t = LISP_TYPE(val);
        if (t == LISP_BUILTIN)
            return HL_NAME_BUILTIN;
        if (t == LISP_LAMBDA || t == LISP_MACRO)
            return HL_NAME_FUNCTION;
    }

    return HL_NAME_VARIABLE;
}
