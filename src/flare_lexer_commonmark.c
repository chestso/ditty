/* flare_lexer_commonmark.c - Flare streaming CommonMark lexer
 *
 * Two-phase CommonMark lexer:
 *   Phase 1 - Block parser: classify lines into a tree of block nodes.
 *   Phase 2 - Inline parser + emitter: walk the tree, tokenize inline
 *             markup inside headings/paragraphs/list items, and emit a
 *             stream of FlareTokens (one per pull()).
 *
 * The lexer still reads the whole source eagerly because CommonMark
 * block structure can depend on later lines, but the public API is the
 * streaming FlareTokenSource.
 */

#include "ditty/highlight.h"
#include <gc.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* --------------------------------------------------------------------------
 * Block node tree
 * -------------------------------------------------------------------------- */

typedef enum
{
    CM_BLOCK_PARAGRAPH,
    CM_BLOCK_HEADING,
    CM_BLOCK_THEMATIC_BREAK,
    CM_BLOCK_FENCED_CODE,
    CM_BLOCK_LIST,
    CM_BLOCK_LIST_ITEM,
    CM_BLOCK_BLOCKQUOTE,
} CmBlockType;

typedef struct CmBlock CmBlock;
struct CmBlock
{
    CmBlockType type;
    size_t level; /* heading level, list indentation, etc. */
    char *text;   /* owned, raw content for leaf blocks */
    size_t text_len;
    int closed; /* for fenced code: has closing fence */
    int setext; /* for headings: 1 if created from a setext underline */

    CmBlock *parent;
    CmBlock *first_child;
    CmBlock *last_child;
    CmBlock *next_sibling;
};

/* --------------------------------------------------------------------------
 * Lexer state
 * -------------------------------------------------------------------------- */

typedef struct
{
    FlareTokenSource base;
    FlareSource *src;
    Environment *env;
    FlareToken *tokens;
    size_t count;
    size_t capacity;
    size_t pos;
} CommonmarkLexer;

/* --------------------------------------------------------------------------
 * Token buffer
 * -------------------------------------------------------------------------- */

static void cm_push(CommonmarkLexer *lex, FlareTokenType type,
                    const char *text, size_t length)
{
    if (length == 0 && type != HL_TEXT && type != HL_MARKUP_PARAGRAPH)
        return;
    if (lex->count >= lex->capacity) {
        lex->capacity = lex->capacity ? lex->capacity * 2 : 32;
        lex->tokens = realloc(lex->tokens, lex->capacity * sizeof(FlareToken));
    }
    char *copy = GC_MALLOC_ATOMIC(length + 1);
    if (copy) {
        memcpy(copy, text, length);
        copy[length] = '\0';
    }
    lex->tokens[lex->count].type = type;
    lex->tokens[lex->count].text = copy;
    lex->tokens[lex->count].length = length;
    lex->count++;
}

static void cm_push_text(CommonmarkLexer *lex, const char *text, size_t length)
{
    if (length > 0)
        cm_push(lex, HL_TEXT, text, length);
}

static void cm_push_newline(CommonmarkLexer *lex)
{
    cm_push_text(lex, "\n", 1);
}

/* --------------------------------------------------------------------------
 * Line classification helpers
 * -------------------------------------------------------------------------- */

static int cm_is_blank(const char *line, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r')
            return 0;
    }
    return 1;
}

/* Returns the bullet character and content offset for a list item line,
 * or 0 if the line is not a list item.  Indentation is not checked here;
 * callers that care about the spec's "up to three spaces" rule enforce it
 * themselves. */
static char cm_is_bullet_item(const char *line, size_t len, size_t *content_off)
{
    size_t i = 0;
    while (i < len && (line[i] == ' ' || line[i] == '\t'))
        i++;
    if (i + 2 > len)
        return 0;
    char c = line[i];
    if ((c == '-' || c == '*' || c == '+') && (line[i + 1] == ' ' || line[i + 1] == '\t')) {
        *content_off = i + 2;
        return c;
    }
    return 0;
}

/* Returns 1 and writes the content offset if the line starts with an ordered
 * list marker like "1. " or "2) ".  Indentation is not checked here. */
static int cm_is_ordered_item(const char *line, size_t len, size_t *content_off)
{
    size_t i = 0;
    while (i < len && (line[i] == ' ' || line[i] == '\t'))
        i++;
    size_t num_start = i;
    size_t num_count = 0;
    while (i < len && line[i] >= '0' && line[i] <= '9') {
        num_count++;
        i++;
    }
    if (num_count == 0 || num_count > 9 || i + 2 > len)
        return 0;
    char delim = line[i];
    if (delim != '.' && delim != ')')
        return 0;
    if (line[i + 1] != ' ' && line[i + 1] != '\t')
        return 0;
    *content_off = i + 2;
    (void)num_start;
    return 1;
}

