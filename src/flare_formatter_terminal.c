/* formatter_terminal.c - ANSI terminal formatter */

#include "../include/ditty/highlight.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----- Hyperlink helpers ----------------------------------------------- */

/* Ensure buffer has room for `need` bytes beyond `pos`. */
static void buf_ensure(char **buf, size_t *cap, size_t pos, size_t need)
{
    while (pos + need > *cap) {
        *cap *= 2;
        *buf = realloc(*buf, *cap);
    }
}

/* Copy a URI into `dst` (at most dst_size-1 bytes + NUL).
 * Returns the copied length, or 0 if nothing left. */
static size_t copy_uri(const char *src, size_t len, char *dst, size_t dst_size)
{
    if (len >= dst_size)
        len = dst_size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return len;
}

/* Extract URI from an inline link token `[text](url)` or `[text](url "title")`.
 * Writes the URI into `uri_buf` (at most uri_buf_size-1 bytes + NUL).
 * Returns the URI length, or 0 if extraction fails. */
static size_t extract_inline_link_uri(const char *text, size_t len,
                                      char *uri_buf, size_t uri_buf_size)
{
    /* Find the `]` that closes the link text */
    size_t pos = 0;
    int depth = 0;
    while (pos < len) {
        if (text[pos] == '[')
            depth++;
        else if (text[pos] == ']') {
            depth--;
            if (depth == 0)
                break;
        }
        pos++;
    }
    if (pos >= len || depth != 0)
        return 0;

    /* Skip `]` */
    pos++;

    /* Check for `(` — inline link */
    if (pos >= len || text[pos] != '(')
        return 0;
    pos++; /* skip `(` */

    /* Find matching `)` */
    size_t url_start = pos;
    int paren_depth = 1;
    while (pos < len) {
        if (text[pos] == '(')
            paren_depth++;
        else if (text[pos] == ')') {
            paren_depth--;
            if (paren_depth == 0)
                break;
        }
        pos++;
    }
    if (pos >= len || paren_depth != 0)
        return 0;

    size_t url_len = pos - url_start;

    /* Strip optional title: `url "title"` or `url 'title'`.
     * The title is preceded by whitespace. */
    size_t end = url_start + url_len;
    for (size_t k = url_start; k < end; k++) {
        if (text[k] == ' ' || text[k] == '\t') {
            /* Found whitespace — everything before is the URL */
            url_len = k - url_start;
            break;
        }
    }

    /* Trim trailing whitespace from URL */
    while (url_len > 0 && (text[url_start + url_len - 1] == ' ' ||
                           text[url_start + url_len - 1] == '\t'))
        url_len--;

    if (url_len == 0)
        return 0;

    return copy_uri(text + url_start, url_len, uri_buf, uri_buf_size);
}

/* Extract URI from an autolink token `<url>`.
 * Writes the URI into `uri_buf` (at most uri_buf_size-1 bytes + NUL).
 * Returns the URI length, or 0 if extraction fails. */
static size_t extract_autolink_uri(const char *text, size_t len,
                                   char *uri_buf, size_t uri_buf_size)
{
    /* Token is `<url>` — find `<` and `>` */
    if (len < 2 || text[0] != '<')
        return 0;

    size_t end = 0;
    for (size_t i = 1; i < len; i++) {
        if (text[i] == '>') {
            end = i;
            break;
        }
    }
    if (end == 0)
        return 0;

    size_t uri_len = end - 1;
    if (uri_len == 0)
        return 0;

    return copy_uri(text + 1, uri_len, uri_buf, uri_buf_size);
}

/* Try to extract a hyperlink URI from a token.
 * Returns the URI length (written to uri_buf), or 0 if no valid URI. */
static size_t extract_hyperlink_uri(FlareTokenType type,
                                    const char *text, size_t length,
                                    char *uri_buf, size_t uri_buf_size)
{
    size_t uri_len = 0;

    if (type == HL_MARKUP_INLINE_LINK)
        uri_len = extract_inline_link_uri(text, length, uri_buf, uri_buf_size);
    else if (type == HL_MARKUP_INLINE_AUTOLINK)
        uri_len = extract_autolink_uri(text, length, uri_buf, uri_buf_size);

    if (uri_len == 0)
        return 0;

    return uri_len;
}

/* Emit an OSC 8 hyperlink opening sequence into the output buffer.
 * Format: ESC ] 8 ; params ; URI BEL */
