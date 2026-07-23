/* flare_style_charmtones.c - Flare style sourced from boba CharmTones
 *
 * All colors come from boba's charmtones.h (the CharmTone palette).
 * This file lives in cli/ (not src/) because libditty.a must not
 * depend on boba. Only the ditty binary links boba and can use this style. */

#include <boba/charmtones.h>
#include <boba/style.h>
#include <ditty/highlight.h>

/* Extract RGB from a TuiColor into a FlareStyleEntry's foreground. */
#define CT_FG(ct_call, e)        \
    do {                         \
        TuiColor _c = (ct_call); \
        (e).fg_r = _c.v.rgb.r;   \
        (e).fg_g = _c.v.rgb.g;   \
        (e).fg_b = _c.v.rgb.b;   \
    } while (0)

#define CT_BG(ct_call, e)        \
    do {                         \
        TuiColor _c = (ct_call); \
        (e).bg_r = _c.v.rgb.r;   \
        (e).bg_g = _c.v.rgb.g;   \
        (e).bg_b = _c.v.rgb.b;   \
    } while (0)

FlareStyle *flare_style_charmtones(void)
{
    FlareStyle *s = flare_style_new();
    if (!s)
        return NULL;

    FlareStyleEntry e;

    /* --- Lisp syntax --- */

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_butter(), e);
    flare_style_set(s, HL_TEXT, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_flamingo(), e);
    flare_style_set(s, HL_KEYWORD, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_chili(), e);
    e.bold = 1;
    flare_style_set(s, HL_KEYWORD_DEFINE, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_cheeky(), e);
    flare_style_set(s, HL_KEYWORD_SPECIAL_FORM, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_pony(), e);
    flare_style_set(s, HL_KEYWORD_CONTROL, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_macaron(), e);
    flare_style_set(s, HL_KEYWORD_MACRO_DEF, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_hazy(), e);
    flare_style_set(s, HL_NAME_FUNCTION, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_sardine(), e);
    flare_style_set(s, HL_NAME_BUILTIN, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_butter(), e);
    flare_style_set(s, HL_NAME_VARIABLE, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_flamingo(), e);
    flare_style_set(s, HL_NAME_KEYWORD_ARG, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_zest(), e);
    flare_style_set(s, HL_LITERAL_STRING, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_julep(), e);
    flare_style_set(s, HL_LITERAL_STRING_ESCAPE, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_sardine(), e);
    flare_style_set(s, HL_LITERAL_NUMBER, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_zest(), e);
    flare_style_set(s, HL_LITERAL_CHARACTER, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_flamingo(), e);
    flare_style_set(s, HL_LITERAL_BOOLEAN, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_oyster(), e);
    e.faint = 1;
    flare_style_set(s, HL_LITERAL_NIL, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_flamingo(), e);
    flare_style_set(s, HL_OPERATOR, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_flamingo(), e);
    flare_style_set(s, HL_OPERATOR_QUOTE, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_flamingo(), e);
    flare_style_set(s, HL_OPERATOR_BACKQUOTE, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_flamingo(), e);
    flare_style_set(s, HL_OPERATOR_UNQUOTE, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_flamingo(), e);
    flare_style_set(s, HL_OPERATOR_UNQUOTE_SPLICING, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_smoke(), e);
    flare_style_set(s, HL_PUNCTUATION, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_squid(), e);
    e.italic = 1;
    flare_style_set(s, HL_COMMENT, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_coral(), e);
    e.underline = 1;
    flare_style_set(s, HL_ERROR, &e);

    /* --- Markdown/CommonMark block markup --- */

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_squid(), e);
    flare_style_set(s, HL_MARKUP, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_flamingo(), e);
    e.bold = 1;
    flare_style_set(s, HL_MARKUP_HEADING_MARKER, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_flamingo(), e);
    flare_style_set(s, HL_MARKUP_SETEXT_UNDERLINE, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_squid(), e);
    e.italic = 1;
    flare_style_set(s, HL_MARKUP_FENCED_INFO, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_zest(), e);
    flare_style_set(s, HL_MARKUP_INDENTED_CODE, &e);

    /* --- Markdown/CommonMark inline markup --- */

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_flamingo(), e);
    flare_style_set(s, HL_MARKUP_INLINE, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_butter(), e);
    e.italic = 1;
    flare_style_set(s, HL_MARKUP_INLINE_EMPHASIS, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_butter(), e);
    e.bold = 1;
    flare_style_set(s, HL_MARKUP_INLINE_STRONG, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_julep(), e);
    CT_BG(tui_ct_char(), e);
    flare_style_set(s, HL_MARKUP_INLINE_CODE, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_hazy(), e);
    e.underline = 1;
    flare_style_set(s, HL_MARKUP_INLINE_LINK, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_hazy(), e);
    flare_style_set(s, HL_MARKUP_INLINE_IMAGE, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_hazy(), e);
    flare_style_set(s, HL_MARKUP_INLINE_AUTOLINK, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_squid(), e);
    flare_style_set(s, HL_MARKUP_INLINE_BREAK, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_zest(), e);
    flare_style_set(s, HL_MARKUP_INLINE_ESCAPE, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_zest(), e);
    flare_style_set(s, HL_MARKUP_INLINE_ENTITY, &e);

    e = (FlareStyleEntry){ 0 };
    CT_FG(tui_ct_squid(), e);
    flare_style_set(s, HL_MARKUP_INLINE_HTML, &e);

    return s;
}

#undef CT_FG
#undef CT_BG
