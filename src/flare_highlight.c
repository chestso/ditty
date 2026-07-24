/* highlight.c - One-shot flare_highlight() convenience */

#include "../include/ditty/highlight.h"
#include "../include/lisp.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations from other source files */
extern FlareColorDepth flare_formatter_depth(const FlareFormatter *f);

FlareResult flare_highlight(const char *input, size_t input_len,
                            const FlareStyle *style,
                            const FlareFormatter *formatter)
{
    FlareResult result = { NULL, 0 };

    if (!input)
        input = "";
    if (input_len == 0 && input[0] != '\0')
        input_len = 0;

    Environment *def_env = NULL;
    FlareSource *def_src = NULL;
    FlareTokenSource *src = NULL;

    /* Resolve defaults */
    FlareStyle *def_style = NULL;
    if (!style) {
        def_style = flare_style_dracula();
        style = def_style;
    }

    FlareFormatter *def_fmt = NULL;
    if (!formatter) {
        def_fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR, NULL, NULL);
        formatter = def_fmt;
    }

    /* Lex */
    def_env = lisp_init();
    if (!def_env)
        goto cleanup;
    def_src = flare_source_string(input,
                                  input_len > 0 ? input_len : strlen(input),
                                  0);
    if (!def_src)
        goto cleanup;
    src = flare_lexer_ditty(def_src, def_env);
    if (!src)
        goto cleanup;

    /* Pull tokens into an array */
    size_t token_count = 0;
    size_t token_capacity = 64;
    FlareToken *tokens = malloc(token_capacity * sizeof(FlareToken));
    if (!tokens)
        goto cleanup;

    FlareToken tok;
    while (flare_token_source_pull(src, &tok)) {
        if (token_count >= token_capacity) {
            token_capacity *= 2;
            FlareToken *tmp = realloc(tokens, token_capacity * sizeof(FlareToken));
            if (!tmp) {
                free(tokens);
                goto cleanup;
            }
            tokens = tmp;
        }
        tokens[token_count++] = tok;
    }

    /* Format */
    FlareColorDepth depth = flare_formatter_depth(formatter);
    char *ansi = flare_format_terminal(tokens, token_count, style, depth,
                                       flare_terminal_supports_hyperlinks());
    free(tokens);

cleanup:
    if (src)
        flare_token_source_free(src);
    else if (def_src)
        flare_source_free(def_src);
    if (def_env)
        lisp_cleanup();
    if (def_style)
        flare_style_free(def_style);
    if (def_fmt)
        flare_formatter_free(def_fmt);

    result.data = ansi;
    result.length = ansi ? strlen(ansi) : 0;
    return result;
}

void flare_result_free(FlareResult result)
{
    free(result.data);
}