static int cm_is_thematic_break(const char *line, size_t len)
{
    size_t i = 0;
    while (i < len && (line[i] == ' ' || line[i] == '\t'))
        i++;
    if (i > 3 || i >= len)
        return 0;
    char c = line[i];
    if (c != '-' && c != '*' && c != '_')
        return 0;
    int count = 0;
    for (; i < len; i++) {
        if (line[i] == c)
            count++;
        else if (line[i] != ' ' && line[i] != '\t')
            return 0;
    }
    return count >= 3;
}

/* Detect a setext heading underline.  Returns 1 for level 1 (=), 2 for level
 * 2 (-), or 0 if the line is not a valid setext underline. */
static int cm_setext_level(const char *line, size_t len)
{
    size_t i = 0;
    while (i < len && (line[i] == ' ' || line[i] == '\t'))
        i++;
    if (i > 3 || i >= len)
        return 0;
    char c = line[i];
    if (c != '=' && c != '-')
        return 0;
    size_t count = 0;
    for (; i < len; i++) {
        if (line[i] == c)
            count++;
        else if (line[i] != ' ' && line[i] != '\t')
            return 0;
    }
    if (count < 1)
        return 0;
    return (c == '=') ? 1 : 2;
}

/* Returns heading level 1-6, or 0 if not an ATX heading. */
static size_t cm_atx_level(const char *line, size_t len)
{
    size_t i = 0;
    while (i < len && (line[i] == ' ' || line[i] == '\t'))
        i++;
    if (i > 3)
        return 0;
    size_t start = i;
    while (i < len && line[i] == '#')
        i++;
    size_t hashes = i - start;
    if (hashes < 1 || hashes > 6)
        return 0;
    if (i < len && line[i] != ' ' && line[i] != '\t')
        return 0;
    return hashes;
}

/* --------------------------------------------------------------------------
 * Block node helpers
 * -------------------------------------------------------------------------- */

static CmBlock *cm_block_new(CmBlockType type)
{
    CmBlock *b = calloc(1, sizeof(CmBlock));
    if (!b)
        return NULL;
    b->type = type;
    return b;
}

static void cm_block_append_child(CmBlock *parent, CmBlock *child)
{
    if (!parent || !child)
        return;
    child->parent = parent;
    child->next_sibling = NULL;
    if (!parent->first_child) {
        parent->first_child = child;
        parent->last_child = child;
    } else {
        parent->last_child->next_sibling = child;
        parent->last_child = child;
    }
}

static void cm_block_free(CmBlock *b)
{
    if (!b)
        return;
    CmBlock *child = b->first_child;
    while (child) {
        CmBlock *next = child->next_sibling;
        cm_block_free(child);
        child = next;
    }
    free(b->text);
    free(b);
}

/* Duplicate a slice of text; caller frees. */
static char *cm_dup(const char *s, size_t len)
{
    char *out = malloc(len + 1);
    if (!out)
        return NULL;
    memcpy(out, s, len);
    out[len] = '\0';
    return out;
}

/* --------------------------------------------------------------------------
 * Phase 1: Block parser
 * -------------------------------------------------------------------------- */

/* Forward declarations */
static size_t cm_indent_of(const char *line, size_t len);
static CmBlock *cm_parse_list(const char **lines, size_t *line_lens,
                              size_t line_count, size_t start, size_t *out_end,
                              size_t base_indent);
static CmBlock *cm_parse_blockquote(const char **lines, size_t *line_lens,
                                    size_t line_count, size_t start, size_t *out_end);
static size_t cm_parse_fenced_code_block(const char **lines, size_t *line_lens,
                                         size_t line_count, size_t i, CmBlock **out);
static CmBlock *cm_parse_heading(const char *line, size_t len);

/* Build a sequence of blocks from lines[start..line_count).  A line whose
 * indentation is less than base_indent ends the block sequence (it belongs
 * to an outer container).  *out_end receives the first unconsumed index. */
