/* test_flare_terminal_style.c - Tests for FlareTerminalStyle */

#include "flare_testkit.h"
#include "../include/ditty/highlight.h"
#include <string.h>
#include <stdlib.h>

/* ===== Terminal style default value tests ===== */

/* Default terminal style has correct margin values */
static void test_terminal_style_defaults(void)
{
    FlareTerminalStyle ts = FLARE_TERMINAL_STYLE_DEFAULT;

    ASSERT_EQ(ts.heading_margin_top, 1);
    ASSERT_EQ(ts.heading_margin_bottom, 1);
    ASSERT_EQ(ts.paragraph_margin_bottom, 1);
    ASSERT_EQ(ts.fenced_margin_top, 1);
    ASSERT_EQ(ts.fenced_margin_bottom, 1);
    ASSERT_EQ(ts.thematic_break_margin, 1);
    ASSERT_EQ(ts.fenced_indent, 2);
    ASSERT_EQ(ts.enable_hyperlinks, 0);
}

/* ===== Heading margin tests ===== */

/* Heading gets blank line before it */
static void test_heading_margin_top(void)
{
    FlareStyle *style = flare_style_dracula();
    ASSERT_NOT_NULL(style);

    FlareToken tokens[] = {
        { HL_MARKUP_PARAGRAPH, "", 0 },
        { HL_TEXT, "before", 6 },
        { HL_TEXT, "\n", 1 },
        { HL_MARKUP_HEADING_MARKER, "##", 2 },
        { HL_TEXT, " heading", 8 },
    };

    FlareTerminalStyle ts = FLARE_TERMINAL_STYLE_DEFAULT;

    char *ansi = flare_format_terminal_with_style(tokens, 5, style,
                                                  BFLARE_COLOR_TRUECOLOR, 0, &ts);
    ASSERT_NOT_NULL(ansi);

    /* Should have blank line before heading (two consecutive newlines) */
    ASSERT_TRUE(strstr(ansi, "before\n\n") != NULL);

    free(ansi);
    flare_style_free(style);
}

/* Heading gets blank line after it */
static void test_heading_margin_bottom(void)
{
    FlareStyle *style = flare_style_dracula();
    ASSERT_NOT_NULL(style);

    FlareToken tokens[] = {
        { HL_MARKUP_HEADING_MARKER, "##", 2 },
        { HL_TEXT, " heading", 8 },
        { HL_TEXT, "\n", 1 },
        { HL_MARKUP_PARAGRAPH, "", 0 },
        { HL_TEXT, "content", 7 },
    };

    FlareTerminalStyle ts = FLARE_TERMINAL_STYLE_DEFAULT;

    char *ansi = flare_format_terminal_with_style(tokens, 5, style,
                                                  BFLARE_COLOR_TRUECOLOR, 0, &ts);
    ASSERT_NOT_NULL(ansi);

    /* Should have blank line after heading (two consecutive newlines) */
    ASSERT_TRUE(strstr(ansi, "\n\ncontent") != NULL);

    free(ansi);
    flare_style_free(style);
}

/* ===== Paragraph margin tests ===== */

/* Paragraph gets blank line after it */
static void test_paragraph_margin_bottom(void)
{
    FlareStyle *style = flare_style_dracula();
    ASSERT_NOT_NULL(style);

    FlareToken tokens[] = {
        { HL_MARKUP_PARAGRAPH, "", 0 },
        { HL_TEXT, "first", 5 },
        { HL_TEXT, "\n", 1 },
        { HL_MARKUP_PARAGRAPH, "", 0 },
        { HL_TEXT, "second", 6 },
    };

    FlareTerminalStyle ts = FLARE_TERMINAL_STYLE_DEFAULT;

    char *ansi = flare_format_terminal_with_style(tokens, 5, style,
                                                  BFLARE_COLOR_TRUECOLOR, 0, &ts);
    ASSERT_NOT_NULL(ansi);

    /* Should have blank line between paragraphs */
    ASSERT_TRUE(strstr(ansi, "first\n\nsecond") != NULL);

    free(ansi);
    flare_style_free(style);
}

/* ===== Fenced code block tests ===== */

