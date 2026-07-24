/* test_flare.c - Flare streaming tests */

#include "flare_testkit.h"
#include "../include/ditty/highlight.h"
#include "../include/ditty/flare_source.h"
#include "../include/ditty/flare_writer.h"
#include "../include/ditty/flare_token_source.h"
#include "../include/ditty/flare_iterator.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ===== Mock token source for testing ===== */

typedef struct
{
    FlareTokenSource base;
    FlareToken *tokens;
    size_t count;
    size_t pos;
} MockTokenSource;

static int mock_pull(FlareTokenSource *src, FlareToken *out)
{
    MockTokenSource *m = (MockTokenSource *)src;
    if (m->pos >= m->count)
        return 0;
    *out = m->tokens[m->pos++];
    return 1;
}

static const char *mock_error(FlareTokenSource *src)
{
    (void)src;
    return NULL;
}

static void mock_free(FlareTokenSource *src)
{
    free(src);
}

static const FlareTokenSourceVTable mock_vtable = {
    .pull = mock_pull,
    .error = mock_error,
    .free = mock_free
};

static FlareTokenSource *mock_source(FlareToken *tokens, size_t count)
{
    MockTokenSource *m = calloc(1, sizeof(MockTokenSource));
    m->base.vtable = &mock_vtable;
    m->tokens = tokens;
    m->count = count;
    m->pos = 0;
    return &m->base;
}

/* ===== Source tests ===== */

/* String source returns correct bytes and EOF */
static void test_source_string_read(void)
{
    const char *text = "hello";
    FlareSource *src = flare_source_string(text, strlen(text), 0);
    ASSERT_NOT_NULL(src);

    char buf[16];
    ssize_t n = flare_source_read(src, buf, sizeof(buf));
    ASSERT_EQ(n, 5);
    ASSERT_TRUE(memcmp(buf, "hello", 5) == 0);

    n = flare_source_read(src, buf, sizeof(buf));
    ASSERT_EQ(n, 0);

    flare_source_free(src);
}

/* File source reads in chunks */
static void test_source_file_read(void)
{
    FILE *f = tmpfile();
    ASSERT_NOT_NULL(f);
    fwrite("hello world", 1, 11, f);
    rewind(f);

    FlareSource *src = flare_source_file(f, "test");
    char buf[5];

    ssize_t n = flare_source_read(src, buf, 5);
    ASSERT_EQ(n, 5);
    ASSERT_TRUE(memcmp(buf, "hello", 5) == 0);

    n = flare_source_read(src, buf, 5);
    ASSERT_EQ(n, 5);
    ASSERT_TRUE(memcmp(buf, " worl", 5) == 0);

    flare_source_free(src);
}

/* ===== Writer tests ===== */

/* Buffer writer accumulates bytes */
static void test_writer_buffer(void)
{
    FlareWriter *w = flare_writer_buffer();
    ASSERT_NOT_NULL(w);

    flare_writer_write(w, "hello", 5);
    flare_writer_write(w, " world", 6);
    flare_writer_flush(w);

    ASSERT_EQ(flare_writer_buffer_len(w), 11);
    ASSERT_TRUE(memcmp(flare_writer_buffer_data(w), "hello world", 11) == 0);

    flare_writer_free(w);
}

/* Null writer discards output */
static void test_writer_null(void)
{
    FlareWriter *w = flare_writer_null();
    ASSERT_NOT_NULL(w);

    flare_writer_write(w, "hello", 5);
    flare_writer_flush(w);

    flare_writer_free(w);
}

/* ===== Token source tests ===== */

/* Vtable dispatch works */
static void test_token_source_vtable(void)
{
    FlareToken tokens[] = {
        { HL_TEXT, "hello", 5 },
        { HL_TEXT, " world", 6 }
    };
    FlareTokenSource *src = mock_source(tokens, 2);
    FlareToken out;

    ASSERT_EQ(flare_token_source_pull(src, &out), 1);
    ASSERT_EQ(out.type, HL_TEXT);
    ASSERT_EQ(out.length, 5);
    ASSERT_TRUE(memcmp(out.text, "hello", 5) == 0);

    ASSERT_EQ(flare_token_source_pull(src, &out), 1);
    ASSERT_EQ(out.length, 6);

    ASSERT_EQ(flare_token_source_pull(src, &out), 0);

    flare_token_source_free(src);
}

/* ===== Layout tests ===== */

/* FlareLayout can be updated and read */
static void test_layout_update(void)
{
    FlareLayout layout = { .width = 76, .terminal_rows = 24, .resized = 0 };

    ASSERT_EQ(layout.width, 76);
    ASSERT_EQ(layout.terminal_rows, 24);
    ASSERT_EQ(layout.resized, 0);

    layout.width = 120;
    layout.resized = 1;

    ASSERT_EQ(layout.width, 120);
    ASSERT_EQ(layout.resized, 1);
}

/* ===== Reflow iterator tests ===== */

