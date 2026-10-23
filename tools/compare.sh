#!/bin/sh
# compare.sh GOLDEN OUT: every golden file equals the generated one byte for byte.
set -eu
for f in "$1"/*; do
    name=$(basename "$f")
    if ! cmp -s "$f" "$2/$name"; then
        echo "compare: $name differs from $1/$name" >&2
        cmp "$f" "$2/$name" >&2 || true
        exit 1
    fi
done
echo "compare: $(ls "$1" | wc -l | tr -d ' ') files byte-identical to $1"
