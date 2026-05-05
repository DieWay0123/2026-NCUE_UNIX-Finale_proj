#!/bin/sh
set -eu

BBFSCHECK_BIN="${BBFSCHECK_BIN:-./bbfscheck}"
TEST_DIR="${TMPDIR:-/tmp}/bbfscheck_test_$$"

cleanup()
{
    rm -rf "$TEST_DIR"
}

trap cleanup EXIT INT TERM

mkdir -p "$TEST_DIR/nested"
printf 'x\n' > "$TEST_DIR/small.txt"
dd if=/dev/zero of="$TEST_DIR/nested/large.bin" bs=1024 count=8 >/dev/null 2>&1

"$BBFSCHECK_BIN" --summary / >/dev/null
"$BBFSCHECK_BIN" --inode / >/dev/null
"$BBFSCHECK_BIN" --scan "$TEST_DIR" --max-depth 2 --small-file 4096 >/dev/null

if "$BBFSCHECK_BIN" --summary "$TEST_DIR/does-not-exist" >/dev/null 2>&1; then
    echo "expected nonexistent path to fail" >&2
    exit 1
fi

echo "bbfscheck tests passed"