static void emit_osc8_open(char **out, size_t *cap, size_t *pos,
                           const char *uri, size_t uri_len)
{
    /* \x1b]8;;<uri>\x07 */
    buf_ensure(out, cap, *pos, uri_len + 8);
    (*out)[(*pos)++] = '\x1b';
    (*out)[(*pos)++] = ']';
    (*out)[(*pos)++] = '8';
    (*out)[(*pos)++] = ';';
    (*out)[(*pos)++] = ';';
    memcpy(*out + *pos, uri, uri_len);
    *pos += uri_len;
    (*out)[(*pos)++] = '\x07';
}

/* Emit an OSC 8 hyperlink closing sequence into the output buffer.
 * Format: ESC ] 8 ; ; BEL */
static void emit_osc8_close(char **out, size_t *cap, size_t *pos)
{
    /* \x1b]8;;\x07 */
    buf_ensure(out, cap, *pos, 7);
    (*out)[(*pos)++] = '\x1b';
    (*out)[(*pos)++] = ']';
    (*out)[(*pos)++] = '8';
    (*out)[(*pos)++] = ';';
    (*out)[(*pos)++] = ';';
    (*out)[(*pos)++] = '\x07';
}

/* Format an SGR fg color sequence into buf. Returns bytes written. */
static int format_fg_color(char *buf, size_t bufsize, const FlareStyleEntry *entry,
                           FlareColorDepth depth)
{
    if (depth == BFLARE_COLOR_TRUECOLOR) {
        return snprintf(buf, bufsize, "\033[38;2;%d;%d;%dm",
                        entry->fg_r, entry->fg_g, entry->fg_b);
    }
    if (depth == BFLARE_COLOR_256) {
        int idx = flare_color_rgb_to_256(entry->fg_r, entry->fg_g, entry->fg_b);
        return snprintf(buf, bufsize, "\033[38;5;%dm", idx);
    }
    /* 16-color: aixterm bright codes 90-97 */
    if (depth == BFLARE_COLOR_16) {
        int idx = flare_color_rgb_to_16(entry->fg_r, entry->fg_g, entry->fg_b);
        if (idx >= 8)
            return snprintf(buf, bufsize, "\033[%dm", 90 + (idx - 8));
        return snprintf(buf, bufsize, "\033[%dm", 30 + idx);
    }
    /* 8-color: bold prefix for bright */
    int idx = flare_color_rgb_to_8(entry->fg_r, entry->fg_g, entry->fg_b);
    if (idx >= 8)
        return snprintf(buf, bufsize, "\033[1m\033[%dm", 30 + (idx - 8));
    return snprintf(buf, bufsize, "\033[%dm", 30 + idx);
}

/* Check if two style entries are visually identical */
static int style_entries_equal(const FlareStyleEntry *a, const FlareStyleEntry *b)
{
    return a->fg_r == b->fg_r && a->fg_g == b->fg_g && a->fg_b == b->fg_b &&
           a->bg_r == b->bg_r && a->bg_g == b->bg_g && a->bg_b == b->bg_b &&
           a->bold == b->bold && a->italic == b->italic &&
           a->underline == b->underline && a->faint == b->faint &&
           a->strikethrough == b->strikethrough;
}

/* Build a full SGR sequence for a style entry.
 * Format: ESC [ 0 ; attributes ; colors m
 * Returns bytes written to buf. */
