/* highlight.h - Public API for ditty flare syntax highlighting
 *
 * A Chroma-inspired, pure-C syntax highlighting library producing
 * ANSI terminal output at 16-color, 256-color, and truecolor depths.
 */

#ifndef DITTY_FLARE_HIGHLIGHT_H
#define DITTY_FLARE_HIGHLIGHT_H

#include "../lisp.h"
#include "flare_source.h"
#include "flare_token_source.h"
#include "flare_writer.h"
#include <stddef.h>
#include <stdint.h>

/* ----- Token type constants --------------------------------------------- */

typedef int FlareTokenType;

#define HL_TEXT                      0
#define HL_KEYWORD                   1000
#define HL_KEYWORD_SPECIAL_FORM      1010
#define HL_KEYWORD_DEFINE            1011
#define HL_KEYWORD_CONTROL           1012
#define HL_KEYWORD_MACRO_DEF         1013
#define HL_NAME                      2000
#define HL_NAME_FUNCTION             2010
#define HL_NAME_BUILTIN              2020
#define HL_NAME_VARIABLE             2030
#define HL_NAME_CONSTANT             2040
#define HL_NAME_KEYWORD_ARG          2050
#define HL_NAME_PACKAGE              2060
#define HL_LITERAL                   3000
#define HL_LITERAL_STRING            3010
#define HL_LITERAL_STRING_ESCAPE     3011
#define HL_LITERAL_NUMBER            3020
#define HL_LITERAL_CHARACTER         3030
#define HL_LITERAL_BOOLEAN           3040
#define HL_LITERAL_NIL               3050
#define HL_OPERATOR                  4000
#define HL_OPERATOR_QUOTE            4010
#define HL_OPERATOR_BACKQUOTE        4020
#define HL_OPERATOR_UNQUOTE          4030
#define HL_OPERATOR_UNQUOTE_SPLICING 4040
#define HL_PUNCTUATION               5000
#define HL_PUNCT_OPEN_PAREN          5010
#define HL_PUNCT_CLOSE_PAREN         5020
#define HL_PUNCT_DOT                 5030
#define HL_PUNCT_HASH                5040
#define HL_COMMENT                   6000
#define HL_COMMENT_LINE              6010
#define HL_ERROR                     7000
#define HL_ERROR_UNCLOSED_STRING     7010
#define HL_ERROR_UNCLOSED_PAREN      7020

/* ----- CommonMark / Markdown token types -------------------------------- */

/* Block structure (category 8000) */
#define HL_MARKUP                   8000
#define HL_MARKUP_HEADING           8010
#define HL_MARKUP_HEADING_MARKER    8011
#define HL_MARKUP_SETEXT_UNDERLINE  8020
#define HL_MARKUP_FENCED_OPEN       8030
#define HL_MARKUP_FENCED_INFO       8031
#define HL_MARKUP_FENCED_CLOSE      8032
#define HL_MARKUP_INDENTED_CODE     8040
#define HL_MARKUP_BLOCKQUOTE_MARKER 8050
#define HL_MARKUP_LIST_MARKER       8060
#define HL_MARKUP_THEMATIC_BREAK    8070
#define HL_MARKUP_HTML_BLOCK        8080
#define HL_MARKUP_LINKREF_DEF       8090
#define HL_MARKUP_PARAGRAPH         8100

/* Inline structure (category 9000) */
#define HL_MARKUP_INLINE          9000
#define HL_MARKUP_INLINE_EMPHASIS 9010
#define HL_MARKUP_INLINE_STRONG   9020
#define HL_MARKUP_INLINE_CODE     9030
#define HL_MARKUP_INLINE_LINK     9040
#define HL_MARKUP_INLINE_IMAGE    9050
#define HL_MARKUP_INLINE_AUTOLINK 9060
#define HL_MARKUP_INLINE_BREAK    9070
#define HL_MARKUP_INLINE_ESCAPE   9080
#define HL_MARKUP_INLINE_ENTITY   9090
#define HL_MARKUP_INLINE_HTML     9100