/* Fenced code block gets margin and indentation */
static void test_fenced_margin_and_indent(void)
{
    FlareStyle *style = flare_style_dracula();
    ASSERT_NOT_NULL(style);

    FlareToken tokens[] = {
        { HL_MARKUP_PARAGRAPH, "", 0 },
        { HL_TEXT, "before", 6 },
        { HL_TEXT, "\n", 1 },
        { HL_MARKUP_FENCED_OPEN, "```", 3 },
        { HL_TEXT, "\ncode\n", 6 },
        { HL_MARKUP_FENCED_CLOSE, "```", 3 },
        { HL_TEXT, "\n", 1 },
        { HL_TEXT, "after", 5 },
    };

    FlareTerminalStyle ts = FLARE_TERMINAL_STYLE_DEFAULT;

    char *ansi = flare_format_terminal_with_style(tokens, 8, style,
                                                  BFLARE_COLOR_TRUECOLOR, 0, &ts);
    ASSERT_NOT_NULL(ansi);

    /* Should have blank line before fenced block */
    ASSERT_TRUE(strstr(ansi, "before\n\n") != NULL);

    /* Code inside fence should be indented by 2 spaces */
    ASSERT_TRUE(strstr(ansi, "\n  code") != NULL);

    free(ansi);
    flare_style_free(style);
}

/* Fenced indent is configurable */
static void test_fenced_indent_custom(void)
{
    FlareStyle *style = flare_style_dracula();
    ASSERT_NOT_NULL(style);

    FlareToken tokens[] = {
        { HL_MARKUP_FENCED_OPEN, "```", 3 },
        { HL_TEXT, "\ncode\n", 6 },
        { HL_MARKUP_FENCED_CLOSE, "```", 3 },
    };

    FlareTerminalStyle ts = FLARE_TERMINAL_STYLE_DEFAULT;
    ts.fenced_indent = 4; /* Custom 4-space indent */

    char *ansi = flare_format_terminal_with_style(tokens, 3, style,
                                                  BFLARE_COLOR_TRUECOLOR, 0, &ts);
    ASSERT_NOT_NULL(ansi);

    /* Code inside fence should be indented by 4 spaces */
    ASSERT_TRUE(strstr(ansi, "\n    code") != NULL);

    free(ansi);
    flare_style_free(style);
}

/* ===== Custom margin tests ===== */

/* Zero margins produce no blank lines */
static void test_zero_margins(void)
{
    FlareStyle *style = flare_style_dracula();
    ASSERT_NOT_NULL(style);

    FlareToken tokens[] = {
        { HL_MARKUP_PARAGRAPH, "", 0 },
        { HL_TEXT, "first", 5 },
        { HL_TEXT, "\n", 1 },
        { HL_MARKUP_PARAGRAPH, "", 0 },
        { HL_TEXT, "second", 6 },
    };

    FlareTerminalStyle ts = FLARE_TERMINAL_STYLE_DEFAULT;
    ts.paragraph_margin_bottom = 0; /* Disable paragraph spacing */

    char *ansi = flare_format_terminal_with_style(tokens, 5, style,
                                                  BFLARE_COLOR_TRUECOLOR, 0, &ts);
    ASSERT_NOT_NULL(ansi);

    /* Should NOT have blank line between paragraphs */
    ASSERT_TRUE(strstr(ansi, "first\n\nsecond") == NULL);
    ASSERT_TRUE(strstr(ansi, "first\nsecond") != NULL);

    free(ansi);
    flare_style_free(style);
}

/* Larger margin values produce multiple blank lines */
static void test_large_margins(void)
{
    FlareStyle *style = flare_style_dracula();
    ASSERT_NOT_NULL(style);

    FlareToken tokens[] = {
        { HL_MARKUP_PARAGRAPH, "", 0 },
        { HL_TEXT, "first", 5 },
        { HL_TEXT, "\n", 1 },
        { HL_MARKUP_PARAGRAPH, "", 0 },
        { HL_TEXT, "second", 6 },
    };

    FlareTerminalStyle ts = FLARE_TERMINAL_STYLE_DEFAULT;
    ts.paragraph_margin_bottom = 2; /* Two blank lines */

    char *ansi = flare_format_terminal_with_style(tokens, 5, style,
                                                  BFLARE_COLOR_TRUECOLOR, 0, &ts);
    ASSERT_NOT_NULL(ansi);

    /* Should have two blank lines (three consecutive newlines) */
    ASSERT_TRUE(strstr(ansi, "first\n\n\nsecond") != NULL);

    free(ansi);
    flare_style_free(style);
}

int main(void)
{
    RUN_TEST(test_terminal_style_defaults);
    RUN_TEST(test_heading_margin_top);
    RUN_TEST(test_heading_margin_bottom);
    RUN_TEST(test_paragraph_margin_bottom);
    RUN_TEST(test_fenced_margin_and_indent);
    RUN_TEST(test_fenced_indent_custom);
    RUN_TEST(test_zero_margins);
    RUN_TEST(test_large_margins);

    TEST_SUMMARY();
    return 0;
}
