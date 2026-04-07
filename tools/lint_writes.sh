#!/bin/sh
# All filesystem reads and writes in this codebase must go through a single
# wrapper that calls safe_path_check() first. This script fails CI if any
# other source file reaches for a filesystem-mutating function directly.
#
# Exempt: src/sd_io.c (the one allowed caller).
set -eu

cd "$(dirname "$0")/.."

# Files we scan — C source and headers in our own tree only.
# Unity and other vendored code under tests/unity/ are excluded.
SOURCES=$(find src lib -type f \( -name '*.c' -o -name '*.h' \) 2>/dev/null | grep -v '^src/sd_io\.c$' || true)

if [ -z "$SOURCES" ]; then
    echo "lint_writes: no sources to scan"
    exit 0
fi

# Patterns that must not appear outside src/sd_io.c.
# Each pattern is an ERE matched by grep -E on each line.
PATTERNS="
fopen[[:space:]]*\(
fwrite[[:space:]]*\(
fputc[[:space:]]*\(
fputs[[:space:]]*\(
fprintf[[:space:]]*\(
fputws[[:space:]]*\(
[^a-zA-Z_]write[[:space:]]*\(
[^a-zA-Z_]pwrite[[:space:]]*\(
[^a-zA-Z_]writev[[:space:]]*\(
[^a-zA-Z_]creat[[:space:]]*\(
[^a-zA-Z_]open[[:space:]]*\(
[^a-zA-Z_]mkdir[[:space:]]*\(
[^a-zA-Z_]rmdir[[:space:]]*\(
[^a-zA-Z_]unlink[[:space:]]*\(
[^a-zA-Z_]rename[[:space:]]*\(
[^a-zA-Z_]remove[[:space:]]*\(
fsFileWrite[[:space:]]*\(
fsFileFlush[[:space:]]*\(
fsFileSetSize[[:space:]]*\(
fsFsCreateFile[[:space:]]*\(
fsFsWriteFile[[:space:]]*\(
fsFsDeleteFile[[:space:]]*\(
fsFsCreateDirectory[[:space:]]*\(
fsFsDeleteDirectory[[:space:]]*\(
fsFsCleanDirectoryRecursively[[:space:]]*\(
fsFsRenameFile[[:space:]]*\(
fsFsRenameDirectory[[:space:]]*\(
"

fail=0
for pattern in $PATTERNS; do
    # shellcheck disable=SC2086
    hits=$(grep -nE "$pattern" $SOURCES 2>/dev/null || true)
    if [ -n "$hits" ]; then
        echo "lint_writes: banned filesystem API '$pattern' used outside src/sd_io.c:"
        echo "$hits" | sed 's/^/  /'
        fail=1
    fi
done

if [ "$fail" -ne 0 ]; then
    echo ""
    echo "All filesystem reads and writes must go through src/sd_io.c,"
    echo "which calls safe_path_check() to verify the destination."
    exit 1
fi

echo "lint_writes: OK"
