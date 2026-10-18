/* colors.h - the REPL's color roles
 *
 * The palette is the Dracula specification (https://draculatheme.com/spec),
 * the same one the highlighter paints with (flare_style_dracula() in
 * libditty, src/flare_style_dracula.c). Two tables, one palette: this file
 * holds the values for the REPL's chrome and its printed results, the style
 * holds them for highlighted Lisp.
 *
 * Roles, not raw colors, belong at call sites - grep here when a role's
 * color changes.
 */

#ifndef DITTY_BIN_COLORS_H
#define DITTY_BIN_COLORS_H

/* ----- Palette (draculatheme.com/spec) ---------------------------------- */

/* #F8F8F2 Foreground */
#define DRACULA_FG_R 248
#define DRACULA_FG_G 248
#define DRACULA_FG_B 242

/* #6272A4 Comment */
#define DRACULA_COMMENT_R 98
#define DRACULA_COMMENT_G 114
#define DRACULA_COMMENT_B 164

/* #FF79C6 Pink */
#define DRACULA_PINK_R 255
#define DRACULA_PINK_G 121
#define DRACULA_PINK_B 198

/* #BD93F9 Purple */
#define DRACULA_PURPLE_R 189
#define DRACULA_PURPLE_G 147
#define DRACULA_PURPLE_B 249

/* #FFB86C Orange */
#define DRACULA_ORANGE_R 255
#define DRACULA_ORANGE_G 184
#define DRACULA_ORANGE_B 108

/* #F1FA8C Yellow */
#define DRACULA_YELLOW_R 241
#define DRACULA_YELLOW_G 250
#define DRACULA_YELLOW_B 140

/* #FF5555 Red */
#define DRACULA_RED_R 255
#define DRACULA_RED_G 85
#define DRACULA_RED_B 85

/* #44475A Selection */
#define DRACULA_SELECTION_R 68
#define DRACULA_SELECTION_G 71
#define DRACULA_SELECTION_B 90

/* ----- Roles ------------------------------------------------------------ */

/* Prompt ">>>" - the app accent. */
#define COLOR_PROMPT_R DRACULA_PINK_R
#define COLOR_PROMPT_G DRACULA_PINK_G
#define COLOR_PROMPT_B DRACULA_PINK_B

/* Divider rule between output and the next prompt. */
#define COLOR_DIVIDER_R DRACULA_COMMENT_R
#define COLOR_DIVIDER_G DRACULA_COMMENT_G
#define COLOR_DIVIDER_B DRACULA_COMMENT_B

/* Error messages. */
#define COLOR_ERROR_R DRACULA_RED_R
#define COLOR_ERROR_G DRACULA_RED_G
#define COLOR_ERROR_B DRACULA_RED_B

/* Welcome and informational text. */
#define COLOR_INFO_R DRACULA_COMMENT_R
#define COLOR_INFO_G DRACULA_COMMENT_G
#define COLOR_INFO_B DRACULA_COMMENT_B

/* Printed nil - absence, so muted. */
#define COLOR_NIL_R DRACULA_COMMENT_R
#define COLOR_NIL_G DRACULA_COMMENT_G
#define COLOR_NIL_B DRACULA_COMMENT_B

/* Printed numbers. */
#define COLOR_NUMBER_R DRACULA_ORANGE_R
#define COLOR_NUMBER_G DRACULA_ORANGE_G
#define COLOR_NUMBER_B DRACULA_ORANGE_B

/* Printed strings. */
#define COLOR_STRING_R DRACULA_YELLOW_R
#define COLOR_STRING_G DRACULA_YELLOW_G
#define COLOR_STRING_B DRACULA_YELLOW_B

/* Printed symbols. */
#define COLOR_SYMBOL_R DRACULA_FG_R
#define COLOR_SYMBOL_G DRACULA_FG_G
#define COLOR_SYMBOL_B DRACULA_FG_B

/* Printed callables (lambdas, macros, builtins). */
#define COLOR_FUNCTION_R DRACULA_PURPLE_R
#define COLOR_FUNCTION_G DRACULA_PURPLE_G
#define COLOR_FUNCTION_B DRACULA_PURPLE_B

/* Everything else a value can print as (lists, vectors, tables, ...). */
#define COLOR_RESULT_R DRACULA_FG_R
#define COLOR_RESULT_G DRACULA_FG_G
#define COLOR_RESULT_B DRACULA_FG_B

#endif /* DITTY_BIN_COLORS_H */