static CmBlock *cm_build_blocks(const char **lines, size_t *line_lens,
                                size_t line_count, size_t start, size_t *out_end,
                                size_t base_indent)
{
    CmBlock *root = cm_block_new(CM_BLOCK_LIST);
    if (!root) {
        *out_end = start;
        return NULL;
    }

    size_t i = start;
    while (i < line_count) {
        const char *line = lines[i];
        size_t len = line_lens[i];

        if (cm_is_blank(line, len)) {
            i++;
            continue;
        }

        size_t indent = cm_indent_of(line, len);
        if (indent < base_indent) {
            /* Belongs to an outer container. */
            break;
        }

        if (cm_is_thematic_break(line, len)) {
            CmBlock *tb = cm_block_new(CM_BLOCK_THEMATIC_BREAK);
            if (tb) {
                tb->text = cm_dup(line, len);
                tb->text_len = len;
                cm_block_append_child(root, tb);
            }
            i++;
            continue;
        }

        if (cm_atx_level(line, len) > 0) {
            CmBlock *h = cm_parse_heading(line, len);
            if (h)
                cm_block_append_child(root, h);
            i++;
            continue;
        }

        CmBlock *fence = NULL;
        size_t next = cm_parse_fenced_code_block(lines, line_lens, line_count, i, &fence);
        if (next > i) {
            cm_block_append_child(root, fence);
            i = next;
            continue;
        }

        /* List item at the current base indent starts a list. */
        size_t content_off = 0;
        if (cm_is_bullet_item(line, len, &content_off) ||
            cm_is_ordered_item(line, len, &content_off)) {
            CmBlock *list = cm_parse_list(lines, line_lens, line_count, i, &i, base_indent);
            if (list) {
                if (list->first_child)
                    cm_block_append_child(root, list);
                else
                    cm_block_free(list);
            }
            continue;
        }

        /* Block quote: a line starting with '>' at any indentation. */
        if (len > 0 && line[0] == '>') {
            CmBlock *quote = cm_parse_blockquote(lines, line_lens, line_count, i, &i);
            if (quote) {
                cm_block_append_child(root, quote);
            }
            continue;
        }

        /* Paragraph: collect consecutive non-blank lines.  A list item at
         * the current indentation interrupts a paragraph (CommonMark
         * example 303).  A setext heading underline also ends the current
         * paragraph and turns it into a heading. */
        size_t para_start = i;
        size_t para_len = 0;
        int setext_level = 0;
        while (i < line_count && !cm_is_blank(lines[i], line_lens[i])) {
            if (i != para_start) {
                size_t off = 0;
                if (cm_is_bullet_item(lines[i], line_lens[i], &off) ||
                    cm_is_ordered_item(lines[i], line_lens[i], &off))
                    break;
            }
            i++;
            if (i < line_count && cm_setext_level(lines[i], line_lens[i])) {
                /* The line we just consumed is the last content line; the
                 * next line is the underline, which will be consumed below. */
                break;
            }
        }

        /* Check whether the current line is a setext heading underline.
         * If so, the previously collected lines form the heading content. */
        size_t para_end = i;
        if (i < line_count) {
            setext_level = cm_setext_level(lines[i], line_lens[i]);
            if (setext_level > 0) {
                para_end = i;
                i++; /* consume the underline */
            }
        }

        for (size_t pidx = para_start; pidx < para_end; pidx++) {
            if (pidx > para_start)
                para_len += 1;
            para_len += line_lens[pidx];
        }
        char *para = malloc(para_len + 1);
        if (para) {
            size_t pos = 0;
            for (size_t pidx = para_start; pidx < para_end; pidx++) {
                if (pidx > para_start)
                    para[pos++] = ' ';
                memcpy(para + pos, lines[pidx], line_lens[pidx]);
                pos += line_lens[pidx];
            }
            para[pos] = '\0';
            CmBlock *p = cm_block_new(setext_level ? CM_BLOCK_HEADING : CM_BLOCK_PARAGRAPH);
            if (p) {
                p->text = para;
                p->text_len = pos;
                p->level = (size_t)(setext_level ? setext_level : 0);
                p->setext = setext_level ? 1 : 0;
                cm_block_append_child(root, p);
            } else {
                free(para);
            }
        }
    }

    *out_end = i;
    return root;
}

/* Count leading spaces of a line (tabs count as one character for simplicity). */
static size_t cm_indent_of(const char *line, size_t len)
{
    size_t i = 0;
    while (i < len && (line[i] == ' ' || line[i] == '\t'))
        i++;
    return i;
}

/* Parse a list starting at line start.  The list continues while each new
 * item marker has indentation == base_indent and is of the same type as the
 * first item.  Lines indented beyond the item content column become children
 * of the current item.  Returns the list block and writes the index of the
 * first unconsumed line in *out_end. */