/* ----- Token type helpers ----------------------------------------------- */

/* Return the category (range base) for a token type:
 *   0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000 */
int flare_token_category(FlareTokenType type);

/* Return the subcategory for a token type:
 *   e.g. HL_KEYWORD_SPECIAL_FORM → 1010 */
int flare_token_subcategory(FlareTokenType type);

/* ----- Core types ------------------------------------------------------- */

/* Token produced by the lexer.
 * In v3.0, tokens own their text. text points to GC-managed memory
 * that remains valid until the source that produced it is freed. */
typedef struct FlareToken FlareToken;
struct FlareToken
{
    FlareTokenType type;
    union
    {
        char *text;    /* owned, GC-allocated token text (v3.0) */
        size_t offset; /* byte offset into source text (v1.0 compat) */
    };
    size_t length; /* byte length of the token text */
};

/* Styling for a token type */
typedef struct
{
    uint8_t fg_r, fg_g, fg_b;
    uint8_t bg_r, bg_g, bg_b;
    int bold, italic, underline, faint, strikethrough;
    int margin_top;    /* blank lines before a block-level token */
    int margin_bottom; /* blank lines after a block-level token */
} FlareStyleEntry;

/* A complete style (collection of type → entry mappings) */
typedef struct FlareStyle FlareStyle;

/* A formatter (produces output from token stream + style) */
typedef struct FlareFormatter FlareFormatter;

/* Result of highlighting: ANSI-escaped string */
typedef struct
{
    char *data;    /* NUL-terminated ANSI string */
    size_t length; /* byte length (not counting NUL) */
} FlareResult;

/* ----- Lexer API -------------------------------------------------------- */

/* Create a streaming Ditty Lisp lexer that reads from src.
 * Requires a non-NULL Environment* (obtained from lisp_init()) for
 * semantic classification of symbols as special forms, builtins,
 * macros, or variables. Passing NULL returns NULL.
 * The returned FlareTokenSource owns src. */
FlareTokenSource *flare_lexer_ditty(FlareSource *src, Environment *env);

/* Create a streaming CommonMark/Markdown lexer that reads from src.
 * Requires a non-NULL Environment* for sub-lexing fenced code blocks
 * with the ditty lexer. Passing NULL returns NULL.
 * The returned FlareTokenSource owns src. */
FlareTokenSource *flare_lexer_commonmark(FlareSource *src, Environment *env);

/* ----- Style API ------------------------------------------------------- */

/* Built-in styles */
FlareStyle *flare_style_monokai(void);
FlareStyle *flare_style_dracula(void);
FlareStyle *flare_style_github_dark(void);
FlareStyle *flare_style_github_light(void);

/* Look up style entry for a token type (walks hierarchy) */
FlareStyleEntry flare_style_get(const FlareStyle *style, FlareTokenType type);

/* Custom style building */
FlareStyle *flare_style_new(void);
void flare_style_set(FlareStyle *style, FlareTokenType type,
                     const FlareStyleEntry *entry);

void flare_style_free(FlareStyle *style);

/* ----- Reflow options -------------------------------------------------- */

/* Reflow options for terminal formatter */
typedef struct FlareReflowOptions FlareReflowOptions;
struct FlareReflowOptions
{
    int width;                /* Target width (default 76, 0 = no reflow) */
    int preserve_paragraphs;  /* Keep paragraph breaks (default 1) */
    int preserve_code;        /* Don't reflow fenced code (default 1) */
    int preserve_headings;    /* Don't reflow headings (default 1) */
    int preserve_hard_breaks; /* Preserve hard line breaks (default 1) */
};

/* Default reflow options (width=76, all preservation enabled) */
#define FLARE_REFLOW_DEFAULT ((FlareReflowOptions){ \
    .width = 76,                                    \
    .preserve_paragraphs = 1,                       \
    .preserve_code = 1,                             \
    .preserve_headings = 1,                         \
    .preserve_hard_breaks = 1 })

