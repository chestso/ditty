/* style_dracula.c - Built-in Dracula style
 *
 * The palette is the Dracula specification (https://draculatheme.com/spec)
 * and nothing else: every entry below is one of the spec's ten named
 * tokens, plus Selection for the inline-code background. This file is the
 * single source of the project's Dracula values - cli/colors.h mirrors it
 * for the REPL's chrome, and the REPL highlights through
 * flare_style_dracula() instead of a private palette.
 *
 * Role vocabulary:
 *   Foreground  text, variables, punctuation
 *   Comment     muted structure: comments, markup, nil
 *   Pink        the accent: keywords, operators, headings
 *   Purple      functions, links, special forms
 *   Cyan        builtins
 *   Green       string escapes
 *   Orange      constants: numbers, characters, booleans
 *   Yellow      strings, code spans, entities
 *   Red         errors
 */

#include "../include/ditty/highlight.h"
#include <stdlib.h>

/* ----- Palette (draculatheme.com/spec) ---------------------------------- */

/* #F8F8F2 Foreground */
#define DRA_FG_R 248
#define DRA_FG_G 248
#define DRA_FG_B 242

/* #6272A4 Comment */
#define DRA_COMMENT_R 98
#define DRA_COMMENT_G 114
#define DRA_COMMENT_B 164

/* #FF79C6 Pink */
#define DRA_PINK_R 255
#define DRA_PINK_G 121
#define DRA_PINK_B 198

/* #BD93F9 Purple */
#define DRA_PURPLE_R 189
#define DRA_PURPLE_G 147
#define DRA_PURPLE_B 249

/* #8BE9FD Cyan */
#define DRA_CYAN_R 139
#define DRA_CYAN_G 233
#define DRA_CYAN_B 253

/* #50FA7B Green */
#define DRA_GREEN_R 80
#define DRA_GREEN_G 250
#define DRA_GREEN_B 123

/* #F1FA8C Yellow */
#define DRA_YELLOW_R 241
#define DRA_YELLOW_G 250
#define DRA_YELLOW_B 140

/* #FFB86C Orange */
#define DRA_ORANGE_R 255
#define DRA_ORANGE_G 184
#define DRA_ORANGE_B 108

/* #FF5555 Red */
#define DRA_RED_R 255
#define DRA_RED_G 85
#define DRA_RED_B 85

/* #44475A Selection */
#define DRA_SELECTION_R 68
#define DRA_SELECTION_G 71
#define DRA_SELECTION_B 90