static CmBlock *cm_parse_list(const char **lines, size_t *line_lens,
                              size_t line_count, size_t start, size_t *out_end,
                              size_t base_indent)
{
    const char *first = lines[start];
    size_t first_len = line_lens[start];
    char bullet = 0;
    int ordered = 0;
    char ordered_delim = 0;
    size_t first_marker_end = 0;

    {
        size_t k = 0;
        while (k < first_len && (first[k] == ' ' || first[k] == '\t'))
            k++;
        if (k < first_len && (first[k] == '-' || first[k] == '*' || first[k] == '+')) {
            bullet = first[k];
            first_marker_end = k + 1;
        } else {
            ordered = 1;
            size_t d = k;
            while (d < first_len && first[d] >= '0' && first[d] <= '9')
                d++;
            ordered_delim = (d < first_len) ? first[d] : 0;
            first_marker_end = d + 1;
        }
    }

    CmBlock *list = cm_block_new(CM_BLOCK_LIST);
    if (!list)
        return NULL;
    list->level = bullet ? 1 : 2;

    size_t i = start;
    while (i < line_count) {
        const char *l = lines[i];
        size_t ll = line_lens[i];

        if (cm_is_blank(l, ll)) {
            i++;
            continue;
        }

        size_t indent = cm_indent_of(l, ll);
        if (indent < base_indent)
            break;
        if (indent > base_indent)
            break; /* belongs to a parent container, not this list */

        size_t off = 0;
        int is_bullet = cm_is_bullet_item(l, ll, &off);
        int is_ordered = 0;
        if (!is_bullet)
            is_ordered = cm_is_ordered_item(l, ll, &off);
        if (!is_bullet && !is_ordered)
            break;

        if (is_bullet) {
            size_t k = indent;
            if (k < ll)
                k++;
            (void)k;
        }

        if (is_bullet && bullet) {
            char c = l[indent];
            if (c != bullet)
                break;
        } else if (is_ordered && ordered) {
            size_t k = indent;
            while (k < ll && l[k] >= '0' && l[k] <= '9')
                k++;
            char delim = (k < ll) ? l[k] : 0;
            if (delim != ordered_delim)
                break;
        } else {
            break;
        }

        CmBlock *item = cm_block_new(CM_BLOCK_LIST_ITEM);
        if (!item)
            break;
        item->text = cm_dup(l, ll);
        item->text_len = ll;
        item->level = off;
        cm_block_append_child(list, item);
        i++;

        /* Determine the content column: the column of the first non-space
         * character after the marker.  Child blocks must be indented by at
         * least this amount to belong to this item. */
        size_t content_col = off;
        while (content_col < ll && (l[content_col] == ' ' || l[content_col] == '\t'))
            content_col++;

        /* Collect any blank lines before child blocks.  A blank line itself
         * is not stored, but it may separate item content from nested blocks. */
        while (i < line_count && cm_is_blank(lines[i], line_lens[i]))
            i++;

        if (i < line_count && cm_indent_of(lines[i], line_lens[i]) >= content_col) {
            size_t child_end = i;
            CmBlock *children = cm_build_blocks(lines, line_lens, line_count,
                                                i, &child_end, content_col);
            if (children) {
                CmBlock *c = children->first_child;
                while (c) {
                    CmBlock *next = c->next_sibling;
                    c->parent = item;
                    c->next_sibling = NULL;
                    if (!item->first_child) {
                        item->first_child = c;
                        item->last_child = c;
                    } else {
                        item->last_child->next_sibling = c;
                        item->last_child = c;
                    }
                    c = next;
                }
                children->first_child = NULL;
                children->last_child = NULL;
                cm_block_free(children);
                i = child_end;
            }
        }
    }

    *out_end = i;
    return list;
}

/* Parse a block quote starting at line start.  Each line must begin with
 * '>' (after optional leading spaces, which we ignore here).  The stripped
 * content of each line becomes a child paragraph or heading of the quote. */
static CmBlock *cm_parse_blockquote(const char **lines, size_t *line_lens,
                                    size_t line_count, size_t start, size_t *out_end)
{
    CmBlock *quote = cm_block_new(CM_BLOCK_BLOCKQUOTE);
    if (!quote) {
        *out_end = start;
        return NULL;
    }

    size_t i = start;
    while (i < line_count) {
        const char *line = lines[i];
        size_t len = line_lens[i];

        if (cm_is_blank(line, len))
            break;

        size_t indent = 0;
        while (indent < len && (line[indent] == ' ' || line[indent] == '\t'))
            indent++;
        if (indent >= len || line[indent] != '>')
            break;

        /* Strip the '>' and one optional following space. */
        size_t content_start = indent + 1;
        if (content_start < len && line[content_start] == ' ')
            content_start++;
        size_t content_len = len - content_start;
        while (content_len > 0 && line[content_start + content_len - 1] == ' ')
            content_len--;

        CmBlock *child = NULL;
        size_t atx = cm_atx_level(line + content_start, content_len);
        if (atx > 0) {
            child = cm_parse_heading(line + content_start, content_len);
        } else {
            child = cm_block_new(CM_BLOCK_PARAGRAPH);
            if (child) {
                child->text = cm_dup(line + content_start, content_len);
                child->text_len = content_len;
            }
        }
        if (child)
            cm_block_append_child(quote, child);
        i++;
    }

    *out_end = i;
    return quote;
}

