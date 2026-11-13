#!/bin/bash
# Test: UTF-8 terminal input arrives as one key event over a real pipe.
# The deterministic split coverage lives in tests/basic/terminal_loop.lisp;
# this checks the whole read-byte/terminal-wait path end to end.

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

SCRIPT="$PROJECT_ROOT/tests/fixtures/terminal_keys.lisp"
EXPECTED="b|l|å|b|æ|r|"

check() {
    if [ "$2" != "$EXPECTED" ]; then
        echo "FAIL: $1 - expected '$EXPECTED', got '$2'"
        exit 1
    fi
}

output=$(printf 'bl\303\245b\303\246r' | "$DITTY_BIN" "$SCRIPT")
check "whole write" "$output"

# Split inside the å (C3 A5). The sleep is best effort: the byte queue may
# coalesce the writes, but when it does not the decoder must still recover.
output=$({ printf 'bl\303'; sleep 0.3; printf '\245b\303\246r'; } | "$DITTY_BIN" "$SCRIPT")
check "split write" "$output"

echo "PASS: terminal_utf8_pipe"
