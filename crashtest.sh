#!/usr/bin/env bash
# crashtest.sh <demo> [ref-A] [ref-B]
#
# A/B memory-safety test (Linux + Mesa). Builds demos/<demo> against gl4es A
# and B with AddressSanitizer, runs one frame of each, and checks that
#   A reports a memory error (the bug reproduces), and
#   B runs clean (the fix removes it).
# Exit status 0 = PASS. ASan decides the verdict because a plain build only
# crashes when the corrupted heap happens to be noticed; the plain run is
# printed for information. Refs not given come from demos/<demo>/refs.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
demo=${1:?usage: crashtest.sh <demo> [ref-A] [ref-B]}
default_ref() { sed -n "s/^$1=//p" "$here/demos/$demo/refs" 2>/dev/null; }
ref_a=${2:-$(default_ref REF_A)}
ref_b=${3:-$(default_ref REF_B)}
[ -n "$ref_a" ] && [ -n "$ref_b" ] || { echo "no refs given and none in demos/$demo/refs" >&2; exit 1; }
out=$here/out/native/$demo
logs=$out/crashtest
export LIBGL_NOBANNER=1

run() { # run <side>  ->  exit status; ASan report (if any) in $logs/<side>.*
    local side=$1 status=0
    rm -f "$logs/$side".*
    (cd "$out" && LIBGL_DEEPBIND=0 ASAN_OPTIONS=detect_leaks=0:log_path="$logs/$side" \
        ./"$side" >/dev/null 2>"$logs/$side.stderr") || status=$?
    echo "$status"
}

report() { # report <side>  ->  ASan's summary line, or empty
    cat "$logs/$1".[0-9]* 2>/dev/null | grep -m1 '^SUMMARY: AddressSanitizer' || true
}

mkdir -p "$logs"

echo "== plain build (for information)"
AB_BUILD_ONLY=1 "$here/build.sh" "$demo" "$ref_a" "$ref_b" native >/dev/null
for side in a b; do
    s=$(run $side)
    echo "$side: exit $s$(grep -m1 -E 'free\(\)|corrupt|Segmentation' "$logs/$side.stderr" | sed 's/^/, /' || true)"
done

echo "== ASan build"
SANITIZE=address AB_BUILD_ONLY=1 "$here/build.sh" "$demo" "$ref_a" "$ref_b" native >/dev/null
status_a=$(run a); summary_a=$(report a)
status_b=$(run b); summary_b=$(report b)
echo "A ($ref_a): exit $status_a${summary_a:+, $summary_a}"
echo "B ($ref_b): exit $status_b${summary_b:+, $summary_b}"
if [ -n "$summary_a" ]; then
    echo "   A's first frames:"
    cat "$logs/a".[0-9]* | grep -m4 -E '^    #[0-3] ' | sed 's/^ */     /'
fi

if [ -n "$summary_a" ] && [ -z "$summary_b" ] && [ "$status_b" = 0 ]; then
    echo "PASS: A reproduces the memory error, B runs clean"
else
    echo "FAIL (reports in $logs)"
    exit 1
fi
