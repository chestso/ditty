/* flare_lexer_ditty.c - Flare streaming Ditty Lisp lexer */

#include "ditty/flare_lexer.h"
#include "ditty/flare_lexer_ditty_classifier.h"
#include "ditty/highlight.h"
#include <gc.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define LEXER_BUFFER_SIZE 4096

/* ===== Ditty Lexer ===== */

typedef struct
{
    FlareTokenSource base;
    FlareSource *src;     /* Owned: will free on destruction */
    Environment *env;     /* Borrowed: caller owns this */
    char *buffer;         /* Reusable read buffer */
    size_t buffer_pos;    /* Current position in buffer */
    size_t buffer_len;    /* Valid bytes in buffer */
    size_t source_offset; /* Offset in source stream */
    int eof;              /* End of input reached */
} DittyLexer;

static int ditty_pull(FlareTokenSource *src, FlareToken *out)
{
    DittyLexer *lex = (DittyLexer *)src;

    if (lex->eof) {
        return 0; /* EOF */
    }

    /* Ensure we have data in buffer */
    if (lex->buffer_pos >= lex->buffer_len) {
        ssize_t n = flare_source_read(lex->src, lex->buffer, LEXER_BUFFER_SIZE);
        if (n <= 0) {
            lex->eof = 1;
            return 0; /* EOF or error */
        }
        lex->buffer_len = n;
        lex->buffer_pos = 0;
    }

    /* Find token end */
    size_t start = lex->buffer_pos;
    char c = lex->buffer[start];
    size_t end = start;
    FlareTokenType type = HL_TEXT;

    /* Whitespace */
    if (isspace((unsigned char)c)) {
        end = start + 1;
        while (end < lex->buffer_len &&
               isspace((unsigned char)lex->buffer[end])) {
            end++;
        }
        type = HL_TEXT;
    }
    /* Punctuation */
    else if (c == '(' || c == ')' || c == '[' || c == ']' ||
             c == '{' || c == '}') {
        end = start + 1;
        if (c == '(')
            type = HL_PUNCT_OPEN_PAREN;
        else if (c == ')')
            type = HL_PUNCT_CLOSE_PAREN;
        else
            type = HL_PUNCTUATION;
    }
    /* String literal */
    else if (c == '"') {
        end = start + 1;
        while (end < lex->buffer_len) {
            if (lex->buffer[end] == '\\' && end + 1 < lex->buffer_len) {
                end += 2; /* Skip escape sequence */
            } else if (lex->buffer[end] == '"') {
                end++;
                break;
            } else {
                end++;
            }
        }
        type = HL_LITERAL_STRING;
    }
    /* Comment */
    else if (c == ';') {
        end = start;
        while (end < lex->buffer_len && lex->buffer[end] != '\n') {
            end++;
        }
        type = HL_COMMENT_LINE;
    }
    /* Hash dispatch: #\char, #(, #t, #f, etc. */
    else if (c == '#') {
        end = start + 1;
        if (end < lex->buffer_len) {
            char next = lex->buffer[end];
            if (next == '\\') {
                /* Character literal: #\x ... */
                end++;
                if (end < lex->buffer_len) {
                    if (isalpha((unsigned char)lex->buffer[end])) {
                        end++;
                        while (end < lex->buffer_len &&
                               !ditty_is_delimiter(lex->buffer[end]) &&
                               lex->buffer[end] != '(' &&
                               lex->buffer[end] != ')') {
                            end++;
                        }
                    } else {
                        end++;
                    }
                }
                type = HL_LITERAL_CHARACTER;
            } else if (next == '(') {
                end++;
                type = HL_PUNCT_HASH;
            } else if (next == 't') {
                end = start + 2;
                type = HL_LITERAL_BOOLEAN;
            } else if (next == 'f') {
                end = start + 2;
                type = HL_LITERAL_NIL;
            } else {
                /* Unknown # sequence, treat as single punctuation */
                end = start + 1;
                type = HL_PUNCT_HASH;
            }
        } else {
            end = start + 1;
            type = HL_PUNCT_HASH;
        }
    }
    /* Quote / backtick / unquote operators */
    else if (c == '\'' || c == '`' || c == ',') {
        end = start + 1;
        if (c == '\'')
            type = HL_OPERATOR_QUOTE;
        else if (c == '`')
            type = HL_OPERATOR_BACKQUOTE;
        else if (end < lex->buffer_len && lex->buffer[end] == '@') {
            end++;
            type = HL_OPERATOR_UNQUOTE_SPLICING;
        } else {
            type = HL_OPERATOR_UNQUOTE;
        }
    }
    /* Number */
    else if (isdigit((unsigned char)c) ||
             ((c == '-' || c == '+') &&
              start + 1 < lex->buffer_len &&
              isdigit((unsigned char)lex->buffer[start + 1]))) {
        end = start + 1;
        while (end < lex->buffer_len &&
               (isdigit((unsigned char)lex->buffer[end]) ||
                lex->buffer[end] == '.')) {
            end++;
        }
        type = HL_LITERAL_NUMBER;
    }
    /* Atom / symbol / identifier */
    else {
        end = start + 1;
        while (end < lex->buffer_len &&
               !isspace((unsigned char)lex->buffer[end]) &&
               !ditty_is_delimiter(lex->buffer[end])) {
            end++;
        }
        size_t len = end - start;
        type = ditty_classify_atom(lex->buffer + start, len, lex->env);
    }

    /* Allocate token text */
    size_t len = end - start;
    char *text = GC_MALLOC_ATOMIC(len + 1);
    if (!text) {
        return -1; /* Error */
    }
    memcpy(text, lex->buffer + start, len);
    text[len] = '\0';

    /* Update state */
    lex->buffer_pos = end;
    lex->source_offset = end;

    /* Fill output token */
    out->type = type;
    out->text = text;
    out->length = len;

    return 1; /* Success */
}

static void ditty_free(FlareTokenSource *src)
{
    DittyLexer *lex = (DittyLexer *)src;
    if (lex->src) {
        flare_source_free(lex->src);
    }
    if (lex->buffer) {
        free(lex->buffer);
    }
    free(lex);
}

static const FlareTokenSourceVTable ditty_vtable = {
    .pull = ditty_pull,
    .error = NULL,
    .free = ditty_free,
};

FlareTokenSource *flare_lexer_ditty(FlareSource *src, Environment *env)
{
    if (!src) {
        return NULL;
    }

    DittyLexer *lex = calloc(1, sizeof(DittyLexer));
    if (!lex) {
        flare_source_free(src);
        return NULL;
    }

    lex->base.vtable = &ditty_vtable;
    lex->src = src; /* Take ownership */
    lex->env = env;
    lex->buffer = malloc(LEXER_BUFFER_SIZE);
    if (!lex->buffer) {
        flare_source_free(src);
        free(lex);
        return NULL;
    }
    lex->buffer_pos = 0;
    lex->buffer_len = 0;
    lex->source_offset = 0;
    lex->eof = 0;

    return &lex->base;
}