/* Try to parse a fenced code block starting at line i. If successful,
 * returns the new line index and stores the block in *out. */
static size_t cm_parse_fenced_code_block(const char **lines, size_t *line_lens,
                                         size_t line_count, size_t i, CmBlock **out)
{
    const char *line = lines[i];
    size_t len = line_lens[i];
    size_t k = 0;
    while (k < len && (line[k] == ' ' || line[k] == '\t'))
        k++;
    if (k > 3)
        return i;
    char fence = line[k];
    if (fence != '`' && fence != '~')
        return i;
    size_t fence_start = k;
    size_t fence_len = 0;
    while (k < len && line[k] == fence) {
        fence_len++;
        k++;
    }
    if (fence_len < 3)
        return i;

    CmBlock *block = cm_block_new(CM_BLOCK_FENCED_CODE);
    if (!block)
        return i;
    block->level = fence_len;

    /* Info string is the rest of the opening line, trimmed. */
    while (k < len && (line[k] == ' ' || line[k] == '\t'))
        k++;
    size_t info_start = k;
    size_t info_end = len;
    while (info_end > info_start &&
           (line[info_end - 1] == ' ' || line[info_end - 1] == '\t'))
        info_end--;
    if (info_end > info_start) {
        block->text = cm_dup(line + info_start, info_end - info_start);
        block->text_len = info_end - info_start;
    }

    size_t j = i + 1;
    while (j < line_count) {
        const char *cl = lines[j];
        size_t cl_len = line_lens[j];
        size_t m = 0;
        while (m < cl_len && (cl[m] == ' ' || cl[m] == '\t'))
            m++;
        if (cl[m] == fence) {
            size_t fl = 0;
            while (m + fl < cl_len && cl[m + fl] == fence)
                fl++;
            if (fl >= fence_len) {
                block->closed = 1;
                j++;
                break;
            }
        }

        /* Store content lines as child text nodes.  Preserve the source
         * indentation; the formatter decides how to indent the block. */
        CmBlock *content = cm_block_new(CM_BLOCK_PARAGRAPH);
        content->text = cm_dup(cl, cl_len);
        content->text_len = cl_len;
        cm_block_append_child(block, content);
        j++;
    }

    *out = block;
    return j;
}

/* Parse a single-line ATX heading. */
static CmBlock *cm_parse_heading(const char *line, size_t len)
{
    size_t level = cm_atx_level(line, len);
    if (level == 0)
        return NULL;

    CmBlock *block = cm_block_new(CM_BLOCK_HEADING);
    if (!block)
        return NULL;
    block->level = level;

    /* Trim leading marker and trailing optional marker. */
    size_t i = 0;
    while (i < len && (line[i] == ' ' || line[i] == '\t'))
        i++;
    while (i < len && line[i] == '#')
        i++;
    while (i < len && (line[i] == ' ' || line[i] == '\t'))
        i++;

    size_t end = len;
    while (end > i && (line[end - 1] == ' ' || line[end - 1] == '\t'))
        end--;
    while (end > i && line[end - 1] == '#')
        end--;
    while (end > i && (line[end - 1] == ' ' || line[end - 1] == '\t'))
        end--;

    block->text = cm_dup(line + i, end - i);
    block->text_len = end - i;
    return block;
}

/* --------------------------------------------------------------------------
 * Phase 2: Inline parser
 * -------------------------------------------------------------------------- */

/* Append a literal text token made of characters that were skipped because
 * they did not form a valid inline construct. */
static void cm_emit_literal(CommonmarkLexer *lex, const char *text, size_t len)
{
    if (len > 0)
        cm_push_text(lex, text, len);
}

/* Parse inline markup inside a heading or paragraph.  Supports emphasis
 * delimited by '*' and code spans delimited by backticks (including runs
 * of multiple backticks).  Unclosed delimiters are emitted as plain text. */
