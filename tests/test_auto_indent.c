/* test_auto_indent.c - Tests for auto-indentation computation
 *
 * When the user presses Enter on an incomplete form, the continuation
 * line should be auto-indented based on the nesting depth of open parens.
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

#define ASSERT_EQ(a, b, msg)                                            \
    do {                                                                \
        if ((a) != (b)) {                                               \
            fprintf(stderr, "  FAIL: %s:%d: %s: expected %d, got %d\n", \
                    __FILE__, __LINE__, msg, (int)(b), (int)(a));       \
            abort();                                                    \
        }                                                               \
    } while (0)

/* Count the number of unclosed parens in a string (ignoring strings/comments) */
static int count_unclosed_parens(const char *text)
{
    int depth = 0;
    int in_string = 0;
    int in_comment = 0;

    for (const char *p = text; *p; p++) {
        if (in_comment) {
            if (p[0] == '\n')
                in_comment = 0;
            continue;
        }
        if (in_string) {
            if (*p == '\\' && p[1])
                p++;
            else if (*p == '"')
                in_string = 0;
            continue;
        }
        if (*p == ';') {
            in_comment = 1;
            continue;
        }
        if (*p == '"') {
            in_string = 1;
            continue;
        }
        if (*p == '(')
            depth++;
        else if (*p == ')')
            depth--;
    }
    return depth < 0 ? 0 : depth;
}

/* Compute auto-indent for a continuation line.
 * Returns the number of spaces to indent. */
static int compute_auto_indent(const char *text)
{
    int depth = count_unclosed_parens(text);
    if (depth <= 0)
        return 0;
    /* Indent 2 spaces per nesting level */
    return depth * 2;
}

static void test_count_no_parens(void)
{
    ASSERT_EQ(count_unclosed_parens("hello"), 0, "no parens");
    ASSERT_EQ(count_unclosed_parens("+ 1 2"), 0, "no parens with spaces");
}

static void test_count_balanced(void)
{
    ASSERT_EQ(count_unclosed_parens("(+ 1 2)"), 0, "balanced parens");
    ASSERT_EQ(count_unclosed_parens("(define (f x) (+ x 1))"), 0, "nested balanced");
}

static void test_count_unclosed(void)
{
    ASSERT_EQ(count_unclosed_parens("(define"), 1, "one unclosed paren");
    ASSERT_EQ(count_unclosed_parens("(define (fib n)"), 1, "outer paren unclosed, inner closed");
    ASSERT_EQ(count_unclosed_parens("(define (fib"), 2, "two unclosed parens");
    ASSERT_EQ(count_unclosed_parens("(+ (* 2 3)"), 1, "one unclosed after nested close");
}

static void test_count_ignores_strings(void)
{
    ASSERT_EQ(count_unclosed_parens("(display \"hello (world\""), 1, "paren in string ignored");
    ASSERT_EQ(count_unclosed_parens("(display \")\")"), 0, "close paren in string ignored");
}

static void test_count_ignores_comments(void)
{
    ASSERT_EQ(count_unclosed_parens("(define ; (comment\n x)"), 0, "parens in comment ignored");
    ASSERT_EQ(count_unclosed_parens("(define x ; open paren in comment (\n 10)"), 0, "balanced with comment");
}

static void test_auto_indent_no_parens(void)
{
    ASSERT_EQ(compute_auto_indent("hello"), 0, "no parens → no indent");
}

static void test_auto_indent_one_level(void)
{
    ASSERT_EQ(compute_auto_indent("(define"), 2, "one paren → 2 spaces");
}

static void test_auto_indent_two_levels(void)
{
    ASSERT_EQ(compute_auto_indent("(define (fib"), 4, "two parens → 4 spaces");
}

static void test_auto_indent_balanced(void)
{
    ASSERT_EQ(compute_auto_indent("(+ 1 2)"), 0, "balanced → no indent");
}

int main(void)
{
    printf("auto-indent tests:\n");

    RUN_TEST(test_count_no_parens);
    RUN_TEST(test_count_balanced);
    RUN_TEST(test_count_unclosed);
    RUN_TEST(test_count_ignores_strings);
    RUN_TEST(test_count_ignores_comments);
    RUN_TEST(test_auto_indent_no_parens);
    RUN_TEST(test_auto_indent_one_level);
    RUN_TEST(test_auto_indent_two_levels);
    RUN_TEST(test_auto_indent_balanced);

    printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