static int build_sgr(const FlareStyleEntry *entry, FlareColorDepth depth, char *buf, size_t bufsize)
{
    int pos = 0;
    int need_semi = 0;

    buf[pos++] = '\033';
    buf[pos++] = '[';
    buf[pos++] = '0';
    need_semi = 1;

    if (entry->bold) {
        if (need_semi)
            buf[pos++] = ';';
        pos += snprintf(buf + pos, bufsize - pos, "1");
        need_semi = 1;
    }
    if (entry->faint) {
        if (need_semi)
            buf[pos++] = ';';
        pos += snprintf(buf + pos, bufsize - pos, "2");
        need_semi = 1;
    }
    if (entry->italic) {
        if (need_semi)
            buf[pos++] = ';';
        pos += snprintf(buf + pos, bufsize - pos, "3");
        need_semi = 1;
    }
    if (entry->underline) {
        if (need_semi)
            buf[pos++] = ';';
        pos += snprintf(buf + pos, bufsize - pos, "4");
        need_semi = 1;
    }
    if (entry->strikethrough) {
        if (need_semi)
            buf[pos++] = ';';
        pos += snprintf(buf + pos, bufsize - pos, "9");
        need_semi = 1;
    }

    /* Foreground color */
    char fgbuf[32];
    int fglen;
    if (depth == BFLARE_COLOR_TRUECOLOR) {
        if (need_semi)
            buf[pos++] = ';';
        fglen = snprintf(fgbuf, sizeof(fgbuf), "38;2;%d;%d;%d",
                         entry->fg_r, entry->fg_g, entry->fg_b);
        memcpy(buf + pos, fgbuf, fglen);
        pos += fglen;
        need_semi = 1;
    } else if (depth == BFLARE_COLOR_256) {
        if (need_semi)
            buf[pos++] = ';';
        int idx = flare_color_rgb_to_256(entry->fg_r, entry->fg_g, entry->fg_b);
        fglen = snprintf(fgbuf, sizeof(fgbuf), "38;5;%d", idx);
        memcpy(buf + pos, fgbuf, fglen);
        pos += fglen;
        need_semi = 1;
    } else if (depth == BFLARE_COLOR_16) {
        int idx = flare_color_rgb_to_16(entry->fg_r, entry->fg_g, entry->fg_b);
        if (need_semi)
            buf[pos++] = ';';
        if (idx >= 8) {
            fglen = snprintf(fgbuf, sizeof(fgbuf), "%d", 90 + (idx - 8));
        } else {
            fglen = snprintf(fgbuf, sizeof(fgbuf), "%d", 30 + idx);
        }
        memcpy(buf + pos, fgbuf, fglen);
        pos += fglen;
        need_semi = 1;
    } else {
        int idx = flare_color_rgb_to_8(entry->fg_r, entry->fg_g, entry->fg_b);
        if (idx >= 8) {
            if (!entry->bold) {
                if (need_semi)
                    buf[pos++] = ';';
                fglen = snprintf(fgbuf, sizeof(fgbuf), "1;%d", 30 + (idx - 8));
            } else {
                if (need_semi)
                    buf[pos++] = ';';
                fglen = snprintf(fgbuf, sizeof(fgbuf), "%d", 30 + (idx - 8));
            }
            memcpy(buf + pos, fgbuf, fglen);
            pos += fglen;
            need_semi = 1;
        } else {
            if (need_semi)
                buf[pos++] = ';';
            fglen = snprintf(fgbuf, sizeof(fgbuf), "%d", 30 + idx);
            memcpy(buf + pos, fgbuf, fglen);
            pos += fglen;
            need_semi = 1;
        }
    }

    /* Background color */
    if (entry->bg_r || entry->bg_g || entry->bg_b) {
        char bgbuf[32];
        int bglen;
        if (depth == BFLARE_COLOR_TRUECOLOR) {
            if (need_semi)
                buf[pos++] = ';';
            bglen = snprintf(bgbuf, sizeof(bgbuf), "48;2;%d;%d;%d",
                             entry->bg_r, entry->bg_g, entry->bg_b);
            memcpy(buf + pos, bgbuf, bglen);
            pos += bglen;
            need_semi = 1;
        } else if (depth == BFLARE_COLOR_256) {
            if (need_semi)
                buf[pos++] = ';';
            int bidx = flare_color_rgb_to_256(entry->bg_r, entry->bg_g, entry->bg_b);
            bglen = snprintf(bgbuf, sizeof(bgbuf), "48;5;%d", bidx);
            memcpy(buf + pos, bgbuf, bglen);
            pos += bglen;
            need_semi = 1;
        } else if (depth == BFLARE_COLOR_16) {
            if (need_semi)
                buf[pos++] = ';';
            int bidx = flare_color_rgb_to_16(entry->bg_r, entry->bg_g, entry->bg_b);
            if (bidx >= 8) {
                bglen = snprintf(bgbuf, sizeof(bgbuf), "%d", 100 + (bidx - 8));
            } else {
                bglen = snprintf(bgbuf, sizeof(bgbuf), "%d", 40 + bidx);
            }
            memcpy(buf + pos, bgbuf, bglen);
            pos += bglen;
            need_semi = 1;
        } else {
            if (need_semi)
                buf[pos++] = ';';
            int bidx = flare_color_rgb_to_8(entry->bg_r, entry->bg_g, entry->bg_b);
            bglen = snprintf(bgbuf, sizeof(bgbuf), "%d", 40 + bidx);
            memcpy(buf + pos, bgbuf, bglen);
            pos += bglen;
            need_semi = 1;
        }
    }

    buf[pos++] = 'm';
    buf[pos] = '\0';
    return pos;
}

