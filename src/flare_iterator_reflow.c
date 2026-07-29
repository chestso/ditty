/* flare_iterator_reflow.c - Reflow iterator for Flare v3.0
 *
 * Pull-based streaming reflow: transforms a token stream by inserting
 * soft line breaks at word boundaries according to a shared FlareLayout.
 */

#include "../include/ditty/flare_iterator.h"
#include "../include/ditty/flare_token_source.h"
#include "../include/ditty/highlight.h"
#include "../include/utf8.h"
#include <gc.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    REFLOW_STATE_NORMAL,
    REFLOW_STATE_SPLITTING
} ReflowStateEnum;

typedef struct
{
    FlareTokenSource base;
    FlareTokenSource *upstream;
    FlareLayout *layout;
    FlareReflowOptions options;

    ReflowStateEnum state;
    int in_fenced_block;
    int in_heading;
    int column;

    FlareToken current;
    size_t split_pos;

    int has_pending;
    FlareToken pending;

    int has_stash;
    FlareToken stash;

    int pending_space; /* 1 if we need to emit a space before the next token */
} ReflowIterator;

static int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void emit_soft_break(ReflowIterator *it)
{
    char *text = GC_MALLOC_ATOMIC(1);
    text[0] = '\n';
    it->has_pending = 1;
    it->pending.type = HL_TEXT;
    it->pending.text = text;
    it->pending.length = 1;
}

static int reflow_pull(FlareTokenSource *src, FlareToken *out);
static const char *reflow_error(FlareTokenSource *src);
static void reflow_free(FlareTokenSource *src);

static const FlareTokenSourceVTable reflow_vtable = {
    .pull = reflow_pull,
    .error = reflow_error,
    .free = reflow_free
};

static int target_width(const ReflowIterator *it)
{
    if (!it->layout || it->layout->width <= 0)
        return 76;
    return it->layout->width;
}

