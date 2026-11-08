#!/bin/bash
# gen-builtin-ref.sh - Generate BUILTIN_REFERENCE.md from docstrings/*.md files
#
# Assembles all per-function documentation from docstrings/*.md into a single
# reference file with a table of contents. Headings are demoted by one
# level (H1 -> H2 for category titles, H2 -> H3 for function entries,
# H3 -> H4 for subsections) so the output is a valid single-document
# hierarchy with one H1 title at the top.
#
# Usage: scripts/gen-builtin-ref.sh doc > BUILTIN_REFERENCE.md

set -e

DOC_DIR="${1:?Usage: $0 <doc-directory>}"

# Collect sorted list of markdown files
md_files=$(find "$DOC_DIR" -name '*.md' -type f | sort)

if [ -z "$md_files" ]; then
	echo "Error: no .md files found in $DOC_DIR" >&2
	exit 1
fi

# --- Header and title ---

cat <<'HEADER'
# Built-in Function Reference

This reference is auto-generated from `docstrings/*.md`. Each section corresponds to a
documentation category, and each subsection documents a single built-in function,
macro, or variable. The source files in `docstrings/` are also used to generate runtime
docstrings via `scripts/gen-docstrings.sh`.

## Table of Contents

HEADER

# --- Table of contents ---

for md_file in $md_files; do
	# Extract H1 category title
	title=$(grep -m1 '^# ' "$md_file" | sed 's/^# //')

	# Anchor: lowercase, replace spaces with hyphens
	anchor=$(echo "$title" | tr '[:upper:]' '[:lower:]' | tr ' ' '-' | tr -cd '[:alnum:]-')

	echo "- [$title](#$anchor)"

	# List function entries (H2 with backtick-quoted names)
	grep -E '^## `[^`]+`' "$md_file" 2>/dev/null | sed 's/^## `\([^`]*\)`.*/  - `\1`/' || true
done

echo ""

# --- Body: each doc file with demoted headings ---

for md_file in $md_files; do
	# Extract H1 category title for a separator comment
	title=$(grep -m1 '^# ' "$md_file" | sed 's/^# //')

	echo ""
	echo "---"
	echo ""

	# Process the file: demote all headings by one level
	# H1 (# ) -> H2 (## )
	# H2 (## ) -> H3 (### )
	# H3 (### ) -> H4 (#### )
	# H4 (#### ) -> H5 (##### )
	# Only real ATX headings (1-6 leading # followed by space/EOL)
	# outside fenced code blocks are demoted. Lines such as "#xFF"
	# (radix literal at line start in a code block) or "#t ..." (a
	# value at the start of a paragraph) are content, not headings.
	awk '
    BEGIN { in_fence = 0 }
    /^[[:space:]]*(```|~~~)/ { in_fence = !in_fence; print; next }
    !in_fence {
        n = 0
        tmp = $0
        while (substr(tmp, 1, 1) == "#") {
            n++
            tmp = substr(tmp, 2)
        }
        if (n >= 1 && n <= 6 && (tmp == "" || substr(tmp, 1, 1) ~ /[[:space:]]/)) {
            print "#" $0
            next
        }
    }
    { print }
    ' "$md_file" | sed 's/\](\([a-zA-Z0-9_-]*\.md\))/](doc\/\1)/g'
done
