/* flare_iterator.h - Flare token stream iterators
 *
 * Iterators transform a stream of tokens lazily. They implement the
 * FlareTokenSource interface so they can be chained and consumed by
 * formatters.
 */

#ifndef DITTY_FLARE_ITERATOR_H
#define DITTY_FLARE_ITERATOR_H

#include "flare_token_source.h"
#include "flare_layout.h"
#include "ditty/highlight.h"

/* Reflow options. Width is not stored here — it lives in FlareLayout
 * so that it can be updated mid-stream (e.g., on terminal resize). */

#define FLARE_ITERATOR_REFLOW_DEFAULT ((FlareReflowOptions){ \
    .preserve_paragraphs = 1,                                \
    .preserve_code = 1,                                      \
    .preserve_headings = 1,                                  \
    .preserve_hard_breaks = 1 })

/* Create a reflow iterator wrapping another token source.
 *
 * OWNERSHIP: The reflow iterator owns `upstream`. Freeing the iterator
 * frees the upstream source (and its upstream, recursively).
 *
 * `layout` is borrowed; the caller must keep it alive for the lifetime
 * of the iterator. */
FlareTokenSource *flare_iterator_reflow(FlareTokenSource *upstream,
                                        FlareLayout *layout,
                                        const FlareReflowOptions *options);

#endif /* DITTY_FLARE_ITERATOR_H */
