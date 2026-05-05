#!/bin/sh
set -eu

target="${1:-testdir}"
count="${2:-1000}"

mkdir -p "$target"
i=1
while [ "$i" -le "$count" ]; do
    printf 'x\n' > "$target/file_$i"
    i=$((i + 1))
done

echo "generated $count small files in $target"
