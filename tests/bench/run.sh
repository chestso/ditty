#!/bin/sh
# Run the interpreter benchmarks and report the median wall-clock time.
#
# Usage: tests/bench/run.sh [DITTY_BIN] [RUNS]
#
# Not part of `make check` — timing noise does not belong in CI.
# Use a release build (../configure --enable-release) for meaningful numbers.

DITTY_BIN="${1:-./build/cli/ditty}"
RUNS="${2:-5}"

now_ns() {
    date +%s%N
}

median() {
    # Sort numeric values, print the middle one (lower middle for even counts).
    sort -n | awk '{ v[NR] = $1 } END { print v[int((NR + 1) / 2)] }'
}

if [ ! -x "$DITTY_BIN" ]; then
    echo "run.sh: no executable at $DITTY_BIN" >&2
    exit 1
fi

for bench in fib loop signal; do
    file="$(dirname "$0")/$bench.lisp"
    times=""
    i=0
    while [ "$i" -lt "$RUNS" ]; do
        start=$(now_ns)
        "$DITTY_BIN" "$file" >/dev/null
        end=$(now_ns)
        times="$times $(( (end - start) / 1000000 ))"
        i=$((i + 1))
    done
    med=$(printf '%s\n' $times | median)
    printf '%-8s median %6s ms   runs:%s\n' "$bench" "$med" "$times"
done
