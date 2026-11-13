#!/bin/bash
# Test: terminal output reaches the descriptor complete and in order.
# - terminal-write is byte-exact, ordered against princ, and survives a
#   reader that starts late (the write has to wait for the pipe to drain).
# - terminal-call-with-raw emits its mode sequences in the right order, with
#   :no-wrap innermost: set last on entry, restored first on exit.

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

export DITTY_LISP_PATH="$PROJECT_ROOT/lisp${DITTY_LISP_PATH:+:$DITTY_LISP_PATH}"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

FIXTURES="$PROJECT_ROOT/tests/fixtures"

# --- terminal-write: bytes and order -------------------------------------
"$DITTY_BIN" "$FIXTURES/terminal_write.lisp" > "$TMP/write.out"
{
    printf 'abc'
    head -c 5000 /dev/zero | tr '\0' x
    printf 'l1\nl2\303\251\033[0m'
    printf 'd'
} > "$TMP/write.expected"

if ! cmp -s "$TMP/write.out" "$TMP/write.expected"; then
    echo "FAIL: terminal-write output differs from the expected bytes"
    cmp "$TMP/write.out" "$TMP/write.expected" || true
    exit 1
fi
echo "PASS: terminal-write bytes and order"

# --- terminal-write: a late reader ----------------------------------------
# The pipe holds 64 KiB, so a one-megabyte write blocks until the reader
# starts; every byte must still arrive.
count=$("$DITTY_BIN" "$FIXTURES/terminal_write_large.lisp" | { sleep 0.3; wc -c; })
count=$(echo "$count" | tr -d ' ')
if [ "$count" != "1000000" ]; then
    echo "FAIL: expected 1000000 bytes from a late reader, got '$count'"
    exit 1
fi
echo "PASS: terminal-write with a late reader"

# --- sessions: order of the mode sequences ---------------------------------
"$DITTY_BIN" "$FIXTURES/terminal_session.lisp" > "$TMP/session.out"
{
    # alt screen and hidden cursor, then :no-wrap last; undone in reverse
    printf '\033[?1049h\033[?25l\033[?7lA\033[?7h\033[?25h\033[?1049l'
    printf '\033[?7lB\033[?7h'
    printf 'C'
    # an error inside the thunk still restores everything, then the handler runs
    printf '\033[?25l\033[?7lD\033[?7h\033[?25hE'
} > "$TMP/session.expected"

if ! cmp -s "$TMP/session.out" "$TMP/session.expected"; then
    echo "FAIL: session escape sequences differ from the expected order"
    cmp "$TMP/session.out" "$TMP/session.expected" || true
    exit 1
fi
echo "PASS: session mode order"

echo "PASS: terminal_output_pipe"
