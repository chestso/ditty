/* formatter.c - FlareFormatter interface */

#include "../include/ditty/highlight.h"
#include "../include/ditty/flare_writer.h"
#include "../include/ditty/flare_layout.h"
#include "../include/ditty/flare_token_source.h"
#include <gc.h>
#include <stdlib.h>
#include <string.h>

struct FlareFormatter
{
    FlareColorDepth depth;
    FlareStyle *style;
    FlareWriter *writer;
    FlareLayout *layout;
    int prev_valid;
    FlareStyleEntry prev_style;
    int hyperlink_open;
    char uri[1024];
    size_t uri_len;
};

static const char *reset_seq = "\033[0m";

static void writer_puts(FlareWriter *w, const char *s)
{
    flare_writer_write(w, s, strlen(s));
}

static void emit_sgr(FlareWriter *w, const FlareStyleEntry *entry,
                     FlareColorDepth depth)
{
    char sgr[128];
    int sgrpos = 0;

    sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "\033[");
    int need_semi = 0;

    if (entry->bold) {
        if (need_semi)
            sgr[sgrpos++] = ';';
        sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "1");
        need_semi = 1;
    }
    if (entry->faint) {
        if (need_semi)
            sgr[sgrpos++] = ';';
        sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "2");
        need_semi = 1;
    }
    if (entry->italic) {
        if (need_semi)
            sgr[sgrpos++] = ';';
        sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "3");
        need_semi = 1;
    }
    if (entry->underline) {
        if (need_semi)
            sgr[sgrpos++] = ';';
        sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "4");
        need_semi = 1;
    }
    if (entry->strikethrough) {
        if (need_semi)
            sgr[sgrpos++] = ';';
        sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "9");
        need_semi = 1;
    }

    if (depth == BFLARE_COLOR_TRUECOLOR) {
        if (need_semi)
            sgr[sgrpos++] = ';';
        sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos,
                           "38;2;%d;%d;%d", entry->fg_r, entry->fg_g, entry->fg_b);
        need_semi = 1;
    } else if (depth == BFLARE_COLOR_256) {
        if (need_semi)
            sgr[sgrpos++] = ';';
        sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos,
                           "38;5;%d", flare_color_rgb_to_256(entry->fg_r, entry->fg_g, entry->fg_b));
        need_semi = 1;
    } else if (depth == BFLARE_COLOR_16) {
        if (need_semi)
            sgr[sgrpos++] = ';';
        int idx = flare_color_rgb_to_16(entry->fg_r, entry->fg_g, entry->fg_b);
        sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "%d",
                           idx >= 8 ? 90 + (idx - 8) : 30 + idx);
        need_semi = 1;
    } else {
        if (need_semi)
            sgr[sgrpos++] = ';';
        int idx = flare_color_rgb_to_8(entry->fg_r, entry->fg_g, entry->fg_b);
        sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "%d",
                           idx >= 8 ? 30 + (idx - 8) : 30 + idx);
        need_semi = 1;
    }

    if (entry->bg_r || entry->bg_g || entry->bg_b) {
        if (need_semi)
            sgr[sgrpos++] = ';';
        if (depth == BFLARE_COLOR_TRUECOLOR) {
            sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos,
                               "48;2;%d;%d;%d", entry->bg_r, entry->bg_g, entry->bg_b);
        } else if (depth == BFLARE_COLOR_256) {
            sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos,
                               "48;5;%d", flare_color_rgb_to_256(entry->bg_r, entry->bg_g, entry->bg_b));
        } else if (depth == BFLARE_COLOR_16) {
            int idx = flare_color_rgb_to_16(entry->bg_r, entry->bg_g, entry->bg_b);
            sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "%d",
                               idx >= 8 ? 100 + (idx - 8) : 40 + idx);
        } else {
            int idx = flare_color_rgb_to_8(entry->bg_r, entry->bg_g, entry->bg_b);
            sgrpos += snprintf(sgr + sgrpos, sizeof(sgr) - sgrpos, "%d",
                               40 + (idx >= 8 ? idx - 8 : idx));
        }
        need_semi = 1;
    }

    sgr[sgrpos++] = 'm';
    flare_writer_write(w, sgr, sgrpos);
}

static int style_entries_equal(const FlareStyleEntry *a, const FlareStyleEntry *b)
{
    return a->fg_r == b->fg_r && a->fg_g == b->fg_g && a->fg_b == b->fg_b &&
           a->bg_r == b->bg_r && a->bg_g == b->bg_g && a->bg_b == b->bg_b &&
           a->bold == b->bold && a->italic == b->italic &&
           a->underline == b->underline && a->faint == b->faint &&
           a->strikethrough == b->strikethrough;
}

static void emit_osc8_open(FlareWriter *w, const char *uri, size_t uri_len)
{
    flare_writer_write(w, "\033]8;;", 5);
    flare_writer_write(w, uri, uri_len);
    flare_writer_write(w, "\007", 1);
}

