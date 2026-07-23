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
                                    const char *input, size_t offset, size_t length,
                                    char *uri_buf, size_t uri_buf_size)
{
    const char *text = input + offset;
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

/* Check if a token type is a fenced code block structural marker that
 * should be suppressed from rendered output.  The fence delimiters
 * (``` or ~~~) and info strings are structural, not visual content. */
static int is_fenced_marker(FlareTokenType type)
{
    return type == HL_MARKUP_FENCED_OPEN || type == HL_MARKUP_FENCED_INFO ||
           type == HL_MARKUP_FENCED_CLOSE;
}

/* Copy token text to the output buffer, tracking line boundaries.
 * When `indent` is true, prepend 2 spaces at the start of each line
 * (after every \n). Returns the new output position. */
static size_t emit_token_text(char **out, size_t *cap, size_t pos,
                              const char *input, size_t offset, size_t length,
                              int indent, int *at_line_start)
{
    const char *src = input + offset;

    if (!indent) {
        buf_ensure(out, cap, pos, length + 5);
        memcpy(*out + pos, src, length);
        pos += length;
    } else {
        size_t src_pos = 0;
        while (src_pos < length) {
            const char *nl = memchr(src + src_pos, '\n', length - src_pos);
            if (!nl) {
                size_t chunk = length - src_pos;
                buf_ensure(out, cap, pos, chunk + 3);
                memcpy(*out + pos, src + src_pos, chunk);
                pos += chunk;
                break;
            }
            size_t chunk = (size_t)(nl - (src + src_pos)) + 1;
            buf_ensure(out, cap, pos, chunk + 3);
            memcpy(*out + pos, src + src_pos, chunk);
            pos += chunk;
            src_pos += chunk;
            /* Emit indentation after the newline (unless it's the
             * very end of the token — the next token will handle it
             * via the at_line_start mechanism). */
            if (src_pos < length) {
                buf_ensure(out, cap, pos, 3);
                memcpy(*out + pos, "  ", 2);
                pos += 2;
            }
        }
    }

    if (length > 0 && src[length - 1] == '\n')
        *at_line_start = 1;
    else if (length > 0)
        *at_line_start = 0;
    return pos;
}

/* Emit inline code text with backtick delimiters replaced by spaces.
 * The opening and closing backtick runs (same length per CommonMark)
 * are replaced with that many space characters.  The inner content
 * is copied verbatim. */
static size_t emit_inline_code_text(char **out, size_t *cap, size_t pos,
                                    const char *input, size_t offset, size_t length,
                                    int *at_line_start)
{
    /* Count opening backtick run */
    size_t bt_len = 0;
    while (bt_len < length && input[offset + bt_len] == '`')
        bt_len++;

    if (bt_len == 0 || bt_len >= length) {
        /* Malformed — emit as-is */
        return emit_token_text(out, cap, pos, input, offset, length, 0,
                               at_line_start);
    }

    /* Emit opening spaces (replacing the backtick delimiters) */
    buf_ensure(out, cap, pos, bt_len + 1);
    memset(*out + pos, ' ', bt_len);
    pos += bt_len;

    /* Emit inner content (between backticks) */
    size_t inner_off = offset + bt_len;
    size_t inner_len = length - 2 * bt_len;
    if (inner_len > 0) {
        buf_ensure(out, cap, pos, inner_len + 1);
        memcpy(*out + pos, input + inner_off, inner_len);
        pos += inner_len;
    }

    /* Emit closing spaces (replacing the backtick delimiters) */
    buf_ensure(out, cap, pos, bt_len + 1);
    memset(*out + pos, ' ', bt_len);
    pos += bt_len;

    *at_line_start = 0;
    return pos;
}

/* Emit only the title text from an inline link token [text](url),
 * suppressing the URL and all syntax characters. */
static size_t emit_link_text(char **out, size_t *cap, size_t pos,
                             const char *input, size_t offset, size_t length,
                             int *at_line_start)
{
    if (length < 2 || input[offset] != '[')
        return emit_token_text(out, cap, pos, input, offset, length, 0,
                               at_line_start);

    /* Find matching `]` with nesting support */
    size_t p = 1;
    int depth = 1;
    while (p < length) {
        if (input[offset + p] == '[')
            depth++;
        else if (input[offset + p] == ']') {
            depth--;
            if (depth == 0)
                break;
        }
        p++;
    }
    if (depth != 0)
        return emit_token_text(out, cap, pos, input, offset, length, 0,
                               at_line_start);

    /* Title text is between offset+1 and offset+p */
    size_t title_off = offset + 1;
    size_t title_len = p - 1;
    if (title_len > 0) {
        buf_ensure(out, cap, pos, title_len + 1);
        memcpy(*out + pos, input + title_off, title_len);
        pos += title_len;
    }

    *at_line_start = 0;
    return pos;
}

/* Emit autolink text <url> with angle brackets stripped. */
static size_t emit_autolink_text(char **out, size_t *cap, size_t pos,
                                 const char *input, size_t offset,
                                 size_t length, int *at_line_start)
{
    if (length < 2 || input[offset] != '<')
        return emit_token_text(out, cap, pos, input, offset, length, 0,
                               at_line_start);

    /* Find closing `>` */
    size_t end = 0;
    for (size_t i = 1; i < length; i++) {
        if (input[offset + i] == '>') {
            end = i;
            break;
        }
    }
    if (end == 0)
        return emit_token_text(out, cap, pos, input, offset, length, 0,
                               at_line_start);

    size_t uri_off = offset + 1;
    size_t uri_len = end - 1;
    if (uri_len > 0) {
        buf_ensure(out, cap, pos, uri_len + 1);
        memcpy(*out + pos, input + uri_off, uri_len);
        pos += uri_len;
    }

    *at_line_start = 0;
    return pos;
}

/* Format token stream into an ANSI string.
 * When enable_hyperlinks is 1, inline links and autolinks emit OSC 8
 * escape sequences for clickable hyperlinks on supporting terminals. */
char *flare_format_terminal_ex(const FlareToken *tokens, size_t count,
                               const char *input, const FlareStyle *style,
                               FlareColorDepth depth, int enable_hyperlinks)
{
    if (!tokens || count == 0 || !input || !style) {
        char *empty = malloc(5);
        if (empty)
            memcpy(empty, "\033[0m", 4), empty[4] = '\0';
        return empty;
    }

    /* Grow-only buffer */
    size_t cap = 256;
    char *out = malloc(cap);
    size_t pos = 0;

    FlareStyleEntry prev = { 0 };
    int prev_valid = 0;
    int in_fenced = 0;
    int at_line_start = 1; /* track line boundaries across tokens */

    for (size_t i = 0; i < count; i++) {
        /* Skip fenced code block structural markers — they are not
         * visual content. When skipping, also consume the line-ending
         * newline that belongs to the fence line:
         *   - \n immediately after FENCED_OPEN/INFO: ends the opening
         *     fence line (not a paragraph separator)
         *   - \n immediately before FENCED_CLOSE: ends the last code
         *     content line (not a paragraph separator)
         * The \n BEFORE FENCED_OPEN and AFTER FENCED_CLOSE are
         * paragraph separators and must be kept. */
        if (is_fenced_marker(tokens[i].type)) {
            if (tokens[i].type == HL_MARKUP_FENCED_OPEN ||
                tokens[i].type == HL_MARKUP_FENCED_INFO) {
                in_fenced = 1;
                at_line_start = 1;
                /* Consume trailing \n that ends the fence line */
                if (i + 1 < count && tokens[i + 1].type == HL_TEXT &&
                    tokens[i + 1].length == 1 &&
                    input[tokens[i + 1].offset] == '\n')
                    i++;
            } else if (tokens[i].type == HL_MARKUP_FENCED_CLOSE) {
                in_fenced = 0;
            }
            prev_valid = 0;
            continue;
        }
        if (tokens[i].type == HL_TEXT && tokens[i].length == 1 &&
            input[tokens[i].offset] == '\n' &&
            i + 1 < count && tokens[i + 1].type == HL_MARKUP_FENCED_CLOSE) {
            /* \n before closing fence: part of the fence block, not
             * a paragraph separator — suppress.  Also clear
             * in_fenced so subsequent content is not indented. */
            in_fenced = 0;
            prev_valid = 0;
            continue;
        }

        /* Emit 2-space indentation at the start of each line inside
         * a fenced code block, before the first styled token. */
        if (in_fenced && at_line_start) {
            buf_ensure(&out, &cap, pos, 2);
            memcpy(out + pos, "  ", 2);
            pos += 2;
            at_line_start = 0;
        }

        FlareStyleEntry entry = flare_style_get(style, tokens[i].type);

        /* Try to extract a hyperlink URI for link/autolink tokens */
        char uri_buf[1024];
        size_t uri_len = 0;
        int has_hyperlink = 0;
        if (enable_hyperlinks &&
            (tokens[i].type == HL_MARKUP_INLINE_LINK ||
             tokens[i].type == HL_MARKUP_INLINE_AUTOLINK)) {
            uri_len = extract_hyperlink_uri(tokens[i].type, input,
                                            tokens[i].offset, tokens[i].length,
                                            uri_buf, sizeof(uri_buf));
            has_hyperlink = (uri_len > 0);
        }

        /* Coalesce: if same style as previous, skip the escape */
        if (prev_valid && style_entries_equal(&prev, &entry)) {
            /* Emit OSC 8 open if this token has a hyperlink */
            if (has_hyperlink)
                emit_osc8_open(&out, &cap, &pos, uri_buf, uri_len);

            /* Just copy the text (inline code: backticks → spaces;
             * links/autolinks: title-only rendering) */
            size_t tlen = tokens[i].length;
            if (tokens[i].type == HL_MARKUP_INLINE_CODE)
                pos = emit_inline_code_text(&out, &cap, pos, input,
                                            tokens[i].offset, tlen,
                                            &at_line_start);
            else if (tokens[i].type == HL_MARKUP_INLINE_LINK)
                pos = emit_link_text(&out, &cap, pos, input,
                                     tokens[i].offset, tlen,
                                     &at_line_start);
            else if (tokens[i].type == HL_MARKUP_INLINE_AUTOLINK)
                pos = emit_autolink_text(&out, &cap, pos, input,
                                         tokens[i].offset, tlen,
                                         &at_line_start);
            else
                pos = emit_token_text(&out, &cap, pos, input,
                                      tokens[i].offset, tlen, in_fenced,
                                      &at_line_start);

            if (has_hyperlink)
                emit_osc8_close(&out, &cap, &pos);
            continue;
        }

        /* Build SGR sequence for this style.
         * Always lead with reset (0) to clear attributes from the previous
         * token — ANSI attributes are cumulative, so without an explicit
         * reset a bold from token N would persist into token N+1. */
        char sgr[128];
        int sglen = 0;

        int need_semi = 0;
        int sgrpos = 0;
        sgr[sgrpos++] = '\033';
        sgr[sgrpos++] = '[';
        sgr[sgrpos++] = '0';
        need_semi = 1;

        if (entry.bold) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            sglen = snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "1");
            sgrpos += sglen;
            need_semi = 1;
        }
        if (entry.faint) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            sglen = snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "2");
            sgrpos += sglen;
            need_semi = 1;
        }
        if (entry.italic) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            sglen = snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "3");
            sgrpos += sglen;
            need_semi = 1;
        }
        if (entry.underline) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            sglen = snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "4");
            sgrpos += sglen;
            need_semi = 1;
        }
        if (entry.strikethrough) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            sglen = snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "9");
            sgrpos += sglen;
            need_semi = 1;
        }

        /* Foreground color */
        char fgbuf[32];
        int fglen;
        if (depth == BFLARE_COLOR_TRUECOLOR) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            fglen = snprintf(fgbuf, sizeof(fgbuf), "38;2;%d;%d;%d", entry.fg_r, entry.fg_g, entry.fg_b);
            memcpy(sgr + sgrpos, fgbuf, fglen);
            sgrpos += fglen;
            need_semi = 1;
        } else if (depth == BFLARE_COLOR_256) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            int idx = flare_color_rgb_to_256(entry.fg_r, entry.fg_g, entry.fg_b);
            fglen = snprintf(fgbuf, sizeof(fgbuf), "38;5;%d", idx);
            memcpy(sgr + sgrpos, fgbuf, fglen);
            sgrpos += fglen;
            need_semi = 1;
        } else if (depth == BFLARE_COLOR_16) {
            /* 16-color: aixterm bright codes 90-97 */
            int idx = flare_color_rgb_to_16(entry.fg_r, entry.fg_g, entry.fg_b);
            if (need_semi)
                sgr[sgrpos++] = ';';
            if (idx >= 8) {
                fglen = snprintf(fgbuf, sizeof(fgbuf), "%d", 90 + (idx - 8));
            } else {
                fglen = snprintf(fgbuf, sizeof(fgbuf), "%d", 30 + idx);
            }
            memcpy(sgr + sgrpos, fgbuf, fglen);
            sgrpos += fglen;
            need_semi = 1;
        } else {
            /* 8-color: normal codes 30-37, bold-intensity for bright */
            int idx = flare_color_rgb_to_8(entry.fg_r, entry.fg_g, entry.fg_b);
            if (idx >= 8) {
                /* Bright: emit 1; (bold-intensity) + color in one SGR param.
                 * Avoid double-bold if entry.bold was already emitted above. */
                if (!entry.bold) {
                    if (need_semi)
                        sgr[sgrpos++] = ';';
                    fglen = snprintf(fgbuf, sizeof(fgbuf), "1;%d", 30 + (idx - 8));
                } else {
                    /* Bold was already emitted as "1" — just append color */
                    if (need_semi)
                        sgr[sgrpos++] = ';';
                    fglen = snprintf(fgbuf, sizeof(fgbuf), "%d", 30 + (idx - 8));
                }
                memcpy(sgr + sgrpos, fgbuf, fglen);
                sgrpos += fglen;
                need_semi = 1;
            } else {
                if (need_semi)
                    sgr[sgrpos++] = ';';
                fglen = snprintf(fgbuf, sizeof(fgbuf), "%d", 30 + idx);
                memcpy(sgr + sgrpos, fgbuf, fglen);
                sgrpos += fglen;
                need_semi = 1;
            }
        }

        /* Background color (only if non-zero to avoid resetting bg
         * on tokens that don't need it) */
        if (entry.bg_r || entry.bg_g || entry.bg_b) {
            char bgbuf[32];
            int bglen;
            if (depth == BFLARE_COLOR_TRUECOLOR) {
                if (need_semi)
                    sgr[sgrpos++] = ';';
                bglen = snprintf(bgbuf, sizeof(bgbuf), "48;2;%d;%d;%d",
                                 entry.bg_r, entry.bg_g, entry.bg_b);
                memcpy(sgr + sgrpos, bgbuf, bglen);
                sgrpos += bglen;
                need_semi = 1;
            } else if (depth == BFLARE_COLOR_256) {
                if (need_semi)
                    sgr[sgrpos++] = ';';
                int bidx = flare_color_rgb_to_256(entry.bg_r, entry.bg_g, entry.bg_b);
                bglen = snprintf(bgbuf, sizeof(bgbuf), "48;5;%d", bidx);
                memcpy(sgr + sgrpos, bgbuf, bglen);
                sgrpos += bglen;
                need_semi = 1;
            } else if (depth == BFLARE_COLOR_16) {
                if (need_semi)
                    sgr[sgrpos++] = ';';
                int bidx = flare_color_rgb_to_16(entry.bg_r, entry.bg_g, entry.bg_b);
                if (bidx >= 8) {
                    bglen = snprintf(bgbuf, sizeof(bgbuf), "%d", 100 + (bidx - 8));
                } else {
                    bglen = snprintf(bgbuf, sizeof(bgbuf), "%d", 40 + bidx);
                }
                memcpy(sgr + sgrpos, bgbuf, bglen);
                sgrpos += bglen;
                need_semi = 1;
            } else {
                if (need_semi)
                    sgr[sgrpos++] = ';';
                int bidx = flare_color_rgb_to_8(entry.bg_r, entry.bg_g, entry.bg_b);
                bglen = snprintf(bgbuf, sizeof(bgbuf), "%d", 40 + bidx);
                memcpy(sgr + sgrpos, bgbuf, bglen);
                sgrpos += bglen;
                need_semi = 1;
            }
        }

        sgr[sgrpos++] = 'm';

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
            pos = emit_inline_code_text(&out, &cap, pos, input,
                                        tokens[i].offset, tlen,
                                        &at_line_start);
        else if (tokens[i].type == HL_MARKUP_INLINE_LINK)
            pos = emit_link_text(&out, &cap, pos, input,
                                 tokens[i].offset, tlen,
                                 &at_line_start);
        else if (tokens[i].type == HL_MARKUP_INLINE_AUTOLINK)
            pos = emit_autolink_text(&out, &cap, pos, input,
                                     tokens[i].offset, tlen,
                                     &at_line_start);
        else
            pos = emit_token_text(&out, &cap, pos, input,
                                  tokens[i].offset, tlen, in_fenced,
                                  &at_line_start);

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

/* Legacy wrapper: auto-detect hyperlink support */
char *flare_format_terminal(const FlareToken *tokens, size_t count,
                            const char *input, const FlareStyle *style,
                            FlareColorDepth depth)
{
    return flare_format_terminal_ex(tokens, count, input, style, depth,
                                    flare_terminal_supports_hyperlinks());
}