/* Reflow iterator passes non-text tokens through */
static void test_reflow_passes_non_text_through(void)
{
    FlareToken tokens[] = {
        { HL_KEYWORD_DEFINE, "define", 6 },
        { HL_NAME_FUNCTION, "foo", 3 },
        { HL_PUNCT_OPEN_PAREN, "(", 1 }
    };
    FlareTokenSource *src = mock_source(tokens, 3);
    FlareLayout layout = { .width = 76, .resized = 0 };
    FlareReflowOptions opts = FLARE_REFLOW_DEFAULT;
    FlareTokenSource *reflowed = flare_iterator_reflow(src, &layout, &opts);

    FlareToken out;
    ASSERT_EQ(flare_token_source_pull(reflowed, &out), 1);
    ASSERT_EQ(out.type, HL_KEYWORD_DEFINE);
    ASSERT_EQ(out.length, 6);

    ASSERT_EQ(flare_token_source_pull(reflowed, &out), 1);
    ASSERT_EQ(out.type, HL_NAME_FUNCTION);

    ASSERT_EQ(flare_token_source_pull(reflowed, &out), 1);
    ASSERT_EQ(out.type, HL_PUNCT_OPEN_PAREN);

    ASSERT_EQ(flare_token_source_pull(reflowed, &out), 0);

    flare_token_source_free(reflowed);
}

/* Reflow iterator splits text token at word boundaries */
static void test_reflow_splits_words(void)
{
    FlareToken tokens[] = {
        { HL_TEXT, "hello world", 11 }
    };
    FlareTokenSource *src = mock_source(tokens, 1);
    FlareLayout layout = { .width = 76, .resized = 0 };
    FlareReflowOptions opts = FLARE_REFLOW_DEFAULT;
    FlareTokenSource *reflowed = flare_iterator_reflow(src, &layout, &opts);

    FlareToken out;
    ASSERT_EQ(flare_token_source_pull(reflowed, &out), 1);
    ASSERT_EQ(out.type, HL_TEXT);
    ASSERT_EQ(out.length, 5);
    ASSERT_TRUE(memcmp(out.text, "hello", 5) == 0);

    ASSERT_EQ(flare_token_source_pull(reflowed, &out), 1);
    ASSERT_EQ(out.type, HL_TEXT);
    ASSERT_EQ(out.length, 5);
    ASSERT_TRUE(memcmp(out.text, "world", 5) == 0);

    ASSERT_EQ(flare_token_source_pull(reflowed, &out), 0);

    flare_token_source_free(reflowed);
}

/* Reflow iterator wraps at width boundary */
static void test_reflow_wraps_at_width(void)
{
    FlareToken tokens[] = {
        { HL_TEXT, "hello world", 11 }
    };
    FlareTokenSource *src = mock_source(tokens, 1);
    FlareLayout layout = { .width = 8, .resized = 0 };
    FlareReflowOptions opts = FLARE_REFLOW_DEFAULT;
    FlareTokenSource *reflowed = flare_iterator_reflow(src, &layout, &opts);

    FlareToken out;
    ASSERT_EQ(flare_token_source_pull(reflowed, &out), 1);
    ASSERT_EQ(out.length, 5);
    ASSERT_TRUE(memcmp(out.text, "hello", 5) == 0);

    ASSERT_EQ(flare_token_source_pull(reflowed, &out), 1);
    ASSERT_EQ(out.length, 1);
    ASSERT_TRUE(memcmp(out.text, "\n", 1) == 0);

    ASSERT_EQ(flare_token_source_pull(reflowed, &out), 1);
    ASSERT_EQ(out.length, 5);
    ASSERT_TRUE(memcmp(out.text, "world", 5) == 0);

    ASSERT_EQ(flare_token_source_pull(reflowed, &out), 0);

    flare_token_source_free(reflowed);
}

/* Formatter inserts blank-line spacing between paragraph blocks */
static void test_formatter_block_spacing(void)
{
    FlareStyle *style = flare_style_dracula();
    ASSERT_NOT_NULL(style);

    FlareToken tokens[] = {
        { HL_MARKUP_PARAGRAPH, "", 0 },
        { HL_TEXT, "first", 5 },
        { HL_TEXT, "\n", 1 },
        { HL_MARKUP_PARAGRAPH, "", 0 },
        { HL_TEXT, "second", 6 },
        { HL_TEXT, "\n", 1 },
    };

    char *ansi = flare_format_terminal(tokens, 6, style, BFLARE_COLOR_TRUECOLOR, 0);
    ASSERT_NOT_NULL(ansi);

    /* Should contain "first" followed by a blank line then "second".
     * The blank line is represented by two consecutive newlines. */
    ASSERT_TRUE(strstr(ansi, "first\n\nsecond") != NULL);

    free(ansi);
    flare_style_free(style);
}

int main(void)
{
    RUN_TEST(test_source_string_read);
    RUN_TEST(test_source_file_read);
    RUN_TEST(test_writer_buffer);
    RUN_TEST(test_writer_null);
    RUN_TEST(test_token_source_vtable);
    RUN_TEST(test_layout_update);
    RUN_TEST(test_reflow_passes_non_text_through);
    RUN_TEST(test_reflow_splits_words);
    RUN_TEST(test_reflow_wraps_at_width);
    RUN_TEST(test_formatter_block_spacing);

    TEST_SUMMARY();
    return 0;
}
