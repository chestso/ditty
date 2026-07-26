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

echo ""
echo "All tests passed!"
