#!/bin/bash
# Test: a stray ')' after a complete form is an error, never a silent stop.
# The reader does not advance past a ')' glued to the previous form, and the
# script and -e runners used to treat "no progress" as end of input: the rest
# of the file was dropped and ditty exited 0, so a broken test file passed.

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

DITTY_BIN="${DITTY_BIN:-}"
if [ -z "$DITTY_BIN" ]; then
    for candidate in "$PROJECT_ROOT/build/cli/ditty" "$PROJECT_ROOT/cli/ditty"; do
        if [ -x "$candidate" ]; then
            DITTY_BIN="$candidate"
            break
        fi
    done
fi
if [ ! -x "$DITTY_BIN" ]; then
    echo "FAIL: ditty not found" >&2
    exit 1
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# run NAME MODE CODE: run CODE as a script file or as -e, capturing stdout,
# stderr and the exit status in $out, $err and $rc.
run() {
    local mode="$2" code="$3"
    if [ "$mode" = file ]; then
        printf '%s' "$code" > "$TMP/script.lisp"
        "$DITTY_BIN" "$TMP/script.lisp" > "$TMP/out" 2> "$TMP/err" && rc=0 || rc=$?
    else
        "$DITTY_BIN" -e "$code" > "$TMP/out" 2> "$TMP/err" && rc=0 || rc=$?
    fi
    out="$(cat "$TMP/out")"
    err="$(cat "$TMP/err")"
}

fail() {
    echo "FAIL: $1"
    echo "  exit: $rc"
    echo "  stdout: $out"
    echo "  stderr: $err"
    exit 1
}

# A stray ')' glued to the previous form, as in "(list x 1)))".
glued='(princ "one ")
(define (f x)
  (list x 1)))
(princ "two ")
'

for mode in file eval; do
    run glued "$mode" "$glued"
    [ "$rc" -eq 1 ] || fail "$mode: glued stray ')' should exit 1"
    echo "$err" | grep -q "Unexpected ')'" || fail "$mode: glued stray ')' should report Unexpected ')'"
    echo "$out" | grep -q "two" && fail "$mode: forms after the stray ')' must not run"
    echo "$out" | grep -q "one" || fail "$mode: forms before the stray ')' still run"
    echo "PASS: $mode: glued stray ')'"
done

# A ')' alone at the start of a line was already an error; keep it one.
run own-line file '(princ "one ")
)
(princ "two ")
'
[ "$rc" -eq 1 ] || fail "own-line stray ')' should exit 1"
echo "$err" | grep -q "Unexpected ')'" || fail "own-line stray ')' should report Unexpected ')'"
echo "PASS: file: stray ')' on its own line"

# An unclosed list is an error too.
run unclosed file '(princ "one ")
(define (f x) (list x 1)
'
[ "$rc" -eq 1 ] || fail "unclosed list should exit 1"
echo "$err" | grep -q "Unclosed list" || fail "unclosed list should report Unclosed list"
echo "PASS: file: unclosed list"

# Valid input still runs to the end: trailing comments, blank lines and
# whitespace must not be mistaken for a parse failure.
valid='(princ "a")
; a comment between forms
(princ "b")   

; a trailing comment

'
run valid file "$valid"
[ "$rc" -eq 0 ] || fail "valid script should exit 0"
[ "$out" = "ab" ] || fail "valid script should run every form (expected 'ab')"
echo "PASS: file: valid script with comments and blank lines"

run comment-only file '; nothing but a comment
'
[ "$rc" -eq 0 ] || fail "comment-only script should exit 0"
echo "PASS: file: comment-only script"

# -e also echoes each form's value, so look for the second form's output.
run valid-e eval '(princ "a") (princ "b") ; trailing comment'
[ "$rc" -eq 0 ] || fail "valid -e should exit 0"
echo "$out" | grep -q '^b' || fail "valid -e should run every form"
echo "PASS: eval: valid code"

echo "PASS: script_stray_paren"
