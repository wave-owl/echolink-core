#!/bin/sh
# check_gsm.sh LOCK DIR [DIR2]: every file listed in LOCK exists in DIR (or DIR2) with the
# locked sha256. Used for src/gsm/ (the vendored copy) and for a reference libgsm 1.0.24
# checkout (src/ and inc/), so the vendored files are proven equal to upstream.
set -eu
[ -f "$1" ] || { echo "check_gsm: no lock file $1" >&2; exit 1; }
lock=$1; dir=$2; dir2=${3:-$2}
if command -v sha256sum >/dev/null 2>&1; then sum="sha256sum"; else sum="shasum -a 256"; fi
fail=0
grep -v '^#' "$lock" | while read -r want file; do
    [ -n "$file" ] || continue
    if [ -f "$dir/$file" ]; then path="$dir/$file"
    elif [ -f "$dir2/$file" ]; then path="$dir2/$file"
    elif [ -f "$dir/../$file" ]; then path="$dir/../$file"
    else echo "check_gsm: missing $file" >&2; exit 1; fi
    got=$($sum "$path" | cut -d' ' -f1)
    if [ "$got" != "$want" ]; then
        echo "check_gsm: $path differs from libgsm 1.0.24 ($got != $want)" >&2; exit 1
    fi
done || fail=1
[ "$fail" = 0 ] || exit 1
echo "check_gsm: $(grep -vc '^#' "$lock") files equal to libgsm 1.0.24"
