/* flare_lexer.h - Flare streaming lexer API
 *
 * Streaming lexers implement the FlareTokenSource interface.
 * They read from FlareSource incrementally and produce tokens one at a time.
 */

#ifndef DITTY_FLARE_LEXER_H
#define DITTY_FLARE_LEXER_H

#include "flare_token_source.h"
#include "flare_source.h"
#include "lisp.h"

/* Create a streaming ditty lexer that reads from src.
 * Returns a FlareTokenSource that owns src (will free it when freed).
 * Returns NULL on allocation failure. */
FlareTokenSource *flare_lexer_ditty(FlareSource *src, Environment *env);

/* Create a streaming commonmark lexer that reads from src.
 * Returns a FlareTokenSource that owns src (will free it when freed).
 * Returns NULL on allocation failure. */
FlareTokenSource *flare_lexer_commonmark(FlareSource *src, Environment *env);

#endif /* DITTY_FLARE_LEXER_H */
