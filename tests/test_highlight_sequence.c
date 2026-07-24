/* test_highlight_sequence.c - Test highlight callback sequence */
#include "flare_testkit.h"
#include "../include/lisp.h"
#include "../include/ditty/highlight.h"
#include "../include/ditty/flare_source.h"
#include "../include/ditty/flare_token_source.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Simulate the REPL highlight callback - direct token collection */
static char *simulate_repl_highlight(const char *text, size_t len, Environment *env)
{
    if (!text || len == 0) {
        char *empty = malloc(1);
        if (empty)
            empty[0] = '\0';
        return empty;
    }

    /* Create a fresh style for each call */
    FlareStyle *style = flare_style_dracula();
    if (!style)
        goto fallback;

    /* Collect tokens */
    FlareSource *source = flare_source_string(text, len, 0);
    if (!source) {
        flare_style_free(style);
        goto fallback;
    }

    FlareTokenSource *lexer = flare_lexer_ditty(source, env);
    if (!lexer) {
        flare_source_free(source);
        flare_style_free(style);
        goto fallback;
    }

    size_t count = 0;
    size_t capacity = 64;
    FlareToken *tokens = malloc(capacity * sizeof(FlareToken));
    if (!tokens) {
        flare_token_source_free(lexer);
        flare_style_free(style);
        goto fallback;
    }

    FlareToken tok;
    int result;
    while ((result = flare_token_source_pull(lexer, &tok)) > 0) {
        if (count >= capacity) {
            capacity *= 2;
            FlareToken *tmp = realloc(tokens, capacity * sizeof(FlareToken));
            if (!tmp) {
                free(tokens);
                flare_token_source_free(lexer);
                flare_style_free(style);
                goto fallback;
            }
            tokens = tmp;
        }
        tokens[count++] = tok;
    }

    flare_token_source_free(lexer);

    if (result < 0) {
        free(tokens);
        flare_style_free(style);
        goto fallback;
    }

    /* Format tokens */
    char *ansi = flare_format_terminal(tokens, count, style, BFLARE_COLOR_TRUECOLOR, 0);

    free(tokens);
    flare_style_free(style);

    if (ansi)
        return ansi;

fallback:
{
    char *fallback = malloc(len + 1);
    if (fallback) {
        memcpy(fallback, text, len);
        fallback[len] = '\0';
    }
    return fallback;
}
}

static void test_highlight_foo_sequence(void)
{
    Environment *env = lisp_init();
    ASSERT_NOT_NULL(env);

    /* Sequence: "foo" -> "foo " -> "foo" (backspace) */
    char *result;

    printf("\n=== Step 1: 'foo' ===\n");
    result = simulate_repl_highlight("foo", 3, env);
    ASSERT_NOT_NULL(result);
    printf("Result: '%s'\n", result);
    free(result);

    printf("\n=== Step 2: 'foo ' ===\n");
    result = simulate_repl_highlight("foo ", 4, env);
    ASSERT_NOT_NULL(result);
    printf("Result: '%s'\n", result);
    free(result);

    printf("\n=== Step 3: 'foo' (after backspace) ===\n");
    result = simulate_repl_highlight("foo", 3, env);
    ASSERT_NOT_NULL(result);
    printf("Result: '%s'\n", result);
    free(result);

    lisp_cleanup();
}

static void test_highlight_quoted_foo_sequence(void)
{
    Environment *env = lisp_init();
    ASSERT_NOT_NULL(env);

    /* Test that backspace sequence works correctly */
    char *result;

    printf("\n=== Step 1: '\\\"foo\\\"' ===\n");
    result = simulate_repl_highlight("\"foo\"", 5, env);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(strlen(result), 30);
    printf("Result strlen: %zu (correct!)\n", strlen(result));
    free(result);

    printf("\n=== Step 2: '\\\"foo\\\" ' ===\n");
    result = simulate_repl_highlight("\"foo\" ", 6, env);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(strlen(result), 52);
    printf("Result strlen: %zu (correct!)\n", strlen(result));
    free(result);

    printf("\n=== Step 3: '\\\"foo\\\"' (after backspace) ===\n");
    result = simulate_repl_highlight("\"foo\"", 5, env);
    ASSERT_NOT_NULL(result);
    /* This was the bug: strlen was 52 instead of 30 because the writer wasn't null-terminating */
    ASSERT_EQ(strlen(result), 30);
    printf("Result strlen: %zu (correct!)\n", strlen(result));
    free(result);

    lisp_cleanup();
}

int main(void)
{
    RUN_TEST(test_highlight_foo_sequence);
    RUN_TEST(test_highlight_quoted_foo_sequence);
    TEST_SUMMARY();
    return 0;
}