/* Check if a token type is a fenced code block structural marker that
 * should be suppressed from rendered output.  The fence delimiters
 * (``` or ~~~) and info strings are structural, not visual content. */
static int is_fenced_marker(FlareTokenType type)
{
    return type == HL_MARKUP_FENCED_OPEN || type == HL_MARKUP_FENCED_INFO ||
           type == HL_MARKUP_FENCED_CLOSE;
}

/* Copy token text to the output buffer, tracking line boundaries.
 * When `indent > 0`, prepend `indent` spaces at the start of each line
 * (after every \n). When `sgr` is non-NULL, re-emit the style after each
 * newline to maintain style continuity across line breaks. Returns the
 * new output position. */
static size_t emit_token_text(char **out, size_t *cap, size_t pos,
                              const char *text, size_t length,
                              int indent, const char *sgr,
                              int *at_line_start)
{
    const char *src = text;
    size_t sgr_len = sgr ? strlen(sgr) : 0;

    if (indent == 0 && sgr == NULL) {
        buf_ensure(out, cap, pos, length + 5);
        memcpy(*out + pos, src, length);
        pos += length;
    } else {
        size_t src_pos = 0;
        while (src_pos < length) {
            const char *nl = memchr(src + src_pos, '\n', length - src_pos);
            if (!nl) {
                size_t chunk = length - src_pos;
                buf_ensure(out, cap, pos, chunk + indent + sgr_len + 5);
                memcpy(*out + pos, src + src_pos, chunk);
                pos += chunk;
                break;
            }
            size_t chunk = (size_t)(nl - (src + src_pos)) + 1;
            buf_ensure(out, cap, pos, chunk + indent + sgr_len + 5);
            memcpy(*out + pos, src + src_pos, chunk);
            pos += chunk;
            src_pos += chunk;
            /* After newline: emit indentation and/or re-apply style.
             * Skip if this is the very end of the token — the next
             * token will handle it via the at_line_start mechanism. */
            if (src_pos < length) {
                if (indent > 0) {
                    buf_ensure(out, cap, pos, indent);
                    for (int s = 0; s < indent; s++) {
                        (*out)[pos++] = ' ';
                    }
                }
                if (sgr && sgr_len > 0) {
                    buf_ensure(out, cap, pos, sgr_len);
                    memcpy(*out + pos, sgr, sgr_len);
                    pos += sgr_len;
                }
            }
        }
    }

    if (length > 0 && src[length - 1] == '\n')
        *at_line_start = 1;
    else if (length > 0)
        *at_line_start = 0;
    return pos;
}

/* Emit inline code text with a space before and after for padding.
 * The space is included in the styled region so the background color
 * extends around the code. */
static size_t emit_inline_code_text(char **out, size_t *cap, size_t pos,
                                    const char *text, size_t length,
                                    const char *sgr,
                                    int *at_line_start)
{
    /* Leading space for padding */
    buf_ensure(out, cap, pos, 1);
    (*out)[pos++] = ' ';

    /* Content */
    if (length > 0)
        pos = emit_token_text(out, cap, pos, text, length, 0, sgr, at_line_start);

    /* Trailing space for padding */
    buf_ensure(out, cap, pos, 1);
    (*out)[pos++] = ' ';

    *at_line_start = 0;
    return pos;
}

/* Emit only the title text from an inline link token [text](url),
 * suppressing the URL and all syntax characters. */
static size_t emit_link_text(char **out, size_t *cap, size_t pos,
                             const char *text, size_t length,
                             const char *sgr,
                             int *at_line_start)
{
    if (length < 2 || text[0] != '[')
        return emit_token_text(out, cap, pos, text, length, 0, sgr,
                               at_line_start);

    /* Find matching `]` with nesting support */
    size_t p = 1;
    int depth = 1;
    while (p < length) {
        if (text[p] == '[')
            depth++;
        else if (text[p] == ']') {
            depth--;
            if (depth == 0)
                break;
        }
        p++;
    }
    if (depth != 0)
        return emit_token_text(out, cap, pos, text, length, 0, sgr,
                               at_line_start);

    /* Title text is between offset+1 and offset+p */
    size_t title_off = 1;
    size_t title_len = p - 1;
    if (title_len > 0) {
        buf_ensure(out, cap, pos, title_len + 1);
        memcpy(*out + pos, text + title_off, title_len);
        pos += title_len;
    }

    *at_line_start = 0;
    return pos;
}

