#!/usr/bin/env bash
# run-x11.sh <demo> [--t <seconds>] [--frames <n>]
#
# Opens the native builds of <demo> side by side on $DISPLAY (default :0):
# Mesa reference, A, B. Build them first with build.sh ... native. The windows
# follow the wall clock, so they show the same pose; --t freezes them.
# Close a window, or press Esc or q in it, to quit that one.
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
. "$here/harness/demos.sh"
case ${1:-} in
''|-h|--help)
    echo "usage: run-x11.sh <demo> [--t <seconds>] [--frames <n>]      (Linux, X11)"
    echo
    echo "Opens ref, A and B side by side on \$DISPLAY (default :0). Build them"
    echo "first with ./build.sh <demo> native."
    echo
    echo "demos (default A -> B):"
    list_demos "${GL4ES_REPO:-$here/../gl4es}" "$here"
    [ -n "${1:-}" ]; exit ;;
esac
demo=$1
shift
out=$here/out/native/$demo
export DISPLAY=${DISPLAY:-:0} LIBGL_NOBANNER=1
x=0
for side in ref a b; do
    [ -x "$out/$side" ] || { echo "missing $out/$side: run build.sh $demo <A> <B> native" >&2; exit 1; }
    "$out/$side" --window --x "$x" "$@" &
    x=$((x + 490))
done
wait
