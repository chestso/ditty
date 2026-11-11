#!/bin/bash
# Test: caught errors should not auto-print ERROR: prefix
# These tests verify the print suppression behavior that can't be tested from within Lisp

set -e

DITTY_BIN="${DITTY_BIN:-./build/cli/ditty}"

echo "Test 1: Caught error returns handler value, no ERROR: prefix"
output=$("$DITTY_BIN" -e "(condition-case err (error 'test) (error 'caught))" 2>&1)
if echo "$output" | grep -q "ERROR:"; then
    echo "FAIL: Test 1 - ERROR: prefix found in output"
    echo "Output: $output"
    exit 1
fi
if [ "$output" != "caught" ]; then
    echo "FAIL: Test 1 - expected 'caught', got '$output'"
    exit 1
fi
echo "PASS: Test 1"

echo "Test 2: Handler returns caught error object - should not print ERROR:"
output=$("$DITTY_BIN" -e "(condition-case err (error 'test) (error err))" 2>&1)
if echo "$output" | grep -q "ERROR:"; then
    echo "FAIL: Test 2 - ERROR: prefix found when handler returns error object"
    echo "Output: $output"
    exit 1
fi
echo "PASS: Test 2"

echo "Test 3: Define caught error, should not print ERROR:"
output=$("$DITTY_BIN" -e "(define e (condition-case err (error 'test) (error err)))" 2>&1)
if echo "$output" | grep -q "ERROR:"; then
    echo "FAIL: Test 3 - ERROR: prefix found on define with caught error"
    echo "Output: $output"
    exit 1
fi
echo "PASS: Test 3"

echo "Test 4: Uncaught error still prints ERROR: and exits 1"
output=$("$DITTY_BIN" -e "(error 'uncaught)" 2>&1) && rc=0 || rc=$?
if [ $rc -eq 0 ]; then
    echo "FAIL: Test 4 - uncaught error should exit non-zero"
    exit 1
fi
if ! echo "$output" | grep -q "ERROR:"; then
    echo "FAIL: Test 4 - uncaught error should print ERROR: prefix"
    echo "Output: $output"
    exit 1
fi
echo "PASS: Test 4"

echo "Test 5: Multiple expressions, caught error in middle"
output=$("$DITTY_BIN" -e "(+ 1 2) (condition-case e (error 'x) (error nil)) (+ 3 4)" 2>&1)
if echo "$output" | grep -q "ERROR:"; then
    echo "FAIL: Test 5 - ERROR: found in multi-expression output"
    echo "Output: $output"
    exit 1
fi
# Should have 3 and 7 as outputs (caught error returns nil which prints nothing)
echo "PASS: Test 5"

echo "Test 6: Caught error in tail-recursive loop"
output=$("$DITTY_BIN" -e "(condition-case err (let loop () (error 'x) (loop)) (error 'caught))" 2>&1)
if echo "$output" | grep -q "ERROR:"; then
    echo "FAIL: Test 6 - ERROR: found for caught error in tail call"
    echo "Output: $output"
    exit 1
fi
if [ "$output" != "caught" ]; then
    echo "FAIL: Test 6 - expected 'caught', got '$output'"
    exit 1
fi
echo "PASS: Test 6"

echo "Test 7: A captured error object prints as a value and exits 0"
output=$("$DITTY_BIN" -e "(condition-case e (error \"x\") (error e))" 2>&1) && rc=0 || rc=$?
if [ $rc -ne 0 ]; then
    echo "FAIL: Test 7 - captured error value should exit 0, got $rc"
    exit 1
fi
case "$output" in
    "#<error"*) ;;
    *)
        echo "FAIL: Test 7 - expected a #<error value, got '$output'"
        exit 1
        ;;
esac
if echo "$output" | grep -q "ERROR:"; then
    echo "FAIL: Test 7 - captured error value printed a diagnostic"
    exit 1
fi
echo "PASS: Test 7"

echo "Test 8: The nested-handler crash is gone"
output=$("$DITTY_BIN" -e "(define (boom) (error \"inner\")) (condition-case o (condition-case i (boom) (error (boom))) (error 'outer))" 2>&1) && rc=0 || rc=$?
if [ $rc -ne 0 ]; then
    echo "FAIL: Test 8 - nested raising handlers should exit 0, got $rc"
    echo "Output: $output"
    exit 1
fi
if [ "$(echo "$output" | tail -n 1)" != "outer" ]; then
    echo "FAIL: Test 8 - expected 'outer' as the last line, got '$output'"
    exit 1
fi
echo "PASS: Test 8"

echo "Test 9: A caught error across a builtin does not break a later uncaught one"
output=$("$DITTY_BIN" -e "(condition-case e (map (lambda (x) (error \"i\")) (list 1)) (error 0)) (error \"later\")" 2>&1) && rc=0 || rc=$?
if [ $rc -eq 0 ]; then
    echo "FAIL: Test 9 - the later uncaught error should exit non-zero"
    exit 1
fi
if ! echo "$output" | grep -q "ERROR:"; then
    echo "FAIL: Test 9 - the later error should print ERROR:"
    echo "Output: $output"
    exit 1
fi
if echo "$output" | grep -qi "internal error\|abort"; then
    echo "FAIL: Test 9 - stale frame state caused an internal error"
    echo "Output: $output"
    exit 1
fi
echo "PASS: Test 9"

echo "Test 10: Cleanups run when an error is not caught"
output=$("$DITTY_BIN" -e "(unwind-protect (error \"x\") (princ \"cleanup\"))" 2>&1) && rc=0 || rc=$?
if [ $rc -eq 0 ]; then
    echo "FAIL: Test 10 - an uncaught error should exit non-zero"
    exit 1
fi
if ! echo "$output" | grep -q "cleanup"; then
    echo "FAIL: Test 10 - cleanup did not run"
    echo "Output: $output"
    exit 1
fi
echo "PASS: Test 10"

echo ""
echo "All tests passed!"
