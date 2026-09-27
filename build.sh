#!/usr/bin/env bash
# build.sh <demo> [ref-A] [ref-B] [web|native|all]
#
# Builds gl4es at two refs of $GL4ES_REPO and links demos/<demo>/main.c
# against each. Refs not given come from demos/<demo>/refs (REF_A=, REF_B=).
#   web     (default)  site/<demo>/{index.html,a/,b/}       needs emcc
#   native             out/native/<demo>/{a,b,ref} + PNGs    needs Linux + Mesa
# gl4es builds are cached per commit under out/gl4es/<sha>/<target>.
# SANITIZE=address (native only) builds gl4es and the demo with ASan.
# AB_BUILD_ONLY=1 (native only) links the binaries without running them.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
usage="usage: build.sh <demo> [ref-A] [ref-B] [web|native|all]"
demo=${1:?$usage}
shift
target=web
refs=()
for arg in "$@"; do
    case $arg in
    web|native|all) target=$arg ;;
    *)              refs+=("$arg") ;;
    esac
done
[ ${#refs[@]} -le 2 ] || { echo "$usage" >&2; exit 1; }
GL4ES_REPO=${GL4ES_REPO:-$here/../gl4es}
JOBS=${JOBS:-2}

demo_dir=$here/demos/$demo
[ -f "$demo_dir/main.c" ] || { echo "no such demo: $demo_dir/main.c" >&2; exit 1; }
default_ref() { sed -n "s/^$1=//p" "$demo_dir/refs" 2>/dev/null; }
ref_a=${refs[0]:-$(default_ref REF_A)}
ref_b=${refs[1]:-$(default_ref REF_B)}
[ -n "$ref_a" ] && [ -n "$ref_b" ] || { echo "no refs given and none in $demo_dir/refs" >&2; exit 1; }

sha_of() { git -C "$GL4ES_REPO" rev-parse --verify "$1^{commit}"; }

# build_gl4es <sha> <web|native>  ->  prints the path of libGL.a
build_gl4es() {
    local sha=$1 kind=$2
    local root=$here/out/gl4es/$sha
    local lib=$root/$kind/lib/libGL.a
    if [ ! -f "$lib" ]; then
        if [ ! -d "$root/src" ]; then
            mkdir -p "$root/src"
            git -C "$GL4ES_REPO" archive "$sha" | tar -x -C "$root/src"
        fi
        mkdir -p "$root/$kind"
        (
            cd "$root/$kind"
            case $kind in
            web)    emcmake cmake ../src -DNOX11=ON -DNOEGL=ON -DSTATICLIB=ON \
                        -DCMAKE_BUILD_TYPE=RelWithDebInfo
                    emmake make -j"$JOBS" ;;
            native*) cmake ../src -DNOX11=ON -DNOEGL=ON -DSTATICLIB=ON \
                        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
                        ${SANITIZE:+-DCMAKE_C_FLAGS="-fsanitize=$SANITIZE -fno-omit-frame-pointer"}
                    make -j"$JOBS" ;;
            esac
        ) >"$root/$kind.log" 2>&1 || { echo "gl4es $sha ($kind) failed, see $root/$kind.log" >&2; exit 1; }
        # the build puts libGL.a in <src>/lib
        mkdir -p "$(dirname "$lib")"
        mv "$root/src/lib/libGL.a" "$lib"
    fi
    echo "$lib"
}

sha_a=$(sha_of "$ref_a")
sha_b=$(sha_of "$ref_b")
# A full or abbreviated SHA is shown short; a branch or tag name as given.
label() { if [[ $1 =~ ^[0-9a-f]{7,40}$ ]]; then echo "${1:0:10}"; else echo "$1"; fi; }
label_a=$(label "$ref_a")
label_b=$(label "$ref_b")
title=$(sed -n 's/^# *//p;q' "$demo_dir/README.md" 2>/dev/null || echo "$demo")