/* ----- Formatter API --------------------------------------------------- */

/* Color depth for formatters */
typedef enum
{
    BFLARE_COLOR_8,        /* ANSI 8-color (SGR 30-37, bold for bright) */
    BFLARE_COLOR_16,       /* ANSI 16-color (SGR 30-37, 90-97) */
    BFLARE_COLOR_256,      /* xterm-256 (SGR 38;5;N) */
    BFLARE_COLOR_TRUECOLOR /* 24-bit RGB (SGR 38;2;R;G;B) */
} FlareColorDepth;

/* Create a terminal formatter at the given color depth.
 * `reflow` is kept for API compatibility; the formatter stores it but
 * the new pull-based pipeline receives options via the caller. */
FlareFormatter *flare_formatter_terminal(FlareColorDepth depth, FlareWriter *writer, FlareStyle *style);
FlareFormatter *flare_formatter_terminal_ex(FlareColorDepth depth, FlareWriter *writer, FlareStyle *style,
                                            const FlareReflowOptions *reflow);

void flare_formatter_free(FlareFormatter *formatter);

/* Format tokens from a FlareTokenSource using the configured formatter.
 * Returns 0 on success, -1 on error. */
int flare_formatter_format(FlareFormatter *fmt, FlareTokenSource *src);

/* Format tokens to ANSI with explicit hyperlink control.
 * When enable_hyperlinks is 1, inline links and autolinks emit OSC 8
 * escape sequences.  When 0, links are styled without hyperlinks.
 * Tokens must use the text-based representation (text/length). */
char *flare_format_terminal(const FlareToken *tokens, size_t count,
                            const FlareStyle *style, FlareColorDepth depth,
                            int enable_hyperlinks);

/* Format tokens with reflow.
 * NULL reflow = source-faithful layout (no reflow). */
char *flare_format_terminal_reflow(const FlareToken *tokens, size_t count,
                                   const FlareStyle *style,
                                   FlareColorDepth depth, int enable_hyperlinks,
                                   const FlareReflowOptions *reflow);

/* ----- Color conversion API -------------------------------------------- */

/* Convert 24-bit RGB to closest 256-color palette index (0-255) */
int flare_color_rgb_to_256(int r, int g, int b);

/* Convert 24-bit RGB to closest 8-color ANSI index (0-7) */
int flare_color_rgb_to_8(int r, int g, int b);

/* Convert 24-bit RGB to closest 16-color ANSI index (0-15) via nearest-match */
int flare_color_rgb_to_16(int r, int g, int b);

/* Reverse mapping: 256-color palette index to RGB */
void flare_color_256_to_rgb(int idx, uint8_t *r, uint8_t *g, uint8_t *b);

/* ----- Terminal capability detection ------------------------------------ */

/* Detect whether the terminal supports OSC 8 hyperlinks.
 * Checks isatty(), FORCE_HYPERLINK, and known terminal environment vars.
 * Returns 1 if hyperlinks are supported, 0 otherwise. */
int flare_terminal_supports_hyperlinks(void);

/* Detect hyperlink support from environment variables only (no isatty check).
 * Useful for testing the env-var cascade in isolation. */
int flare_terminal_hyperlinks_env(void);

/* ----- One-shot highlight API ------------------------------------------ */

/* Highlight source text in one call.
 *
 * style and formatter are optional (NULL = defaults: monokai style,
 * truecolor terminal formatter).
 *
 * Returns a FlareResult with a malloc'd ANSI string.
 * Caller frees with flare_result_free(). */
FlareResult flare_highlight(const char *input, size_t input_len,
                            const FlareStyle *style,
                            const FlareFormatter *formatter);

void flare_result_free(FlareResult result);

#endif /* DITTY_FLARE_HIGHLIGHT_H */
