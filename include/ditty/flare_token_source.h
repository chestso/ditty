/* flare_token_source.h - Flare token source interface
 *
 * Token sources produce tokens one at a time via pull().
 * Lexers and iterators both implement this interface, enabling
 * composition in the Flare v3.0 pipeline.
 */

#ifndef DITTY_FLARE_TOKEN_SOURCE_H
#define DITTY_FLARE_TOKEN_SOURCE_H

#include <stddef.h>

/* Forward declarations — FlareToken is defined in ditty/highlight.h */
typedef struct FlareToken FlareToken;

typedef struct FlareTokenSource FlareTokenSource;

typedef struct
{
    /* Pull next token into `out`.
     * out->text is owned by the source and valid until the source is freed.
     * Returns 1 on success, 0 on EOF, negative on error. */
    int (*pull)(FlareTokenSource *src, FlareToken *out);

    /* Get error message from last failed pull. May return NULL. */
    const char *(*error)(FlareTokenSource *src);

    /* Destroy source and all owned resources, including upstream source. */
    void (*free)(FlareTokenSource *src);
} FlareTokenSourceVTable;

struct FlareTokenSource
{
    const FlareTokenSourceVTable *vtable;
};

/* Pull helper */
int flare_token_source_pull(FlareTokenSource *src, FlareToken *out);

/* Get error message */
const char *flare_token_source_error(FlareTokenSource *src);

/* Destroy token source (cascades ownership) */
void flare_token_source_free(FlareTokenSource *src);

#endif /* DITTY_FLARE_TOKEN_SOURCE_H */