static void emit_osc8_close(FlareWriter *w)
{
    flare_writer_write(w, "\033]8;;\007", 7);
}

static size_t extract_uri(const char *text, size_t len, char *out, size_t out_size)
{
    if (len > 2 && text[0] == '<' && text[len - 1] == '>') {
        size_t n = len - 2 < out_size - 1 ? len - 2 : out_size - 1;
        memcpy(out, text + 1, n);
        out[n] = '\0';
        return n;
    }
    if (text[0] == '[') {
        size_t pos = 1;
        int depth = 1;
        while (pos < len && depth > 0) {
            if (text[pos] == '[')
                depth++;
            else if (text[pos] == ']')
                depth--;
            pos++;
        }
        if (pos < len && text[pos] == '(') {
            pos++;
            size_t start = pos;
            int paren = 1;
            while (pos < len && paren > 0) {
                if (text[pos] == '(')
                    paren++;
                else if (text[pos] == ')')
                    paren--;
                if (paren > 0)
                    pos++;
            }
            size_t n = pos - start < out_size - 1 ? pos - start : out_size - 1;
            memcpy(out, text + start, n);
            out[n] = '\0';
            return n;
        }
    }
    return 0;
}

static int format_token(FlareFormatter *fmt, const FlareToken *token)
{
    FlareWriter *w = fmt->writer;

    if (fmt->layout && fmt->layout->resized) {
        writer_puts(w, "\033[2J\033[H");
        fmt->layout->resized = 0;
        fmt->prev_valid = 0;
    }

    FlareStyleEntry entry = flare_style_get(fmt->style, token->type);

    if (!fmt->prev_valid || !style_entries_equal(&fmt->prev_style, &entry)) {
        if (fmt->prev_valid)
            writer_puts(w, reset_seq);
        emit_sgr(w, &entry, fmt->depth);
        fmt->prev_style = entry;
        fmt->prev_valid = 1;
    }

    size_t uri_len = 0;
    if (token->type == HL_MARKUP_INLINE_LINK || token->type == HL_MARKUP_INLINE_AUTOLINK) {
        uri_len = extract_uri(token->text, token->length, fmt->uri, sizeof(fmt->uri));
        if (uri_len > 0) {
            emit_osc8_open(w, fmt->uri, uri_len);
            fmt->hyperlink_open = 1;
        }
    }

    const char *text = token->text;
    size_t len = token->length;
    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\n') {
            writer_puts(w, reset_seq);
            flare_writer_write(w, "\n", 1);
            if (i + 1 < len) {
                emit_sgr(w, &entry, fmt->depth);
                if (uri_len > 0)
                    emit_osc8_open(w, fmt->uri, uri_len);
            }
        } else {
            flare_writer_write(w, &text[i], 1);
        }
    }

    if (fmt->hyperlink_open) {
        emit_osc8_close(w);
        fmt->hyperlink_open = 0;
    }

    return 0;
}

FlareFormatter *flare_formatter_terminal(FlareColorDepth depth, FlareWriter *writer, FlareStyle *style)
{
    return flare_formatter_terminal_ex(depth, writer, style, NULL);
}

FlareFormatter *flare_formatter_terminal_ex(FlareColorDepth depth, FlareWriter *writer, FlareStyle *style,
                                            const FlareReflowOptions *reflow)
{
    FlareFormatter *f = calloc(1, sizeof(FlareFormatter));
    if (!f)
        return NULL;
    f->depth = depth;
    f->writer = writer;
    f->style = style;
    (void)reflow;
    return f;
}

void flare_formatter_free(FlareFormatter *formatter)
{
    free(formatter);
}
int flare_formatter_format(FlareFormatter *fmt, FlareTokenSource *src)
{
    if (!fmt || !src)
        return -1;

    /* Collect tokens into an array, then render with the single terminal
     * formatter. This keeps all terminal rendering logic in one place. */
    size_t count = 0;
    size_t capacity = 64;
    FlareToken *tokens = malloc(capacity * sizeof(FlareToken));
    if (!tokens)
        return -1;

    FlareToken tok;
    int result;
    while ((result = flare_token_source_pull(src, &tok)) > 0) {
        if (count >= capacity) {
            capacity *= 2;
            FlareToken *tmp = realloc(tokens, capacity * sizeof(FlareToken));
            if (!tmp) {
                free(tokens);
                return -1;
            }
            tokens = tmp;
        }
        tokens[count++] = tok;
    }

    if (result < 0) {
        free(tokens);
        return -1;
    }

    char *ansi = flare_format_terminal(tokens, count, fmt->style, fmt->depth,
                                       flare_terminal_supports_hyperlinks());
    free(tokens);

    if (!ansi)
        return -1;

    size_t len = strlen(ansi);
    flare_writer_write(fmt->writer, ansi, len);
    free(ansi);

    return 0;
}

/* Internal accessor used by formatter_terminal.c */
FlareColorDepth flare_formatter_depth(const FlareFormatter *f)
{
    return f ? f->depth : BFLARE_COLOR_TRUECOLOR;
}