static void cm_parse_inline(CommonmarkLexer *lex, const char *text, size_t len)
{
    size_t i = 0;
    while (i < len) {
        /* Emphasis / strong with '*' runs.  Match opening and closing runs
         * of equal length.  1 asterisk = emphasis, 2+ = strong. */
        if (text[i] == '*') {
            size_t run = 0;
            while (i + run < len && text[i + run] == '*')
                run++;

            /* Find a matching closing run of the same length. */
            size_t close = i + run;
            size_t matched = 0;
            while (close < len) {
                if (text[close] == '*') {
                    size_t r = 0;
                    while (close + r < len && text[close + r] == '*')
                        r++;
                    if (r == run) {
                        matched = r;
                        break;
                    }
                    close += r;
                } else {
                    close++;
                }
            }

            if (matched > 0) {
                FlareTokenType delim_type = (run >= 2) ? HL_MARKUP_INLINE_STRONG : HL_MARKUP_INLINE_EMPHASIS;
                size_t content_start = i + run;
                size_t content_len = close - content_start;

                cm_push(lex, delim_type, text + i, run);
                if (content_len > 0)
                    cm_push_text(lex, text + content_start, content_len);
                cm_push(lex, delim_type, text + close, run);
                i = close + run;
                continue;
            }

            /* No matching closing run: emit the leading asterisks as plain text. */
            cm_emit_literal(lex, text + i, run);
            i += run;
            continue;
        }

        /* Code span with one or more backticks.  Find a matching closing
         * backtick run of the same length, extract the content, and apply
         * CommonMark normalization. */
        if (text[i] == '`') {
            size_t start = i;
            size_t backticks = 0;
            while (i < len && text[i] == '`') {
                backticks++;
                i++;
            }
            size_t content_start = i;
            size_t close = content_start;
            int found = 0;
            while (close < len) {
                if (text[close] == '`') {
                    size_t run = 0;
                    size_t k = close;
                    while (k < len && text[k] == '`') {
                        run++;
                        k++;
                    }
                    if (run == backticks) {
                        found = 1;
                        break;
                    }
                    close = k;
                } else {
                    close++;
                }
            }

            if (found) {
                size_t content_len = close - content_start;
                char *content = malloc(content_len + 1);
                if (content) {
                    /* Copy content and convert line endings to spaces. */
                    for (size_t k = 0; k < content_len; k++) {
                        char c = text[content_start + k];
                        content[k] = (c == '\n' || c == '\r') ? ' ' : c;
                    }
                    content[content_len] = '\0';

                    /* CommonMark normalization: if the content both begins
                     * and ends with a space, and is not entirely spaces,
                     * remove one space from each end. */
                    if (content_len >= 2) {
                        int all_spaces = 1;
                        for (size_t k = 0; k < content_len; k++) {
                            if (content[k] != ' ') {
                                all_spaces = 0;
                                break;
                            }
                        }
                        if (!all_spaces && content[0] == ' ' &&
                            content[content_len - 1] == ' ') {
                            memmove(content, content + 1, content_len - 2);
                            content_len -= 2;
                            content[content_len] = '\0';
                        }
                    }

                    cm_push(lex, HL_MARKUP_INLINE_CODE, content, content_len);
                    free(content);
                }
                i = close + backticks;
                continue;
            }

            /* No matching close: emit the backticks as plain text. */
            cm_emit_literal(lex, text + start, backticks);
            continue;
        }

        /* Scan plain text until the next special character. */
        size_t start = i;
        while (i < len && text[i] != '*' && text[i] != '`')
            i++;
        cm_emit_literal(lex, text + start, i - start);
    }
}

/* --------------------------------------------------------------------------
 * Phase 2: Token emission from block tree
 * -------------------------------------------------------------------------- */

static void cm_emit_block(CommonmarkLexer *lex, const CmBlock *block);

static void cm_emit_children(CommonmarkLexer *lex, const CmBlock *block)
{
    for (CmBlock *child = block->first_child; child; child = child->next_sibling)
        cm_emit_block(lex, child);
}

static void cm_emit_heading(CommonmarkLexer *lex, const CmBlock *block)
{
    char marker[16];
    int marker_len = 0;
    if (block->setext) {
        /* Setext heading marker: '=' for level 1, '-' for level 2. */
        marker[0] = (block->level == 1) ? '=' : '-';
        marker[1] = ' ';
        marker[2] = '\0';
        marker_len = 2;
    } else {
        /* ATX heading marker: hashes plus trailing space. */
        marker_len = snprintf(marker, sizeof(marker), "%.*s ",
                              (int)(block->level < 6 ? block->level : 6),
                              "######");
    }
    if (marker_len > 0)
        cm_push(lex, HL_MARKUP_HEADING_MARKER, marker, (size_t)marker_len);
    if (block->text_len > 0)
        cm_parse_inline(lex, block->text, block->text_len);
    cm_push_newline(lex);
}