/* Emit autolink text <url> with angle brackets stripped. */
static size_t emit_autolink_text(char **out, size_t *cap, size_t pos,
                                 const char *text, size_t length,
                                 const char *sgr,
                                 int *at_line_start)
{
    if (length < 2 || text[0] != '<')
        return emit_token_text(out, cap, pos, text, length, 0, sgr,
                               at_line_start);

    /* Find closing `>` */
    size_t end = 0;
    for (size_t i = 1; i < length; i++) {
        if (text[i] == '>') {
            end = i;
            break;
        }
    }
    if (end == 0)
        return emit_token_text(out, cap, pos, text, length, 0, sgr,
                               at_line_start);

    size_t uri_off = 1;
    size_t uri_len = end - 1;
    if (uri_len > 0) {
        buf_ensure(out, cap, pos, uri_len + 1);
        memcpy(*out + pos, text + uri_off, uri_len);
        pos += uri_len;
    }

    *at_line_start = 0;
    return pos;
}

/* Format token stream into an ANSI string.
 * When enable_hyperlinks is 1, inline links and autolinks emit OSC 8
 * escape sequences for clickable hyperlinks on supporting terminals. */
char *flare_format_terminal(const FlareToken *tokens, size_t count,
                            const FlareStyle *style, FlareColorDepth depth,
                            int enable_hyperlinks)
{
    FlareTerminalStyle ts = FLARE_TERMINAL_STYLE_DEFAULT;
    ts.enable_hyperlinks = enable_hyperlinks;
    return flare_format_terminal_with_style(tokens, count, style, depth, enable_hyperlinks, &ts);
}