FlareStyle *flare_style_build_dracula(void)
{
    FlareStyle *s = flare_style_new();
    if (!s)
        return NULL;

    FlareStyleEntry e;

    /* --- Lisp: text, names, keywords --- */

    e = (FlareStyleEntry){ .fg_r = DRA_FG_R, .fg_g = DRA_FG_G, .fg_b = DRA_FG_B };
    flare_style_set(s, HL_TEXT, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B };
    flare_style_set(s, HL_KEYWORD, &e);

    /* Special forms carry the accent's quieter neighbour: they are
     * structure, not vocabulary. */
    e = (FlareStyleEntry){ .fg_r = DRA_PURPLE_R, .fg_g = DRA_PURPLE_G, .fg_b = DRA_PURPLE_B };
    flare_style_set(s, HL_KEYWORD_SPECIAL_FORM, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B, .bold = 1 };
    flare_style_set(s, HL_KEYWORD_DEFINE, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B };
    flare_style_set(s, HL_KEYWORD_CONTROL, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B, .bold = 1 };
    flare_style_set(s, HL_KEYWORD_MACRO_DEF, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PURPLE_R, .fg_g = DRA_PURPLE_G, .fg_b = DRA_PURPLE_B };
    flare_style_set(s, HL_NAME_FUNCTION, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_CYAN_R, .fg_g = DRA_CYAN_G, .fg_b = DRA_CYAN_B };
    flare_style_set(s, HL_NAME_BUILTIN, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_FG_R, .fg_g = DRA_FG_G, .fg_b = DRA_FG_B };
    flare_style_set(s, HL_NAME_VARIABLE, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B };
    flare_style_set(s, HL_NAME_KEYWORD_ARG, &e);

    /* --- Lisp: literals --- */

    e = (FlareStyleEntry){ .fg_r = DRA_YELLOW_R, .fg_g = DRA_YELLOW_G, .fg_b = DRA_YELLOW_B };
    flare_style_set(s, HL_LITERAL_STRING, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_GREEN_R, .fg_g = DRA_GREEN_G, .fg_b = DRA_GREEN_B };
    flare_style_set(s, HL_LITERAL_STRING_ESCAPE, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_ORANGE_R, .fg_g = DRA_ORANGE_G, .fg_b = DRA_ORANGE_B };
    flare_style_set(s, HL_LITERAL_NUMBER, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_ORANGE_R, .fg_g = DRA_ORANGE_G, .fg_b = DRA_ORANGE_B };
    flare_style_set(s, HL_LITERAL_CHARACTER, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_ORANGE_R, .fg_g = DRA_ORANGE_G, .fg_b = DRA_ORANGE_B };
    flare_style_set(s, HL_LITERAL_BOOLEAN, &e);

    /* nil is absence: muted, and the faint attribute says so. */
    e = (FlareStyleEntry){ .fg_r = DRA_COMMENT_R, .fg_g = DRA_COMMENT_G, .fg_b = DRA_COMMENT_B, .faint = 1 };
    flare_style_set(s, HL_LITERAL_NIL, &e);

    /* --- Lisp: operators, punctuation, comments, errors --- */

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B };
    flare_style_set(s, HL_OPERATOR, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B };
    flare_style_set(s, HL_OPERATOR_QUOTE, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B };
    flare_style_set(s, HL_OPERATOR_BACKQUOTE, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B };
    flare_style_set(s, HL_OPERATOR_UNQUOTE, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B };
    flare_style_set(s, HL_OPERATOR_UNQUOTE_SPLICING, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_FG_R, .fg_g = DRA_FG_G, .fg_b = DRA_FG_B };
    flare_style_set(s, HL_PUNCTUATION, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_COMMENT_R, .fg_g = DRA_COMMENT_G, .fg_b = DRA_COMMENT_B, .italic = 1 };
    flare_style_set(s, HL_COMMENT, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_RED_R, .fg_g = DRA_RED_G, .fg_b = DRA_RED_B, .underline = 1 };
    flare_style_set(s, HL_ERROR, &e);

    /* --- Markdown/CommonMark block markup --- */

    e = (FlareStyleEntry){ .fg_r = DRA_COMMENT_R, .fg_g = DRA_COMMENT_G, .fg_b = DRA_COMMENT_B };
    flare_style_set(s, HL_MARKUP, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B, .bold = 1 };
    flare_style_set(s, HL_MARKUP_HEADING_MARKER, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_FG_R, .fg_g = DRA_FG_G, .fg_b = DRA_FG_B };
    flare_style_set(s, HL_MARKUP_PARAGRAPH, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B };
    flare_style_set(s, HL_MARKUP_THEMATIC_BREAK, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B };
    flare_style_set(s, HL_MARKUP_SETEXT_UNDERLINE, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_COMMENT_R, .fg_g = DRA_COMMENT_G, .fg_b = DRA_COMMENT_B, .italic = 1 };
    flare_style_set(s, HL_MARKUP_FENCED_OPEN, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_COMMENT_R, .fg_g = DRA_COMMENT_G, .fg_b = DRA_COMMENT_B, .italic = 1 };
    flare_style_set(s, HL_MARKUP_FENCED_INFO, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_YELLOW_R, .fg_g = DRA_YELLOW_G, .fg_b = DRA_YELLOW_B };
    flare_style_set(s, HL_MARKUP_INDENTED_CODE, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B };
    flare_style_set(s, HL_MARKUP_LIST_MARKER, &e);

    /* --- Markdown/CommonMark inline markup --- */

    e = (FlareStyleEntry){ .fg_r = DRA_PINK_R, .fg_g = DRA_PINK_G, .fg_b = DRA_PINK_B };
    flare_style_set(s, HL_MARKUP_INLINE, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_FG_R, .fg_g = DRA_FG_G, .fg_b = DRA_FG_B, .italic = 1 };
    flare_style_set(s, HL_MARKUP_INLINE_EMPHASIS, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_FG_R, .fg_g = DRA_FG_G, .fg_b = DRA_FG_B, .bold = 1 };
    flare_style_set(s, HL_MARKUP_INLINE_STRONG, &e);

    /* Inline code is a code span: the string role on the spec's Selection
     * token, so it reads as a span without leaving the palette. */
    e = (FlareStyleEntry){ .fg_r = DRA_YELLOW_R, .fg_g = DRA_YELLOW_G, .fg_b = DRA_YELLOW_B, .bg_r = DRA_SELECTION_R, .bg_g = DRA_SELECTION_G, .bg_b = DRA_SELECTION_B };
    flare_style_set(s, HL_MARKUP_INLINE_CODE, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PURPLE_R, .fg_g = DRA_PURPLE_G, .fg_b = DRA_PURPLE_B, .underline = 1 };
    flare_style_set(s, HL_MARKUP_INLINE_LINK, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PURPLE_R, .fg_g = DRA_PURPLE_G, .fg_b = DRA_PURPLE_B };
    flare_style_set(s, HL_MARKUP_INLINE_IMAGE, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_PURPLE_R, .fg_g = DRA_PURPLE_G, .fg_b = DRA_PURPLE_B };
    flare_style_set(s, HL_MARKUP_INLINE_AUTOLINK, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_COMMENT_R, .fg_g = DRA_COMMENT_G, .fg_b = DRA_COMMENT_B };
    flare_style_set(s, HL_MARKUP_INLINE_BREAK, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_YELLOW_R, .fg_g = DRA_YELLOW_G, .fg_b = DRA_YELLOW_B };
    flare_style_set(s, HL_MARKUP_INLINE_ESCAPE, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_YELLOW_R, .fg_g = DRA_YELLOW_G, .fg_b = DRA_YELLOW_B };
    flare_style_set(s, HL_MARKUP_INLINE_ENTITY, &e);

    e = (FlareStyleEntry){ .fg_r = DRA_COMMENT_R, .fg_g = DRA_COMMENT_G, .fg_b = DRA_COMMENT_B };
    flare_style_set(s, HL_MARKUP_INLINE_HTML, &e);

    return s;
}