static void cm_emit_paragraph(CommonmarkLexer *lex, const CmBlock *block)
{
    cm_push(lex, HL_MARKUP_PARAGRAPH, "", 0);
    if (block->text_len > 0)
        cm_parse_inline(lex, block->text, block->text_len);
    cm_push_newline(lex);
}

static void cm_emit_thematic_break(CommonmarkLexer *lex, const CmBlock *block)
{
    if (block->text_len > 0)
        cm_push(lex, HL_MARKUP_THEMATIC_BREAK, block->text, block->text_len);
    cm_push_newline(lex);
}

static void cm_emit_blockquote(CommonmarkLexer *lex, const CmBlock *block)
{
    for (CmBlock *child = block->first_child; child; child = child->next_sibling) {
        /* Block quote marker: we normalize to "> " for the output. */
        cm_push(lex, HL_MARKUP_BLOCKQUOTE_MARKER, "> ", 2);
        if (child->type == CM_BLOCK_HEADING) {
            char marker[16];
            size_t level = child->level < 6 ? child->level : 6;
            size_t n = 0;
            for (size_t k = 0; k < level; k++)
                marker[n++] = '#';
            marker[n++] = ' ';
            cm_push(lex, HL_MARKUP_HEADING_MARKER, marker, n);
            if (child->text_len > 0)
                cm_parse_inline(lex, child->text, child->text_len);
            cm_push_newline(lex);
        } else if (child->type == CM_BLOCK_PARAGRAPH) {
            if (child->text_len > 0)
                cm_parse_inline(lex, child->text, child->text_len);
            cm_push_newline(lex);
        } else {
            cm_emit_block(lex, child);
        }
    }
}

static void cm_emit_list_item(CommonmarkLexer *lex, const CmBlock *block)
{
    const char *line = block->text;
    size_t len = block->text_len;
    size_t content_off = 0;
    if (cm_is_bullet_item(line, len, &content_off) ||
        cm_is_ordered_item(line, len, &content_off)) {
        /* Skip leading spaces so the marker token is just the marker and
         * delimiter (e.g. "- " or "1. "), regardless of nesting depth. */
        size_t marker_start = 0;
        while (marker_start < len && (line[marker_start] == ' ' || line[marker_start] == '\t'))
            marker_start++;
        size_t marker_end = content_off;
        while (content_off < len && (line[content_off] == ' ' || line[content_off] == '\t'))
            content_off++;
        cm_push(lex, HL_MARKUP_LIST_MARKER, line + marker_start,
                (marker_end > marker_start) ? (marker_end - marker_start) : 0);
        while (content_off < len) {
            size_t end = len;
            while (end > content_off && (line[end - 1] == ' ' || line[end - 1] == '\t'))
                end--;
            cm_parse_inline(lex, line + content_off, end - content_off);
            break;
        }
    }
    cm_push_newline(lex);

    /* Emit any nested blocks (sub-lists, paragraphs, code blocks, etc.) that
     * belong to this list item. */
    cm_emit_children(lex, block);
}

static void cm_emit_fenced_code(CommonmarkLexer *lex, const CmBlock *block)
{
    /* Opening fence line */
    cm_push(lex, HL_MARKUP_FENCED_OPEN, "```", 3);
    if (block->text_len > 0)
        cm_push(lex, HL_MARKUP_FENCED_INFO, block->text, block->text_len);
    cm_push_newline(lex);

    /* Content lines */
    for (CmBlock *child = block->first_child; child; child = child->next_sibling) {
        if (child->text_len > 0)
            cm_push(lex, HL_MARKUP_INDENTED_CODE, child->text, child->text_len);
        cm_push_newline(lex);
    }

    /* Closing fence line */
    cm_push(lex, HL_MARKUP_FENCED_CLOSE, "```", 3);
    cm_push_newline(lex);
}