# Where the commits can be browsed: $GL4ES_WEB_URL, else the checkout's
# origin remote if it's on GitHub.
web_url=${GL4ES_WEB_URL:-$(git -C "$GL4ES_REPO" remote get-url origin 2>/dev/null \
    | sed -nE 's#^(git@github\.com:|https://github\.com/)([^/]+/[^/]+)$#https://github.com/\2#p' \
    | sed 's/\.git$//')}

html_escape() { sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' -e 's/"/\&quot;/g'; }

# The A/B page's commit section: A itself, then every commit in A..B.
commits_html() {
    local subj
    commit_link() { # commit_link <sha>  ->  <code>sha</code>, linked when web_url is known
        if [ -n "$web_url" ]; then echo "<a href=\"$web_url/commit/$1\"><code>${1:0:10}</code></a>"
        else echo "<code>${1:0:10}</code>"; fi
    }
    subj=$(git -C "$GL4ES_REPO" log -1 --format=%s "$sha_a" | html_escape)
    echo "<p>A is $(commit_link "$sha_a") $subj</p>"
    echo "<p>B adds these commits:</p>"
    echo "<ol>"
    git -C "$GL4ES_REPO" log --reverse --format='%H %s' "$sha_a..$sha_b" | while read -r sha s; do
        echo "<li>$(commit_link "$sha") $(echo "$s" | html_escape)</li>"
    done
    echo "</ol>"
    if [ -n "$web_url" ]; then
        echo "<p><a href=\"$web_url/compare/${sha_a:0:12}...${sha_b:0:12}\">Full diff A...B on GitHub</a></p>"
    fi
}

build_web() {
    local out=$here/site/$demo
    mkdir -p "$out/a" "$out/b"
    local side ref sha lib
    for side in a b; do
        if [ $side = a ]; then ref=$ref_a sha=$sha_a; else ref=$ref_b sha=$sha_b; fi
        lib=$(build_gl4es "$sha" web)
        emcc -O2 -I"$here/harness" -I"$here/out/gl4es/$sha/src/include" \
            -include "$here/out/gl4es/$sha/src/include/GL/gl.h" \
            -DAB_SIDE="\"$side\"" -DAB_REF="\"$(label "$ref")\"" -DAB_SHA="\"${sha:0:10}\"" \
            "$here/harness/web.c" "$here/harness/common.c" "$demo_dir/main.c" \
            "$lib" -sUSE_WEBGL2=1 -sFULL_ES2=1 \
            -sALLOW_MEMORY_GROWTH=1 --shell-file "$here/harness/shell.html" \
            -o "$out/$side/index.html"
    done
    local commits=$here/out/commits-$demo.html
    mkdir -p "$here/out"
    commits_html >"$commits"
    sed -e "s|@TITLE@|$title|g" -e "s|@DEMO@|$demo|g" \
        -e "s|@REF_A@|$label_a|g" -e "s|@SHA_A@|${sha_a:0:10}|g" \
        -e "s|@REF_B@|$label_b|g" -e "s|@SHA_B@|${sha_b:0:10}|g" \
        "$here/harness/ab.html" \
        | awk -v f="$commits" '/@COMMITS@/ { while ((getline l < f) > 0) print l; next } { print }' \
        >"$out/index.html"
    [ -f "$demo_dir/README.md" ] && cp "$demo_dir/README.md" "$out/README.md"
    echo "web: $out/index.html"
}

build_native() {
    local out=$here/out/native/$demo
    mkdir -p "$out"
    local cc=${CC:-gcc} side ref sha lib kind=native
    local flags=(-O2 -Wall -I"$here/harness" "$here/harness/native.c" "$here/harness/common.c" "$demo_dir/main.c")
    if [ -n "${SANITIZE:-}" ]; then
        kind=native-$SANITIZE
        flags+=(-fsanitize="$SANITIZE" -fno-omit-frame-pointer)
    fi
    for side in a b; do
        if [ $side = a ]; then ref=$ref_a sha=$sha_a; else ref=$ref_b sha=$sha_b; fi
        lib=$(build_gl4es "$sha" "$kind")
        $cc -I"$here/out/gl4es/$sha/src/include" "${flags[@]}" \
            -DAB_SIDE="\"$side\"" -DAB_REF="\"$(label "$ref")\"" -DAB_SHA="\"${sha:0:10}\"" \
            "$lib" -lEGL -lX11 -ldl -lm -o "$out/$side"
    done
    # Mesa desktop GL (compatibility profile), no gl4es: the "correct" reference
    $cc "${flags[@]}" -DAB_SIDE="\"ref\"" -DAB_REF="\"mesa\"" -DAB_SHA="\"-\"" \
        -DAB_DESKTOP_GL -lEGL -lOpenGL -lX11 -lm -o "$out/ref"
    [ -n "${AB_BUILD_ONLY:-}" ] && { echo "native: $out/{ref,a,b}"; return; }
    # gl4es dlopens libGLESv2 with RTLD_DEEPBIND by default; ASan refuses it.
    [ -n "${SANITIZE:-}" ] && export LIBGL_DEEPBIND=0
    local frame=${AB_FRAME_T:-}
    for side in ref a b; do
        rm -f "$out/$side.ppm" "$out/$side.png"
        # A side that crashes is a result, not a build failure.
        (cd "$out" && ./$side --screenshot "$side.ppm" ${frame:+--t "$frame"}) \
            || { echo "$side: exited with status $?" >&2; continue; }
        command -v pnmtopng >/dev/null && pnmtopng "$out/$side.ppm" >"$out/$side.png" && rm "$out/$side.ppm"
    done
    echo "native: $out/{ref,a,b}.png"
}

# Fail before doing any work, saying what's missing and what else to try.
check_web() {
    command -v emcc >/dev/null && command -v emcmake >/dev/null && command -v cmake >/dev/null && return
    {
        echo "build.sh: the web target needs Emscripten (emcc, emcmake) and cmake."
        if ! command -v cmake >/dev/null; then
            echo "  cmake is not installed."
        fi
        if ! command -v emcc >/dev/null; then
            local env
            for env in "${EMSDK:-}/emsdk_env.sh" "$HOME/emsdk/emsdk_env.sh" \
                       "$HOME/src/emsdk/emsdk_env.sh"; do
                [ -f "$env" ] && break
                env=
            done
            if [ -n "$env" ]; then
                echo "  emsdk is installed but not in this shell's environment. Run:"
                echo "      source $env"
            else
                echo "  Install emsdk: https://emscripten.org/docs/getting_started/downloads.html"
                echo "  then source its emsdk_env.sh."
            fi
        fi
        echo "Or build natively (Linux + Mesa): ./build.sh $demo native"
    } >&2
    exit 1
}
check_native() {
    local missing=
    if [ "$(uname -s)" != Linux ]; then
        echo "build.sh: the native target needs Linux + Mesa (EGL surfaceless); this is $(uname -s)." >&2
        echo "Build for the web instead: ./build.sh $demo web" >&2
        exit 1
    fi
    command -v cmake >/dev/null || missing="$missing cmake"
    if ! command -v "${CC:-gcc}" >/dev/null; then
        missing="$missing ${CC:-gcc}"
    elif ! printf '#include <EGL/egl.h>\n#include <X11/Xlib.h>\n' \
            | "${CC:-gcc}" -E -x c - >/dev/null 2>&1; then
        missing="$missing EGL/X11-headers"
    fi
    if [ -n "$missing" ]; then
        echo "build.sh: the native target is missing:$missing" >&2
        echo "  (Debian/Ubuntu: apt install build-essential cmake libegl-dev libx11-dev libgl-dev)" >&2
        echo "Or build for the web: ./build.sh $demo web" >&2
        exit 1
    fi
}

case $target in
web)    check_web; build_web ;;
native) check_native; build_native ;;
all)    check_web; check_native; build_web; build_native ;;
*)      echo "unknown target: $target" >&2; exit 1 ;;
esac