static int reflow_pull(FlareTokenSource *src, FlareToken *out)
{
    ReflowIterator *it = (ReflowIterator *)src;

    if (it->has_pending) {
        *out = it->pending;
        it->has_pending = 0;
        /* Update column with the word's width */
        it->column += utf8_display_width(it->pending.text);
        return 1;
    }

    /* Emit pending space before the next non-text token */
    if (it->pending_space) {
        char *space_text = GC_MALLOC_ATOMIC(2);
        space_text[0] = ' ';
        space_text[1] = '\0';
        out->type = HL_TEXT;
        out->text = space_text;
        out->length = 1;
        it->pending_space = 0;
        /* Don't increment column - the space was already counted when we set pending_space */
        return 1;
    }

    if (it->state == REFLOW_STATE_SPLITTING) {
        const char *text = it->current.text;
        size_t len = it->current.length;

        /* Skip leading spaces if we're at column 0 (after a wrap or at start) */
        if (it->column == 0) {
            while (it->split_pos < len && is_space(text[it->split_pos]))
                it->split_pos++;
        }

        if (it->split_pos >= len) {
            /* Check if the original text ended with a trailing space */
            if (len > 0 && is_space(text[len - 1]) && it->column > 0) {
                it->pending_space = 1;
            }
            it->state = REFLOW_STATE_NORMAL;
            return reflow_pull(src, out);
        }

        /* Find the next word */
        size_t word_start = it->split_pos;
        /* If at a space, skip it (we handle spacing explicitly below) */
        if (is_space(text[word_start])) {
            word_start++;
            it->split_pos = word_start;
        }
        while (it->split_pos < len && !is_space(text[it->split_pos]))
            it->split_pos++;
        size_t word_len = it->split_pos - word_start;

        /* Skip empty words (can happen with multiple consecutive spaces) */
        if (word_len == 0) {
            /* Check if the original text ended with a trailing space */
            if (len > 0 && is_space(text[len - 1]) && it->column > 0) {
                it->pending_space = 1;
            }
            it->state = REFLOW_STATE_NORMAL;
            return reflow_pull(src, out);
        }

        /* Calculate word width */
        char *word_tmp = GC_MALLOC_ATOMIC(word_len + 1);
        memcpy(word_tmp, text + word_start, word_len);
        word_tmp[word_len] = '\0';
        int width = utf8_display_width(word_tmp);
        int tw = target_width(it);

        /* Check if word fits (account for space if column > 0) */
        int space_width = (it->column > 0) ? 1 : 0;
        if (it->column + space_width + width > tw && it->column > 0) {
            /* Word doesn't fit on current line - wrap first */
            emit_soft_break(it);
            it->column = 0;
            it->split_pos = word_start; /* reprocess this word after wrap */
            *out = it->pending;
            it->has_pending = 0;
            return 1;
        }

        /* Word fits - emit space first if needed */
        if (space_width) {
            char *space_text = GC_MALLOC_ATOMIC(2);
            space_text[0] = ' ';
            space_text[1] = '\0';
            out->type = HL_TEXT;
            out->text = space_text;
            out->length = 1;
            it->column += 1;
            /* Stash word for next call */
            it->has_pending = 1;
            it->pending.type = HL_TEXT;
            it->pending.text = word_tmp;
            it->pending.length = word_len;
            return 1;
        }

        /* At column 0, emit word directly */
        out->type = HL_TEXT;
        out->text = word_tmp;
        out->length = word_len;
        it->column += width;
        return 1;
    }

    FlareToken token;
    int result = flare_token_source_pull(it->upstream, &token);
    if (result <= 0)
        return result;

    if (token.type == HL_MARKUP_FENCED_OPEN) {
        it->in_fenced_block = 1;
        *out = token;
        return 1;
    }
    if (token.type == HL_MARKUP_FENCED_CLOSE) {
        it->in_fenced_block = 0;
        *out = token;
        return 1;
    }
    if (token.type == HL_MARKUP_HEADING_MARKER) {
        it->in_heading = !it->in_heading;
        *out = token;
        return 1;
    }
    if (token.type == HL_MARKUP_SETEXT_UNDERLINE) {
        it->in_heading = 0;
        *out = token;
        return 1;
    }

    if (it->in_fenced_block || it->in_heading) {
        *out = token;
        if (token.type == HL_TEXT && token.length == 1 &&
            token.text[0] == '\n')
            it->column = 0;
        return 1;
    }

    if (token.type == HL_MARKUP_INLINE_BREAK) {
        if (it->options.preserve_hard_breaks) {
            it->column = 0;
            *out = token;
            return 1;
        }
    }

    if (token.type == HL_TEXT) {
        int all_space = 1;
        for (size_t i = 0; i < token.length; i++) {
            if (!is_space(token.text[i])) {
                all_space = 0;
                break;
            }
        }
        if (all_space) {
            it->column = 0;
            *out = token;
            return 1;
        }

        it->current = token;
        it->split_pos = 0;
        it->state = REFLOW_STATE_SPLITTING;
        return reflow_pull(src, out);
    }

    /* Pass through other tokens but update column for their display width */
    *out = token;
    if (token.text && token.length > 0) {
        it->column += utf8_display_width(token.text);
    }
    return 1;
}

static const char *reflow_error(FlareTokenSource *src)
{
    ReflowIterator *it = (ReflowIterator *)src;
    if (!it->upstream)
        return NULL;
    return flare_token_source_error(it->upstream);
}

static void reflow_free(FlareTokenSource *src)
{
    ReflowIterator *it = (ReflowIterator *)src;
    if (!it)
        return;
    if (it->upstream)
        flare_token_source_free(it->upstream);
    free(it);
}

FlareTokenSource *flare_iterator_reflow(FlareTokenSource *upstream,
                                        FlareLayout *layout,
                                        const FlareReflowOptions *options)
{
    ReflowIterator *it = calloc(1, sizeof(ReflowIterator));
    if (!it)
        return NULL;
    it->base.vtable = &reflow_vtable;
    it->upstream = upstream;
    it->layout = layout;
    if (options)
        it->options = *options;
    else
        it->options = FLARE_ITERATOR_REFLOW_DEFAULT;
    it->state = REFLOW_STATE_NORMAL;
    return &it->base;
}
