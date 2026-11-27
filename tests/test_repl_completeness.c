/* test_repl_completeness.c - Tests for lisp_read() incomplete input detection
 *
 * Validates that the reader correctly distinguishes complete forms from
 * incomplete (unclosed) input, which is the foundation for the inline REPL's
 * "eval on Enter when form is complete" behavior.
 */

#include "../include/lisp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn)                 \
    do {                             \
        tests_run++;                 \
        fn();                        \
        tests_passed++;              \
        printf("  PASS: %s\n", #fn); \
    } while (0)

#define ASSERT_TRUE(cond, msg)                     \
    do {                                           \
        if (!(cond)) {                             \
            fprintf(stderr, "  FAIL: %s:%d: %s\n", \
                    __FILE__, __LINE__, msg);      \
            abort();                               \
        }                                          \
    } while (0)

/* Test: lisp_read on "(define x" returns unclosed-input error */
static void test_read_incomplete_paren(void)
{
    const char *input = "(define x";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL for incomplete input");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_ERROR, "incomplete paren should be an error");
    ASSERT_TRUE(LISP_ERROR_TYPE(result) == sym_unclosed_input,
                "incomplete paren should be unclosed-input error type");
}

/* Test: lisp_read on "(define x 10)" returns a valid expression */
static void test_read_complete_form(void)
{
    const char *input = "(define x 10)";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL for complete input");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_CONS, "complete form should be a cons cell");
}

/* Test: lisp_read on "(+ 1 2) extra" reads only first form, ptr advances */
static void test_read_stops_at_first_complete_form(void)
{
    const char *input = "(+ 1 2) extra";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_CONS, "first form should be a cons cell");
    ASSERT_TRUE(*ptr == ' ', "ptr should point to space after closing paren");
}

/* Test: lisp_read on "(\"hello" returns unclosed-input (unclosed string) */
static void test_read_incomplete_string(void)
{
    const char *input = "(\"hello";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL for unclosed string");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_ERROR, "unclosed string should be an error");
    ASSERT_TRUE(LISP_ERROR_TYPE(result) == sym_unclosed_input,
                "unclosed string should be unclosed-input error type");
}

/* Test: lisp_read on "#(1 2" returns unclosed-input (unclosed vector) */
static void test_read_incomplete_vector(void)
{
    const char *input = "#(1 2";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL for unclosed vector");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_ERROR, "unclosed vector should be an error");
    ASSERT_TRUE(LISP_ERROR_TYPE(result) == sym_unclosed_input,
                "unclosed vector should be unclosed-input error type");
}

/* Test: lisp_read on a complete atom returns a symbol */
static void test_read_complete_atom(void)
{
    const char *input = "hello";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL for atom");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_SYMBOL, "atom should be a symbol");
}

/* Test: lisp_read on nested complete form "(+ (* 2 3) 1)" works */
static void test_read_nested_complete(void)
{
    const char *input = "(+ (* 2 3) 1)";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL for nested form");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_CONS, "nested complete form should be a cons cell");
}

/* Test: lisp_read on nested incomplete "(+ (* 2 3)" returns unclosed */
static void test_read_nested_incomplete(void)
{
    const char *input = "(+ (* 2 3)";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL for nested incomplete");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_ERROR, "nested incomplete should be an error");
    ASSERT_TRUE(LISP_ERROR_TYPE(result) == sym_unclosed_input,
                "nested incomplete should be unclosed-input");
}

/* Test: lisp_read on whitespace-only input returns NULL */
static void test_read_whitespace_only(void)
{
    const char *input = "   \t  \n  ";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result == NULL, "whitespace-only should return NULL");
}