static void cm_emit_block(CommonmarkLexer *lex, const CmBlock *block)
{
    switch (block->type) {
    case CM_BLOCK_HEADING:
        cm_emit_heading(lex, block);
        break;
    case CM_BLOCK_PARAGRAPH:
        cm_emit_paragraph(lex, block);
        break;
    case CM_BLOCK_THEMATIC_BREAK:
        cm_emit_thematic_break(lex, block);
        break;
    case CM_BLOCK_FENCED_CODE:
        cm_emit_fenced_code(lex, block);
        break;
    case CM_BLOCK_LIST:
        cm_emit_children(lex, block);
        break;
    case CM_BLOCK_LIST_ITEM:
        cm_emit_list_item(lex, block);
        break;
    case CM_BLOCK_BLOCKQUOTE:
        cm_emit_blockquote(lex, block);
        break;
    default:
        /* Unsupported block types emit their children. */
        cm_emit_children(lex, block);
        break;
    }
}

/* --------------------------------------------------------------------------
 * Source reading / line splitting
 * -------------------------------------------------------------------------- */

static int cm_read_all(FlareSource *src, char **out_buf, size_t *out_len)
{
    size_t cap = 4096;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf)
        return -1;

    while (1) {
        ssize_t n = flare_source_read(src, buf + len, cap - len);
        if (n < 0) {
            free(buf);
            return -1;
        }
        if (n == 0)
            break;
        len += (size_t)n;
        if (len == cap) {
            cap *= 2;
            char *tmp = realloc(buf, cap);
            if (!tmp) {
                free(buf);
                return -1;
            }
            buf = tmp;
        }
    }

    if (len == cap) {
        char *tmp = realloc(buf, cap + 1);
        if (!tmp) {
            free(buf);
            return -1;
        }
        buf = tmp;
    }
    buf[len] = '\0';
    *out_buf = buf;
    *out_len = len;
    return 0;
}

static void cm_split_lines(const char *input, size_t input_len,
                           const char ***out_lines, size_t **out_lens,
                           size_t *out_count)
{
    size_t cap = 64;
    const char **lines = malloc(cap * sizeof(char *));
    size_t *lens = malloc(cap * sizeof(size_t));
    size_t count = 0;

    const char *p = input;
    while (p < input + input_len) {
        const char *nl = memchr(p, '\n', input_len - (size_t)(p - input));
        const char *line_start = p;
        size_t line_len;
        if (nl) {
            line_len = (size_t)(nl - p);
            p = nl + 1;
        } else {
            line_len = input_len - (size_t)(p - input);
            p = input + input_len;
        }
        if (line_len > 0 && line_start[line_len - 1] == '\r')
            line_len--;

        if (count >= cap) {
            cap *= 2;
            lines = realloc(lines, cap * sizeof(char *));
            lens = realloc(lens, cap * sizeof(size_t));
        }
        lines[count] = line_start;
        lens[count] = line_len;
        count++;
    }

    *out_lines = lines;
    *out_lens = lens;
    *out_count = count;
}

/* --------------------------------------------------------------------------
 * FlareTokenSource vtable
 * -------------------------------------------------------------------------- */

static int commonmark_pull(FlareTokenSource *src, FlareToken *out)
{
    CommonmarkLexer *lex = (CommonmarkLexer *)src;
    if (lex->pos >= lex->count)
        return 0;
    *out = lex->tokens[lex->pos++];
    return 1;
}

static void commonmark_free(FlareTokenSource *src)
{
    CommonmarkLexer *lex = (CommonmarkLexer *)src;
    if (lex->src)
        flare_source_free(lex->src);
    if (lex->tokens)
        free(lex->tokens);
    free(lex);
}

static const FlareTokenSourceVTable commonmark_vtable = {
    .pull = commonmark_pull,
    .error = NULL,
    .free = commonmark_free,
};

/* --------------------------------------------------------------------------
 * Public constructor
 * -------------------------------------------------------------------------- */

FlareTokenSource *flare_lexer_commonmark(FlareSource *src, Environment *env)
{
    if (!src)
        return NULL;

    CommonmarkLexer *lex = calloc(1, sizeof(CommonmarkLexer));
    if (!lex) {
        flare_source_free(src);
        return NULL;
    }
    lex->base.vtable = &commonmark_vtable;
    lex->src = src;
    lex->env = env;

    char *input = NULL;
    size_t input_len = 0;
    if (cm_read_all(src, &input, &input_len) < 0) {
        flare_source_free(src);
        free(lex);
        return NULL;
    }

    const char **lines = NULL;
    size_t *line_lens = NULL;
    size_t line_count = 0;
    cm_split_lines(input, input_len, &lines, &line_lens, &line_count);

    size_t consumed = 0;
    CmBlock *root = cm_build_blocks(lines, line_lens, line_count, 0, &consumed, 0);
    if (root) {
        cm_emit_children(lex, root);
        cm_block_free(root);
    }

    free(lines);
    free(line_lens);
    free(input);

    return &lex->base;
}
