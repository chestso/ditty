/* test_flare_lexer_commonmark.c - TDD tests for CommonMark streaming lexer */

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

static FlareTokenSource *lex_string(const char *text)
{
    FlareSource *src = flare_source_string(text, strlen(text), 0);
    ASSERT_NOT_NULL(src);
    return flare_lexer_commonmark(src, g_env);
}

static void test_cm_empty(void)
{
    setup();
    FlareTokenSource *ts = lex_string("");
    FlareToken tok;
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_paragraph(void)
{
    setup();
    FlareTokenSource *ts = lex_string("hello world");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_PARAGRAPH);
    ASSERT_EQ(tok.length, 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 11);
    ASSERT_TRUE(memcmp(tok.text, "hello world", 11) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_atx_heading(void)
{
    setup();
    FlareTokenSource *ts = lex_string("# Heading");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_HEADING_MARKER);
    ASSERT_EQ(tok.length, 2);
    ASSERT_TRUE(memcmp(tok.text, "# ", 2) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 7);
    ASSERT_TRUE(memcmp(tok.text, "Heading", 7) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_thematic_break(void)
{
    setup();
    FlareTokenSource *ts = lex_string("---");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_THEMATIC_BREAK);
    ASSERT_EQ(tok.length, 3);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_inline_emphasis(void)
{
    setup();
    FlareTokenSource *ts = lex_string("*emphasis*");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_PARAGRAPH);
    ASSERT_EQ(tok.length, 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_INLINE_EMPHASIS);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "*", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 8);
    ASSERT_TRUE(memcmp(tok.text, "emphasis", 8) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_INLINE_EMPHASIS);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "*", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_inline_strong(void)
{
    setup();
    FlareTokenSource *ts = lex_string("**strong**");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_PARAGRAPH);
    ASSERT_EQ(tok.length, 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_INLINE_STRONG);
    ASSERT_EQ(tok.length, 2);
    ASSERT_TRUE(memcmp(tok.text, "**", 2) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 6);
    ASSERT_TRUE(memcmp(tok.text, "strong", 6) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_INLINE_STRONG);
    ASSERT_EQ(tok.length, 2);
    ASSERT_TRUE(memcmp(tok.text, "**", 2) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_inline_code(void)
{
    setup();
    FlareTokenSource *ts = lex_string("`code`");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_PARAGRAPH);
    ASSERT_EQ(tok.length, 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_INLINE_CODE);
    ASSERT_EQ(tok.length, 4);
    ASSERT_TRUE(memcmp(tok.text, "code", 4) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_inline_code_with_backtick(void)
{
    setup();
    FlareTokenSource *ts = lex_string("`` ` ``");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_PARAGRAPH);
    ASSERT_EQ(tok.length, 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_INLINE_CODE);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "`", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_fenced_code_block(void)
{
    setup();
    FlareTokenSource *ts = lex_string("```\nhello\n```");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_FENCED_OPEN);
    ASSERT_EQ(tok.length, 3);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_INDENTED_CODE);
    ASSERT_EQ(tok.length, 5);
    ASSERT_TRUE(memcmp(tok.text, "hello", 5) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_FENCED_CLOSE);
    ASSERT_EQ(tok.length, 3);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_bullet_list(void)
{
    setup();
    FlareTokenSource *ts = lex_string("- one\n- two\n- three\n");
    FlareToken tok;

    const char *items[] = { "one", "two", "three" };
    size_t lens[] = { 3, 3, 5 };
    for (size_t n = 0; n < 3; n++) {
        ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
        ASSERT_EQ(tok.type, HL_MARKUP_LIST_MARKER);

        ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
        ASSERT_EQ(tok.type, HL_TEXT);
        ASSERT_EQ(tok.length, lens[n]);
        ASSERT_TRUE(memcmp(tok.text, items[n], lens[n]) == 0);

        ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
        ASSERT_EQ(tok.type, HL_TEXT);
        ASSERT_EQ(tok.length, 1);
        ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);
    }

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_ordered_list(void)
{
    setup();
    FlareTokenSource *ts = lex_string("1. first\n2. second\n");
    FlareToken tok;

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_LIST_MARKER);
    ASSERT_EQ(tok.length, 3);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 5);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_LIST_MARKER);
    ASSERT_EQ(tok.length, 3);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 6);
    ASSERT_TRUE(memcmp(tok.text, "second", 6) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_loose_bullet_list(void)
{
    setup();
    FlareTokenSource *ts = lex_string("- one\n\n- two\n- three\n");
    FlareToken tok;

    const char *items[] = { "one", "two", "three" };
    size_t lens[] = { 3, 3, 5 };
    for (size_t n = 0; n < 3; n++) {
        ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
        ASSERT_EQ(tok.type, HL_MARKUP_LIST_MARKER);

        ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
        ASSERT_EQ(tok.type, HL_TEXT);
        ASSERT_EQ(tok.length, lens[n]);
        ASSERT_TRUE(memcmp(tok.text, items[n], lens[n]) == 0);

        ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
        ASSERT_EQ(tok.type, HL_TEXT);
        ASSERT_EQ(tok.length, 1);
        ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);
    }

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_list_interrupts_paragraph(void)
{
    setup();
    FlareTokenSource *ts = lex_string("Foo\n- bar\n- baz\n");
    FlareToken tok;

    /* Paragraph "Foo" */
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_PARAGRAPH);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 3);
    ASSERT_TRUE(memcmp(tok.text, "Foo", 3) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    /* First list item */
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_LIST_MARKER);
    ASSERT_EQ(tok.length, 2);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 3);
    ASSERT_TRUE(memcmp(tok.text, "bar", 3) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    /* Second list item */
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_LIST_MARKER);
    ASSERT_EQ(tok.length, 2);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 3);
    ASSERT_TRUE(memcmp(tok.text, "baz", 3) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_nested_bullet_list(void)
{
    setup();
    FlareTokenSource *ts = lex_string("- foo\n  - bar\n    - baz\n");
    FlareToken tok;

    /* Outer item: "- foo" */
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_LIST_MARKER);
    ASSERT_EQ(tok.length, 2);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 3);
    ASSERT_TRUE(memcmp(tok.text, "foo", 3) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    /* Nested item: "- bar" */
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_LIST_MARKER);
    ASSERT_EQ(tok.length, 2);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 3);
    ASSERT_TRUE(memcmp(tok.text, "bar", 3) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    /* Inner-nested item: "- baz" */
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_LIST_MARKER);
    ASSERT_EQ(tok.length, 2);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 3);
    ASSERT_TRUE(memcmp(tok.text, "baz", 3) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_block_quote(void)
{
    setup();
    FlareTokenSource *ts = lex_string("> # Foo\n> bar\n> baz\n");
    FlareToken tok;

    /* Block quote marker */
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_BLOCKQUOTE_MARKER);
    ASSERT_EQ(tok.length, 2);
    ASSERT_TRUE(memcmp(tok.text, "> ", 2) == 0);

    /* Heading inside block quote */
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_HEADING_MARKER);
    ASSERT_EQ(tok.length, 2);
    ASSERT_TRUE(memcmp(tok.text, "# ", 2) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 3);
    ASSERT_TRUE(memcmp(tok.text, "Foo", 3) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    /* Second line marker and text */
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_BLOCKQUOTE_MARKER);
    ASSERT_EQ(tok.length, 2);
    ASSERT_TRUE(memcmp(tok.text, "> ", 2) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 3);
    ASSERT_TRUE(memcmp(tok.text, "bar", 3) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    /* Third line marker and text */
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_BLOCKQUOTE_MARKER);
    ASSERT_EQ(tok.length, 2);
    ASSERT_TRUE(memcmp(tok.text, "> ", 2) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 3);
    ASSERT_TRUE(memcmp(tok.text, "baz", 3) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

static void test_cm_setext_heading(void)
{
    setup();
    FlareTokenSource *ts = lex_string("Foo *bar*\n=========\n");
    FlareToken tok;

    /* Heading marker "= " (setext underline normalized) */
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_HEADING_MARKER);
    ASSERT_EQ(tok.length, 2);
    ASSERT_TRUE(memcmp(tok.text, "= ", 2) == 0);

    /* Content: "Foo " then emphasis */
    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 4);
    ASSERT_TRUE(memcmp(tok.text, "Foo ", 4) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_INLINE_EMPHASIS);
    ASSERT_EQ(tok.length, 1);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 3);
    ASSERT_TRUE(memcmp(tok.text, "bar", 3) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_MARKUP_INLINE_EMPHASIS);
    ASSERT_EQ(tok.length, 1);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 1);
    ASSERT_EQ(tok.type, HL_TEXT);
    ASSERT_EQ(tok.length, 1);
    ASSERT_TRUE(memcmp(tok.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(ts, &tok), 0);
    flare_token_source_free(ts);
    teardown();
}

int main(void)
{
    RUN_TEST(test_cm_empty);
    RUN_TEST(test_cm_paragraph);
    RUN_TEST(test_cm_atx_heading);
    RUN_TEST(test_cm_thematic_break);
    RUN_TEST(test_cm_inline_emphasis);
    RUN_TEST(test_cm_inline_strong);
    RUN_TEST(test_cm_inline_code);
    RUN_TEST(test_cm_inline_code_with_backtick);
    RUN_TEST(test_cm_fenced_code_block);
    RUN_TEST(test_cm_bullet_list);
    RUN_TEST(test_cm_ordered_list);
    RUN_TEST(test_cm_loose_bullet_list);
    RUN_TEST(test_cm_list_interrupts_paragraph);
    RUN_TEST(test_cm_nested_bullet_list);
    RUN_TEST(test_cm_block_quote);
    RUN_TEST(test_cm_setext_heading);

    TEST_SUMMARY();
    return 0;
}
