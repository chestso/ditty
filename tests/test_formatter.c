/* test_formatter.c - Formatter ANSI output tests */

#include "flare_testkit.h"
#include "lisp.h"
#include <bloom-lisp/highlight.h>
#include <string.h>

static Environment *env;

static void test_formatter_truecolor_produces_rgb(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_bloom_lisp(env);
    FlareResult r = flare_highlight("\"hello\"", 7, lex, style, fmt);
    ASSERT_TRUE(strstr(r.data, "38;2;") != NULL);
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_256_produces_palette_index(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_256);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_bloom_lisp(env);
    FlareResult r = flare_highlight("\"hello\"", 7, lex, style, fmt);
    ASSERT_TRUE(strstr(r.data, "38;5;") != NULL);
    ASSERT_TRUE(strstr(r.data, "38;2;") == NULL);
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_16_produces_aixterm_ansi(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_16);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_bloom_lisp(env);
    FlareResult r = flare_highlight("\"hello\"", 7, lex, style, fmt);
    ASSERT_TRUE(strstr(r.data, "38;") == NULL);
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_8_produces_basic_ansi(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_8);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_bloom_lisp(env);
    FlareResult r = flare_highlight("\"hello\"", 7, lex, style, fmt);
    ASSERT_TRUE(strstr(r.data, "38;") == NULL);
    ASSERT_TRUE(strstr(r.data, "\033[90") == NULL);
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_ends_with_reset(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_bloom_lisp(env);
    FlareResult r = flare_highlight("(+ 1 2)", 7, lex, style, fmt);
    size_t len = r.length;
    ASSERT_TRUE(len >= 4);
    ASSERT_STR_EQ(r.data + len - 4, "\033[0m");
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_coalesces_same_style(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_bloom_lisp(env);
    FlareResult r = flare_highlight("(1 2 3)", 7, lex, style, fmt);
    int sgr_count = 0;
    for (size_t i = 0; i < r.length; i++) {
        if (r.data[i] == '\033' && r.data[i + 1] == '[')
            sgr_count++;
    }
    ASSERT_TRUE(sgr_count <= 8);
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

int main(void)
{
    env = lisp_init();
    RUN_TEST(test_formatter_truecolor_produces_rgb);
    RUN_TEST(test_formatter_256_produces_palette_index);
    RUN_TEST(test_formatter_16_produces_aixterm_ansi);
    RUN_TEST(test_formatter_8_produces_basic_ansi);
    RUN_TEST(test_formatter_ends_with_reset);
    RUN_TEST(test_formatter_coalesces_same_style);
    lisp_cleanup();
    TEST_SUMMARY();
}