char *flare_format_terminal_with_style(const FlareToken *tokens, size_t count,
                                       const FlareStyle *style, FlareColorDepth depth,
                                       int enable_hyperlinks,
                                       const FlareTerminalStyle *term_style)
{
    if (!tokens || count == 0 || !style) {
        char *empty = malloc(5);
        if (empty)
            memcpy(empty, "\033[0m", 4), empty[4] = '\0';
        return empty;
    }

    FlareTerminalStyle ts = term_style ? *term_style : FLARE_TERMINAL_STYLE_DEFAULT;

    /* Grow-only buffer */
    size_t cap = 256;
    char *out = malloc(cap);
    if (!out)
        return NULL;
    size_t pos = 0;

    FlareStyleEntry prev = { 0 };
    int prev_valid = 0;
    int in_fenced = 0;
    int at_line_start = 1; /* track line boundaries across tokens */
    int bold_depth = 0;    /* nested bold tracking */
    int italic_depth = 0;  /* nested italic tracking */

    /* Block spacing state */
    int prev_margin_bottom = 0;
    int first_block = 1;

    for (size_t i = 0; i < count; i++) {
        /* Block-level spacing. Tokens that begin a new block can request
         * blank lines above/below via their terminal style margins. Do this
         * before fenced-marker skipping so HL_MARKUP_FENCED_OPEN can
         * request a leading blank line. */
        if (tokens[i].type == HL_MARKUP_HEADING_MARKER ||
            tokens[i].type == HL_MARKUP_PARAGRAPH ||
            tokens[i].type == HL_MARKUP_THEMATIC_BREAK ||
            tokens[i].type == HL_MARKUP_FENCED_OPEN) {
            int margin_top = 0, margin_bottom = 0;
            switch (tokens[i].type) {
            case HL_MARKUP_HEADING_MARKER:
                margin_top = ts.heading_margin_top;
                margin_bottom = ts.heading_margin_bottom;
                break;
            case HL_MARKUP_PARAGRAPH:
                margin_top = 0;
                margin_bottom = ts.paragraph_margin_bottom;
                break;
            case HL_MARKUP_THEMATIC_BREAK:
                margin_top = ts.thematic_break_margin;
                margin_bottom = ts.thematic_break_margin;
                break;
            case HL_MARKUP_FENCED_OPEN:
                margin_top = ts.fenced_margin_top;
                margin_bottom = ts.fenced_margin_bottom;
                break;
            default:
                break;
            }
            if (!first_block) {
                int spacing = prev_margin_bottom > margin_top ? prev_margin_bottom : margin_top;
                for (int s = 0; s < spacing; s++) {
                    buf_ensure(&out, &cap, pos, 1);
                    out[pos++] = '\n';
                }
            }
            prev_margin_bottom = margin_bottom;
            first_block = 0;
            if (tokens[i].type == HL_MARKUP_PARAGRAPH)
                continue;
            if (tokens[i].type == HL_MARKUP_FENCED_OPEN) {
                in_fenced = 1;
                at_line_start = 1;
                /* Consume trailing \n that ends the fence line */
                if (i + 1 < count && tokens[i + 1].type == HL_TEXT &&
                    tokens[i + 1].length == 1 &&
                    tokens[i + 1].text[0] == '\n')
                    i++;
                prev_valid = 0;
                continue;
            }
        }

        /* Skip remaining fenced code block structural markers — they are not
         * visual content. When skipping, also consume the line-ending
         * newline that belongs to the fence line:
         *   - \n immediately after FENCED_OPEN/INFO: ends the opening
         *     fence line (not a paragraph separator)
         *   - \n immediately before FENCED_CLOSE: ends the last code
         *     content line (not a paragraph separator)
         * The \n BEFORE FENCED_OPEN and AFTER FENCED_CLOSE are
         * paragraph separators and must be kept. */
        if (is_fenced_marker(tokens[i].type)) {
            if (tokens[i].type == HL_MARKUP_FENCED_INFO) {
                in_fenced = 1;
                at_line_start = 1;
                /* Consume trailing \n that ends the fence line */
                if (i + 1 < count && tokens[i + 1].type == HL_TEXT &&
                    tokens[i + 1].length == 1 &&
                    tokens[i + 1].text[0] == '\n')
                    i++;
            } else if (tokens[i].type == HL_MARKUP_FENCED_CLOSE) {
                in_fenced = 0;
            }
            prev_valid = 0;
            continue;
        }
        if (tokens[i].type == HL_TEXT && tokens[i].length == 1 &&
            tokens[i].text[0] == '\n' &&
            i + 1 < count && tokens[i + 1].type == HL_MARKUP_FENCED_CLOSE) {
            /* \n before closing fence: part of the fence block, not
             * a paragraph separator — suppress.  Also clear
             * in_fenced so subsequent content is not indented. */
            in_fenced = 0;
            prev_valid = 0;
            continue;
        }

        /* Emit indentation at the start of each line inside
         * a fenced code block, before the first styled token. */
        if (in_fenced && at_line_start && ts.fenced_indent > 0) {
            buf_ensure(&out, &cap, pos, ts.fenced_indent);
            for (int s = 0; s < ts.fenced_indent; s++) {
                out[pos++] = ' ';
            }
            at_line_start = 0;
        }

        /* Handle emphasis/strong delimiter tokens: toggle state and skip.
         * The lexer emits delimiters separately from content, so we track
         * depth to apply bold/italic to the TEXT tokens between them.
         * STRONG with length >= 3 means both bold AND italic (***text***). */
        if (tokens[i].type == HL_MARKUP_INLINE_STRONG) {
            if (tokens[i].length >= 3) {
                /* *** toggles both bold and italic */
                bold_depth = !bold_depth;
                italic_depth = !italic_depth;
            } else {
                /* ** toggles bold only */
                bold_depth = !bold_depth;
            }
            prev_valid = 0; /* Force style recompute for next token */
            continue;
        }
        if (tokens[i].type == HL_MARKUP_INLINE_EMPHASIS) {
            /* * or _ toggles italic */
            italic_depth = !italic_depth;
            prev_valid = 0;
            continue;
        }

        FlareStyleEntry entry = flare_style_get(style, tokens[i].type);

        /* Apply bold/italic from delimiter tracking (overriding style defaults) */
        if (bold_depth)
            entry.bold = 1;
        if (italic_depth)
            entry.italic = 1;

        /* Try to extract a hyperlink URI for link/autolink tokens */
        char uri_buf[1024];
        size_t uri_len = 0;
        int has_hyperlink = 0;
        if (ts.enable_hyperlinks &&
            (tokens[i].type == HL_MARKUP_INLINE_LINK ||
             tokens[i].type == HL_MARKUP_INLINE_AUTOLINK)) {
            uri_len = extract_hyperlink_uri(tokens[i].type, tokens[i].text, tokens[i].length,
                                            uri_buf, sizeof(uri_buf));
            has_hyperlink = (uri_len > 0);
        }

        /* Coalesce: if same style as previous, skip the escape */
        if (prev_valid && style_entries_equal(&prev, &entry)) {
            /* Build SGR from prev for style continuity across newlines */
            char prev_sgr[128];
            int prev_sgr_len = build_sgr(&prev, depth, prev_sgr, sizeof(prev_sgr));
            (void)prev_sgr_len; /* Used for memcpy below */

            /* Emit OSC 8 open if this token has a hyperlink */
            if (has_hyperlink)
                emit_osc8_open(&out, &cap, &pos, uri_buf, uri_len);

            /* Just copy the text (inline code: backticks → spaces;
             * links/autolinks: title-only rendering) */
            size_t tlen = tokens[i].length;
            if (tokens[i].type == HL_MARKUP_INLINE_CODE)
                pos = emit_inline_code_text(&out, &cap, pos, tokens[i].text, tlen,
                                            prev_sgr, &at_line_start);
            else if (tokens[i].type == HL_MARKUP_INLINE_LINK)
                pos = emit_link_text(&out, &cap, pos, tokens[i].text, tlen,
                                     prev_sgr, &at_line_start);
            else if (tokens[i].type == HL_MARKUP_INLINE_AUTOLINK)
                pos = emit_autolink_text(&out, &cap, pos, tokens[i].text, tlen,
                                         prev_sgr, &at_line_start);
            else
                pos = emit_token_text(&out, &cap, pos, tokens[i].text, tlen,
                                      in_fenced ? ts.fenced_indent : 0,
                                      prev_sgr, &at_line_start);

            if (has_hyperlink)
                emit_osc8_close(&out, &cap, &pos);
            continue;
        }

        /* Build SGR sequence for this style. */
        char sgr[128];
        int sgrpos = build_sgr(&entry, depth, sgr, sizeof(sgr));

        /* Write SGR + token text.
         * If this token has a hyperlink, wrap it in OSC 8 sequences:
         *   open URI ... SGR ... text ... close */
        size_t tlen = tokens[i].length;
        if (has_hyperlink)
            emit_osc8_open(&out, &cap, &pos, uri_buf, uri_len);

        buf_ensure(&out, &cap, pos, sgrpos + tlen + 5);

        memcpy(out + pos, sgr, sgrpos);
        pos += sgrpos;
        if (tokens[i].type == HL_MARKUP_INLINE_CODE)
            pos = emit_inline_code_text(&out, &cap, pos, tokens[i].text, tlen,
                                        sgr, &at_line_start);
        else if (tokens[i].type == HL_MARKUP_INLINE_LINK)
            pos = emit_link_text(&out, &cap, pos, tokens[i].text, tlen,
                                 sgr, &at_line_start);
        else if (tokens[i].type == HL_MARKUP_INLINE_AUTOLINK)
            pos = emit_autolink_text(&out, &cap, pos, tokens[i].text, tlen,
                                     sgr, &at_line_start);
        else
            pos = emit_token_text(&out, &cap, pos, tokens[i].text, tlen,
                                  in_fenced ? ts.fenced_indent : 0,
                                  sgr, &at_line_start);

        if (has_hyperlink)
            emit_osc8_close(&out, &cap, &pos);

        prev = entry;
        prev_valid = 1;
    }

    /* Trailing SGR reset */
    while (pos + 5 > cap) {
        cap *= 2;
        out = realloc(out, cap);
    }
    memcpy(out + pos, "\033[0m", 4);
    pos += 4;
    out[pos] = '\0';

    return out;
}

char *flare_format_terminal_reflow(const FlareToken *tokens, size_t count,
                                   const FlareStyle *style,
                                   FlareColorDepth depth, int enable_hyperlinks,
                                   const FlareReflowOptions *reflow)
{
    (void)reflow;
    return flare_format_terminal(tokens, count, style, depth, enable_hyperlinks);
}

char *flare_format_terminal_reflow_ex(const FlareToken *tokens, size_t count,
                                      const FlareStyle *style,
                                      FlareColorDepth depth, int enable_hyperlinks,
                                      const FlareTerminalStyle *term_style,
                                      const FlareReflowOptions *reflow)
{
    (void)reflow;
    return flare_format_terminal_with_style(tokens, count, style, depth, enable_hyperlinks, term_style);
}
