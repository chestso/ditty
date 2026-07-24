/* test_flare_lexer_ditty.c - TDD tests for streaming lexer */

#include "flare_testkit.h"
#include "ditty/highlight.h"
#include "ditty/flare_source.h"
#include "ditty/flare_token_source.h"
#include "ditty/highlight.h"
#include "lisp.h"
#include <string.h>

static Environment *g_env;

static void setup(void)
{
    g_env = lisp_init();
    ASSERT_NOT_NULL(g_env);
}

static void teardown(void)
{
    lisp_cleanup();
    g_env = NULL;
}

/* Token source for the lexer */
static FlareTokenSource *lex_string(const char *text)
{
    FlareSource *src = flare_source_string(text, strlen(text), 0);
    ASSERT_NOT_NULL(src);
    return flare_lexer_ditty(src, g_env);
}

static void test_empty(void)
{
    setup();
    FlareTokenSource *ts = lex_string("");
    FlareToken tok;
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_whitespace_preserved(void)
{
    setup();
    FlareTokenSource *ts = lex_string("  ");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 2);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_numbers_and_builtins(void)
{
    setup();
    FlareTokenSource *ts = lex_string("(+ 1 2)");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_PUNCT_OPEN_PAREN);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_NAME_BUILTIN);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "+", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_LITERAL_NUMBER);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_LITERAL_NUMBER);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_PUNCT_CLOSE_PAREN);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_define_classifies_keyword(void)
{
    setup();
    FlareTokenSource *ts = lex_string("(define x 1)");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_PUNCT_OPEN_PAREN);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_KEYWORD_DEFINE);
    ASSERT_EQ(tok.length, 6);
    ASSERT_TRUE(memcmp(tok.text, "define", 6) == 0);

    flare_token_source_free(ts);
    teardown();
}

static void test_string_literal(void)
{
    setup();
    FlareTokenSource *ts = lex_string("\"hello world\"");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_LITERAL_STRING);
    ASSERT_EQ(tok.length, 13);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_comment(void)
{
    setup();
    FlareTokenSource *ts = lex_string("; comment\n");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_COMMENT_LINE);
    ASSERT_EQ(tok.length, 9);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

int main(void)
{
    RUN_TEST(test_empty);
    RUN_TEST(test_whitespace_preserved);
    RUN_TEST(test_numbers_and_builtins);
    RUN_TEST(test_define_classifies_keyword);
    RUN_TEST(test_string_literal);
    RUN_TEST(test_comment);

    TEST_SUMMARY();
    return 0;
}