/* Test: lisp_read on "'(" returns unclosed-input (quote + unclosed list) */
static void test_read_quote_unclosed(void)
{
    const char *input = "'(";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL for '(");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_ERROR, "'( should be an error");
    ASSERT_TRUE(LISP_ERROR_TYPE(result) == sym_unclosed_input,
                "'( should be unclosed-input, not wrapped in quote");
}

/* Test: lisp_read on "`(" returns unclosed-input (backquote + unclosed list) */
static void test_read_backquote_unclosed(void)
{
    const char *input = "`(";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL for `(");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_ERROR, "`( should be an error");
    ASSERT_TRUE(LISP_ERROR_TYPE(result) == sym_unclosed_input,
                "`( should be unclosed-input, not wrapped in quasiquote");
}

/* Test: lisp_read on ",(" returns unclosed-input (unquote + unclosed list) */
static void test_read_unquote_unclosed(void)
{
    const char *input = ",(";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL for ,(");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_ERROR, ",( should be an error");
    ASSERT_TRUE(LISP_ERROR_TYPE(result) == sym_unclosed_input,
                ",( should be unclosed-input, not wrapped in unquote");
}

/* Test: lisp_read on ",@(" returns unclosed-input (unquote-splicing + unclosed) */
static void test_read_unquote_splicing_unclosed(void)
{
    const char *input = ",@(";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL for ,@(");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_ERROR, ",@( should be an error");
    ASSERT_TRUE(LISP_ERROR_TYPE(result) == sym_unclosed_input,
                ",@( should be unclosed-input, not wrapped in unquote-splicing");
}

/* Test: a string ending in a lone backslash is unclosed-input.
 * Regression: the old escape default case consumed the NUL terminator
 * and read past the end of the input buffer. */
static void test_read_string_trailing_backslash(void)
{
    const char *input = "\"abc\\";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL for trailing backslash");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_ERROR, "trailing backslash should be an error");
    ASSERT_TRUE(LISP_ERROR_TYPE(result) == sym_unclosed_input,
                "trailing backslash should be unclosed-input");
}

/* Test: hex escapes truncated at end of input are unclosed-input, so the
 * REPL continues the line instead of evaluating a malformed form. */
static void test_read_string_truncated_hex_escape(void)
{
    const char *inputs[] = { "\"\\x1", "\"\\u12", "\"\\U0001F6" };
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        const char *ptr = inputs[i];
        LispObject *result = lisp_read(&ptr);

        ASSERT_TRUE(result != NULL, "read should return non-NULL for truncated escape");
        ASSERT_TRUE(LISP_TYPE(result) == LISP_ERROR, "truncated escape should be an error");
        ASSERT_TRUE(LISP_ERROR_TYPE(result) == sym_unclosed_input,
                    "truncated hex escape should be unclosed-input");
    }
}

/* Test: malformed escapes in a complete string are reader-error, NOT
 * unclosed-input: the form is complete, just invalid. */
static void test_read_string_bad_escape(void)
{
    const char *inputs[] = {
        "\"\\q\"",        /* unknown escape */
        "\"\\x1\"",       /* \x needs exactly 2 hex digits */
        "\"\\u12x4\"",    /* \u needs exactly 4 hex digits */
        "\"\\ud800\"",    /* surrogate codepoint */
        "\"\\U00110000\"" /* beyond U+10FFFF */
    };
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        const char *ptr = inputs[i];
        LispObject *result = lisp_read(&ptr);

        ASSERT_TRUE(result != NULL, "read should return non-NULL for bad escape");
        ASSERT_TRUE(LISP_TYPE(result) == LISP_ERROR, "bad escape should be an error");
        ASSERT_TRUE(LISP_ERROR_TYPE(result) == sym_reader_error,
                    "bad escape should be reader-error, not unclosed-input");
    }
}

/* Test: hex escapes are fixed-width — "\x41bc" reads as "Abc", not U+41BC */
static void test_read_string_hex_fixed_width(void)
{
    const char *input = "\"\\x41bc\"";
    const char *ptr = input;
    LispObject *result = lisp_read(&ptr);

    ASSERT_TRUE(result != NULL, "read should return non-NULL");
    ASSERT_TRUE(LISP_TYPE(result) == LISP_STRING, "hex escape string should read as a string");
    ASSERT_TRUE(strcmp(LISP_STR_VAL(result), "Abc") == 0,
                "\\x41bc should be \"Abc\" (exactly two hex digits)");
    ASSERT_TRUE(*ptr == '\0', "ptr should advance past the closing quote");
}

int main(void)
{
    lisp_init();

    RUN_TEST(test_read_incomplete_paren);
    RUN_TEST(test_read_complete_form);
    RUN_TEST(test_read_stops_at_first_complete_form);
    RUN_TEST(test_read_incomplete_string);
    RUN_TEST(test_read_incomplete_vector);
    RUN_TEST(test_read_complete_atom);
    RUN_TEST(test_read_nested_complete);
    RUN_TEST(test_read_nested_incomplete);
    RUN_TEST(test_read_whitespace_only);
    RUN_TEST(test_read_quote_unclosed);
    RUN_TEST(test_read_backquote_unclosed);
    RUN_TEST(test_read_unquote_unclosed);
    RUN_TEST(test_read_unquote_splicing_unclosed);
    RUN_TEST(test_read_string_trailing_backslash);
    RUN_TEST(test_read_string_truncated_hex_escape);
    RUN_TEST(test_read_string_bad_escape);
    RUN_TEST(test_read_string_hex_fixed_width);

    lisp_cleanup();

    printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
