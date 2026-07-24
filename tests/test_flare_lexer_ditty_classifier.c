/* test_flare_lexer_ditty_classifier.c - TDD tests for ditty atom classifier */

#include "flare_testkit.h"
#include "ditty/flare_lexer_ditty_classifier.h"
#include "ditty/highlight.h"
#include "lisp.h"
#include <string.h>

static Environment *g_env;

static void setup(void)
{
    g_env = lisp_init();
    ASSERT_NOT_NULL(g_env);
}

static void teardown(void)
{
    lisp_cleanup();
    g_env = NULL;
}

/* Literals */
static void test_classify_nil(void)
{
    setup();
    ASSERT_EQ(ditty_classify_atom("nil", 3, g_env), HL_LITERAL_NIL);
    teardown();
}

static void test_classify_true(void)
{
    setup();
    ASSERT_EQ(ditty_classify_atom("#t", 2, g_env), HL_LITERAL_BOOLEAN);
    teardown();
}

static void test_classify_false(void)
{
    setup();
    ASSERT_EQ(ditty_classify_atom("#f", 2, g_env), HL_LITERAL_NIL);
    teardown();
}

/* Numbers */
static void test_classify_integer(void)
{
    setup();
    ASSERT_EQ(ditty_classify_atom("42", 2, g_env), HL_LITERAL_NUMBER);
    teardown();
}

static void test_classify_negative_integer(void)
{
    setup();
    ASSERT_EQ(ditty_classify_atom("-7", 2, g_env), HL_LITERAL_NUMBER);
    teardown();
}

static void test_classify_float(void)
{
    setup();
    ASSERT_EQ(ditty_classify_atom("3.14", 4, g_env), HL_LITERAL_NUMBER);
    teardown();
}

/* Keyword argument */
static void test_classify_keyword_arg(void)
{
    setup();
    ASSERT_EQ(ditty_classify_atom(":foo", 4, g_env), HL_NAME_KEYWORD_ARG);
    teardown();
}

/* Special forms */
static void test_classify_define(void)
{
    setup();
    ASSERT_EQ(ditty_classify_atom("define", 6, g_env), HL_KEYWORD_DEFINE);
    teardown();
}

static void test_classify_if(void)
{
    setup();
    ASSERT_EQ(ditty_classify_atom("if", 2, g_env), HL_KEYWORD_CONTROL);
    teardown();
}

/* Builtins */
static void test_classify_builtin(void)
{
    setup();
    ASSERT_EQ(ditty_classify_atom("+", 1, g_env), HL_NAME_BUILTIN);
    ASSERT_EQ(ditty_classify_atom("=", 1, g_env), HL_NAME_BUILTIN);
    teardown();
}

/* Variables */
static void test_classify_variable(void)
{
    setup();
    ASSERT_EQ(ditty_classify_atom("factorial", 9, g_env), HL_NAME_VARIABLE);
    ASSERT_EQ(ditty_classify_atom("x", 1, g_env), HL_NAME_VARIABLE);
    teardown();
}

/* Function defined after evaluation becomes a function */
static void test_classify_user_function(void)
{
    setup();
    /* Before definition it's a variable */
    ASSERT_EQ(ditty_classify_atom("my-func", 7, g_env), HL_NAME_VARIABLE);

    /* Define it via the evaluator */
    const char *def = "(define (my-func x) x)";
    LispObject *expr = lisp_read(&def);
    ASSERT_NOT_NULL(expr);
    lisp_eval(expr, g_env);

    ASSERT_EQ(ditty_classify_atom("my-func", 7, g_env), HL_NAME_FUNCTION);
    teardown();
}

int main(void)
{
    RUN_TEST(test_classify_nil);
    RUN_TEST(test_classify_true);
    RUN_TEST(test_classify_false);
    RUN_TEST(test_classify_integer);
    RUN_TEST(test_classify_negative_integer);
    RUN_TEST(test_classify_float);
    RUN_TEST(test_classify_keyword_arg);
    RUN_TEST(test_classify_define);
    RUN_TEST(test_classify_if);
    RUN_TEST(test_classify_builtin);
    RUN_TEST(test_classify_variable);
    RUN_TEST(test_classify_user_function);

    TEST_SUMMARY();
    return 0;
}
