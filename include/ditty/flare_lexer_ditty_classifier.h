/* flare_lexer_ditty_classifier.h - Shared ditty atom classifier
 *
 * Classifies a single ditty atom (symbol/identifier/literal) into a
 * FlareTokenType using the runtime Environment for dynamic lookup of
 * special forms, builtins, lambdas, and macros.
 */

#ifndef DITTY_FLARE_LEXER_DITTY_CLASSIFIER_H
#define DITTY_FLARE_LEXER_DITTY_CLASSIFIER_H

#include "../lisp.h"
#include "highlight.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Return 1 if c is a token delimiter, 0 otherwise. */
int ditty_is_delimiter(char c);

/* Classify a single atom token.
 * text: atom text (not necessarily NUL-terminated)
 * len: byte length of atom text
 * env: runtime environment for dynamic classification
 *
 * Returns the appropriate FlareTokenType. */
FlareTokenType ditty_classify_atom(const char *text, size_t len, Environment *env);

#ifdef __cplusplus
}
#endif

#endif /* DITTY_FLARE_LEXER_DITTY_CLASSIFIER_H */
