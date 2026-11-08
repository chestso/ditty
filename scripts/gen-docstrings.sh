#!/bin/bash
# gen-docstrings.sh - Generate src/docstrings.gen.h from docstrings/*.md files
#
# Each .md file uses this structure:
#   # Category Title        (H1 - skipped)
#   Category description    (skipped)
#   ## `function-name`      (H2 - function delimiter; exactly this, no trailing text)
#   Docstring body...       (captured, H3 converted to H2)
#   ## `next-function`      (next function)
#   ## Plain heading        (H2 - ends the current section, body skipped)
# Duplicate function headings anywhere in the tree are an error.
#
# Usage: scripts/gen-docstrings.sh doc > src/docstrings.gen.h

set -e

DOC_DIR="${1:?Usage: $0 <doc-directory>}"

# A duplicate heading is ambiguous: lookup_builtin_doc() returns the first
# match, silently shadowing every later entry. Reject duplicates up front,
# whether they occur within a single file or across files.
duplicate_names="$(
	for md_file in $(find "$DOC_DIR" -name '*.md' -type f | sort); do
		grep -E '^## `[^`]+`[[:space:]]*$' "$md_file" || true
	done | sed -E 's/^## `([^`]+)`.*/\1/' | sort | uniq -d
)"
if [ -n "$duplicate_names" ]; then
	echo "gen-docstrings.sh: duplicate docstring headings:" >&2
	printf '%s\n' "$duplicate_names" >&2
	exit 1
fi

cat <<'HEADER'
// AUTO-GENERATED from docstrings/*.md — do not edit

#ifndef DOCSTRINGS_GEN_H
#define DOCSTRINGS_GEN_H

#include <string.h>

struct builtin_doc {
    const char *name;
    const char *docstring;
};

static const struct builtin_doc builtin_docs[] = {
HEADER

# Process all markdown files in sorted order for deterministic output
for md_file in $(find "$DOC_DIR" -name '*.md' -type f | sort); do
	awk '
    BEGIN {
        name = ""
        nlines = 0
    }

    # Skip H1 headings (category title)
    /^# / { next }

    # H2 heading. A backtick-quoted name on its own starts a function
    # section; any other H2 ends the current section and its body is
    # skipped until the next function heading. (Without this, a trailing
    # section such as "## Standard Library Aliases" would leak into the
    # preceding function docstring, and "## `name` extra text" would be
    # misread as a function named "name".)
    /^## / {
        if ($0 ~ /^## `[^`]+`[[:space:]]*$/) {
            # Emit previous function if we have one
            if (name != "") {
                emit(name)
            }

            # Extract function name: ## `name` -> name
            n = $0
            sub(/^## `/, "", n)
            sub(/`.*$/, "", n)
            name = n
            nlines = 0
        } else {
            if (name != "") {
                emit(name)
            }
            name = ""
            nlines = 0
        }
        next
    }

    # Accumulate body lines when inside a function section
    name != "" {
        nlines++
        body[nlines] = $0
    }

    END {
        if (name != "") {
            emit(name)
        }
    }

    function emit(n,    i, start, end, line) {
        # Find first non-blank line
        start = 1
        while (start <= nlines && body[start] == "") start++

        # Find last non-blank line
        end = nlines
        while (end >= start && body[end] == "") end--

        if (start > end) return

        # Emit C struct entry
        printf "    {\"%s\",\n", c_escape(n)

        for (i = start; i <= end; i++) {
            line = body[i]

            # Convert H3 to H2 (### -> ##) for runtime compatibility
            if (line ~ /^### /) {
                sub(/^### /, "## ", line)
            }

            line = c_escape(line)

            if (i < end) {
                printf "     \"%s\\n\"\n", line
            } else {
                printf "     \"%s\"},\n", line
            }
        }

        # Reset for next function
        nlines = 0
        delete body
    }

    function c_escape(s) {
        gsub(/\\/, "\\\\", s)
        gsub(/"/, "\\\"", s)
        return s
    }
    ' "$md_file"
done

cat <<'FOOTER'
    {NULL, NULL}
};

static const char *lookup_builtin_doc(const char *name) {
    for (int i = 0; builtin_docs[i].name != NULL; i++) {
        if (strcmp(builtin_docs[i].name, name) == 0)
            return builtin_docs[i].docstring;
    }
    return NULL;
}

#endif
FOOTER
