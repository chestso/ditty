/* flare_token_source.c - FlareTokenSource helper functions
 *
 * Token sources produce tokens one at a time via pull().
 */

#include "../include/ditty/flare_token_source.h"

int flare_token_source_pull(FlareTokenSource *src, FlareToken *out)
{
    if (!src || !src->vtable || !src->vtable->pull)
        return 0;
    return src->vtable->pull(src, out);
}

const char *flare_token_source_error(FlareTokenSource *src)
{
    if (!src || !src->vtable || !src->vtable->error)
        return NULL;
    return src->vtable->error(src);
}

void flare_token_source_free(FlareTokenSource *src)
{
    if (!src)
        return;
    if (src->vtable && src->vtable->free)
        src->vtable->free(src);
}
