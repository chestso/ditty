/* test_formatter.c - Formatter ANSI output tests */

#include "flare_testkit.h"
#include "lisp.h"
#include <ditty/highlight.h>
#include <string.h>

static Environment *env;

static void test_formatter_truecolor_produces_rgb(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_ditty(env);
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
    FlareLexer *lex = flare_lexer_ditty(env);
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
    FlareLexer *lex = flare_lexer_ditty(env);
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
    FlareLexer *lex = flare_lexer_ditty(env);
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
    FlareLexer *lex = flare_lexer_ditty(env);
    FlareResult r = flare_highlight("(+ 1 2)", 7, lex, style, fmt);
    size_t len = r.length;
    ASSERT_TRUE(len >= 4);
    ASSERT_STR_EQ(r.data + len - 4, "\033[0m");
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_bold_does_not_leak_into_next_token(void)
{
    /* Regression: bold from HL_KEYWORD_DEFINE (e.g. "defun", "set!")
     * was persisting into subsequent non-bold tokens because the SGR
     * sequence only set new attributes without first resetting old ones.
     * ANSI attributes are cumulative — \033[38;2;...m does NOT cancel
     * a prior \033[1m.  Every SGR must lead with 0 (reset). */
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_ditty(env);
    FlareResult r = flare_highlight("(set! x 1)", 10, lex, style, fmt);

    /* Find the SGR right after the bold "set!" token.
     * The next token should start with \033[0; (reset) and NOT contain
     * ";1;" (bold) since HL_NAME_VARIABLE and HL_PUNCTUATION are non-bold. */
    const char *set_sgr = strstr(r.data, "\033[0;1;");
    ASSERT_NOT_NULL(set_sgr); /* "set!" itself must be bold */

    /* Scan SGR sequences after the bold one: none should contain ";1;"
     * (bold parameter) unless the token actually requests bold. */
    const char *p = set_sgr + 4; /* skip past the bold SGR intro */
    while ((p = strstr(p, "\033[")) != NULL) {
        /* Find the 'm' that ends this SGR */
        const char *m = strchr(p, 'm');
        if (!m)
            break;
        /* The trailing \033[0m reset is expected — skip it */
        if (m - p == 3 && p[1] == '0') {
            p = m + 1;
            continue;
        }
        /* Check that ";1;" does not appear inside the SGR params */
        size_t sgr_len = m - p;
        char sgr_buf[128];
        ASSERT_TRUE(sgr_len < sizeof(sgr_buf));
        memcpy(sgr_buf, p, sgr_len);
        sgr_buf[sgr_len] = '\0';
        /* ";1;" = bold as a non-leading param; "0;1;" = reset+bold (ok) */
        ASSERT_TRUE(strstr(sgr_buf, ";1;") == NULL);
        p = m + 1;
    }

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_coalesces_same_style(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_ditty(env);
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

/* Non-lisp fenced code blocks also have fence markers suppressed.
 * The opening/closing ``` and info strings are structural, not content. */
static void test_formatter_plain_fenced_suppresses_markers(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_dracula();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("```ruby\ncode\n```\n", 16,
                                    lex, style, fmt);

    /* Strip ANSI escapes to get plain text */
    char plain[256];
    size_t j = 0;
    for (size_t i = 0; i < r.length && j < sizeof(plain) - 1; i++) {
        if (r.data[i] == '\033') {
            while (i < r.length && r.data[i] != 'm')
                i++;
            continue;
        }
        plain[j++] = r.data[i];
    }
    plain[j] = '\0';

    /* Fence markers and info string should NOT appear in output */
    ASSERT_TRUE(strstr(plain, "```") == NULL);
    ASSERT_TRUE(strstr(plain, "ruby") == NULL);
    /* Code content should appear */
    ASSERT_TRUE(strstr(plain, "code") != NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Regression: fenced code block markers and their line-ending newlines
 * must be suppressed from rendered output.  Previously the opening fence
 * newline was emitted as a standalone HL_TEXT token, producing a spurious
 * blank line between the preceding paragraph and the code content.
 * Input:  "Text\n\n```lisp\n'()\n```\n"
 * Output should have exactly one blank line (paragraph separator)
 * between "Text" and "'()", not two. */
static void test_formatter_fenced_no_spurious_blank_line(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_dracula();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("Text\n\n```lisp\n'()\n```\n", 22,
                                    lex, style, fmt);

    /* Strip ANSI escapes to get plain text */
    char plain[256];
    size_t j = 0;
    for (size_t i = 0; i < r.length && j < sizeof(plain) - 1; i++) {
        if (r.data[i] == '\033') {
            while (i < r.length && r.data[i] != 'm')
                i++;
            continue;
        }
        plain[j++] = r.data[i];
    }
    plain[j] = '\0';

    /* Expected: "Text\n\n  '()\n" — one blank line, not two.
     * A spurious blank line would produce "Text\n\n\n  '()\n"
     * (three consecutive newlines). */
    const char *triple = strstr(plain, "\n\n\n");
    ASSERT_TRUE(triple == NULL);

    /* Verify the content is actually present */
    ASSERT_TRUE(strstr(plain, "Text") != NULL);
    ASSERT_TRUE(strstr(plain, "'()") != NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Regression: blank lines within and around fenced code blocks must be
 * preserved.  The formatter should only suppress the fence markers and
 * their line-ending newlines, not legitimate blank lines:
 *   - blank line between preceding text and code block (paragraph sep)
 *   - blank line within the code block (between code lines)
 *   - blank line after the code block (paragraph sep) */
static void test_formatter_fenced_preserves_blank_lines(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_dracula();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("Text\n\n```lisp\nfoo\n\nbar\n```\n\nAfter\n",
                                    34, lex, style, fmt);

    /* Strip ANSI escapes to get plain text */
    char plain[512];
    size_t j = 0;
    for (size_t i = 0; i < r.length && j < sizeof(plain) - 1; i++) {
        if (r.data[i] == '\033') {
            while (i < r.length && r.data[i] != 'm')
                i++;
            continue;
        }
        plain[j++] = r.data[i];
    }
    plain[j] = '\0';

    /* Expected: "Text\n\n  foo\n  \n  bar\n\nAfter\n"
     * Fenced code content is indented by 2 spaces.
     * - blank line between Text and indented foo (paragraph separator)
     * - blank line within the code block (between foo and bar,
     *   also indented)
     * - blank line between bar and After (paragraph separator) */
    ASSERT_TRUE(strstr(plain, "Text\n\n") != NULL);
    ASSERT_TRUE(strstr(plain, "  foo\n  \n  bar") != NULL);
    ASSERT_TRUE(strstr(plain, "bar\n\nAfter") != NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Helper: strip ANSI escapes from `r.data` into `plain`.
 * Handles CSI SGR sequences (\x1b[...m) and OSC 8 sequences
 * (\x1b]...\x07) used for terminal hyperlinks. */
static void strip_ansi(FlareResult r, char *plain, size_t plain_size)
{
    size_t j = 0;
    for (size_t i = 0; i < r.length && j < plain_size - 1; i++) {
        if (r.data[i] == '\033') {
            if (i + 1 < r.length && r.data[i + 1] == ']') {
                /* OSC sequence: skip until BEL (\x07) */
                i += 2;
                while (i < r.length && r.data[i] != '\x07')
                    i++;
            } else {
                /* CSI sequence: skip until 'm' */
                while (i < r.length && r.data[i] != 'm')
                    i++;
            }
            continue;
        }
        plain[j++] = r.data[i];
    }
    plain[j] = '\0';
}

/* Inline code: backtick delimiters should be replaced with spaces,
 * and the content should have a gray background applied. */
static void test_inline_code_backticks_become_spaces(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("`foo`", 5, lex, style, fmt);

    char plain[256];
    strip_ansi(r, plain, sizeof(plain));

    /* Should contain " foo " (spaces where backticks were) */
    ASSERT_TRUE(strstr(plain, " foo ") != NULL);
    /* Should NOT contain any backtick character */
    ASSERT_TRUE(strchr(plain, '`') == NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Double backtick code spans: both backticks on each side become spaces */
static void test_inline_code_double_backticks_become_spaces(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("`` foo ` bar ``", 15, lex, style, fmt);

    char plain[256];
    strip_ansi(r, plain, sizeof(plain));

    /* Two backticks on each side → two spaces on each side */
    ASSERT_TRUE(strstr(plain, "  foo ` bar  ") != NULL);
    /* No backtick at the boundaries (the inner ` in "foo ` bar" is content) */
    ASSERT_TRUE(plain[0] != '`');
    ASSERT_TRUE(plain[strlen(plain) - 1] != '`');

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Inline code should emit a background color SGR (48;2;R;G;B in truecolor) */
static void test_inline_code_has_background_color(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("`foo`", 5, lex, style, fmt);

    /* Should contain a 48;2; background color SGR */
    ASSERT_TRUE(strstr(r.data, "48;2;") != NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Inline code background should not appear on non-code tokens */
static void test_inline_code_bg_only_on_code(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("hello `world` end", 16, lex, style, fmt);

    /* The "hello" and "end" tokens should NOT have 48;2; */
    /* Find the SGR for "hello" — it's the first SGR in the output */
    const char *hello_sgr = strstr(r.data, "\033[");
    ASSERT_NOT_NULL(hello_sgr);
    /* Find the 'm' to get the full SGR */
    const char *m = strchr(hello_sgr, 'm');
    ASSERT_NOT_NULL(m);
    /* Check that the first SGR (for "hello") does not contain 48;2; */
    char first_sgr[128];
    size_t sgr_len = m - hello_sgr + 1;
    ASSERT_TRUE(sgr_len < sizeof(first_sgr));
    memcpy(first_sgr, hello_sgr, sgr_len);
    first_sgr[sgr_len] = '\0';
    ASSERT_TRUE(strstr(first_sgr, "48;2;") == NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Background color should also work at 256-color depth */
static void test_inline_code_bg_256_color(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_256);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("`foo`", 5, lex, style, fmt);

    /* Should contain a 48;5; background color SGR */
    ASSERT_TRUE(strstr(r.data, "48;5;") != NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Inline link: only the title text should be rendered, not the URL
 * or any markdown syntax characters. */
static void test_link_renders_only_title(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("[click here](https://example.com)", 33,
                                    lex, style, fmt);

    char plain[256];
    strip_ansi(r, plain, sizeof(plain));

    /* Should contain the title */
    ASSERT_TRUE(strstr(plain, "click here") != NULL);
    /* Should NOT contain the URL */
    ASSERT_TRUE(strstr(plain, "https://example.com") == NULL);
    /* Should NOT contain brackets or parens */
    ASSERT_TRUE(strchr(plain, '[') == NULL);
    ASSERT_TRUE(strchr(plain, ']') == NULL);
    ASSERT_TRUE(strchr(plain, '(') == NULL);
    ASSERT_TRUE(strchr(plain, ')') == NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Link with title attribute: [text](url "title") — only text is shown */
static void test_link_with_title_renders_only_text(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("[foo](https://x.com \"bar\")", 26,
                                    lex, style, fmt);

    char plain[256];
    strip_ansi(r, plain, sizeof(plain));

    ASSERT_TRUE(strstr(plain, "foo") != NULL);
    ASSERT_TRUE(strstr(plain, "https://x.com") == NULL);
    ASSERT_TRUE(strstr(plain, "bar") == NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Autolink: <url> renders the URL (it's both title and target) but
 * strips the angle brackets */
static void test_autolink_strips_angle_brackets(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("<https://example.com>", 21,
                                    lex, style, fmt);

    char plain[256];
    strip_ansi(r, plain, sizeof(plain));

    /* URL content should be present */
    ASSERT_TRUE(strstr(plain, "https://example.com") != NULL);
    /* Angle brackets should NOT be present */
    ASSERT_TRUE(strchr(plain, '<') == NULL);
    ASSERT_TRUE(strchr(plain, '>') == NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Link with surrounding text: the URL is suppressed, surrounding text is fine */
static void test_link_in_context_suppresses_url(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("See [docs](https://docs.x.com) now", 34,
                                    lex, style, fmt);

    char plain[256];
    strip_ansi(r, plain, sizeof(plain));

    ASSERT_TRUE(strstr(plain, "See ") != NULL);
    ASSERT_TRUE(strstr(plain, "docs") != NULL);
    ASSERT_TRUE(strstr(plain, " now") != NULL);
    ASSERT_TRUE(strstr(plain, "https://docs.x.com") == NULL);
    ASSERT_TRUE(strchr(plain, '[') == NULL);
    ASSERT_TRUE(strchr(plain, '(') == NULL);

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
    RUN_TEST(test_formatter_bold_does_not_leak_into_next_token);
    RUN_TEST(test_formatter_coalesces_same_style);
    RUN_TEST(test_formatter_plain_fenced_suppresses_markers);
    RUN_TEST(test_formatter_fenced_no_spurious_blank_line);
    RUN_TEST(test_formatter_fenced_preserves_blank_lines);
    RUN_TEST(test_inline_code_backticks_become_spaces);
    RUN_TEST(test_inline_code_double_backticks_become_spaces);
    RUN_TEST(test_inline_code_has_background_color);
    RUN_TEST(test_inline_code_bg_only_on_code);
    RUN_TEST(test_inline_code_bg_256_color);
    RUN_TEST(test_link_renders_only_title);
    RUN_TEST(test_link_with_title_renders_only_text);
    RUN_TEST(test_autolink_strips_angle_brackets);
    RUN_TEST(test_link_in_context_suppresses_url);
    lisp_cleanup();
    TEST_SUMMARY();
}
