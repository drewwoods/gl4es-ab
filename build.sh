#!/usr/bin/env bash
# build.sh <demo> [ref-A] [ref-B] [web|native|all]      (build.sh -h for help)
#
# Builds gl4es at two refs of $GL4ES_REPO and links demos/<demo>/main.c
# against each. Refs not given come from demos/<demo>/refs (REF_A=, REF_B=).
#   web     (default)  site/<demo>/{index.html,a/,b/}       needs emcc
#   native             out/native/<demo>/{a,b,ref} + PNGs    needs Linux + Mesa
# gl4es builds are cached per commit under out/gl4es/<sha>/<target>, and built
# with -ffile-prefix-map so no local paths end up in the published pages.
# SANITIZE=address (native only) builds gl4es and the demo with ASan.
# AB_BUILD_ONLY=1 (native only) links the binaries without running them.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
GL4ES_REPO=${GL4ES_REPO:-$(cd "$here/.." && pwd)/gl4es}
JOBS=${JOBS:-2}
. "$here/harness/demos.sh"

usage() {
    cat <<USAGE
usage: build.sh <demo> [ref-A] [ref-B] [web|native|all]

Builds gl4es at ref-A and ref-B (default: demos/<demo>/refs) and the demo
against each.

demos (default A -> B):
$(list_demos "$GL4ES_REPO" "$here")

targets:
  web      (default) site/<demo>/ A/B page; needs emcc (source emsdk_env.sh)
  native   out/native/<demo>/{ref,a,b}.png, headless; needs Linux + Mesa
  all      both

environment:
  GL4ES_REPO=<dir>     gl4es checkout (now: $GL4ES_REPO)
  JOBS=<n>             parallel make jobs (now: $JOBS)
  SANITIZE=address     native: build gl4es and the demo with ASan
  AB_BUILD_ONLY=1      native: link, don't run
  AB_FRAME_T=<s>       native: time to render the screenshot at (default 1)
  GL4ES_WEB_URL=<url>  GitHub base for the gl4es commit links (default: origin)
  AB_WEB_URL=<url>     GitHub base for the demo source links (default: origin)

see also:
  crashtest.sh <demo> [ref-A] [ref-B]   ASan A/B verdict (Linux)
  run-x11.sh <demo> [--t <s>]           ref, A, B in X11 windows (Linux)

examples:
  ./build.sh 01-rasterpos                      web page, default refs
  ./build.sh 01-rasterpos master my-branch     web page, other refs
  JOBS=2 ./build.sh 01-rasterpos native        headless screenshots
USAGE
}
case ${1:-} in
''|-h|--help) usage; [ -n "${1:-}" ]; exit ;;
esac
demo=$1
shift
target=web
refs=()
for arg in "$@"; do
    case $arg in
    web|native|all) target=$arg ;;
    *)              refs+=("$arg") ;;
    esac
done
[ ${#refs[@]} -le 2 ] || { usage >&2; exit 1; }

demo_dir=$here/demos/$demo
[ -f "$demo_dir/main.c" ] || { echo "no such demo: $demo" >&2; echo "demos:" >&2; list_demos "$GL4ES_REPO" "$here" >&2; exit 1; }
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
                        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
                        -DCMAKE_C_FLAGS="-ffile-prefix-map=$root/src/=gl4es/"
                    emmake make -j"$JOBS" ;;
            native*) cmake ../src -DNOX11=ON -DNOEGL=ON -DSTATICLIB=ON \
                        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
                        -DCMAKE_C_FLAGS="-ffile-prefix-map=$root/src/=gl4es/${SANITIZE:+ -fsanitize=$SANITIZE -fno-omit-frame-pointer}"
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
github_url() { # github_url <repo dir>  ->  https://github.com/owner/name, if origin is GitHub
    # Empty (not an error) outside a git checkout or without an origin remote.
    { git -C "$1" remote get-url origin 2>/dev/null || true; } \
        | sed -nE 's#^(git@github\.com:|https://github\.com/)([^/]+/[^/]+)$#https://github.com/\2#p' \
        | sed 's/\.git$//'
}
web_url=${GL4ES_WEB_URL:-$(github_url "$GL4ES_REPO")}
# And where this repo's demo source can be browsed: $AB_WEB_URL, else origin,
# on the current branch.
ab_url=${AB_WEB_URL:-$(github_url "$here")}
ab_branch=$(git -C "$here" rev-parse --abbrev-ref HEAD 2>/dev/null || echo main)

# The A/B page's pointer to the demo source.
source_html() {
    local tree=$ab_url/tree/$ab_branch blob=$ab_url/blob/$ab_branch
    if [ -n "$ab_url" ]; then
        echo "Demo source: <a href=\"$tree/demos/$demo\">demos/$demo</a>" \
             "(<a href=\"$blob/demos/$demo/main.c\">main.c</a>," \
             "<a href=\"$blob/demos/$demo/README.md\">README.md</a>," \
             "<a href=\"$blob/demos/$demo/refs\">refs</a>)" \
             "· <a href=\"$tree/harness\">harness</a>"
    else
        echo "Demo source: <code>demos/$demo/main.c</code> in gl4es-ab"
    fi
}

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

# site/index.html: every demo that has a built page, from harness/index.html.
site_index() {
    local items=$here/out/site-index.html d name title
    mkdir -p "$here/out"
    : >"$items"
    for d in "$here"/site/*/; do
        name=$(basename "$d")
        [ -f "$d/index.html" ] || continue
        title=$(sed -n 's/^# *//p;q' "$here/demos/$name/README.md" 2>/dev/null | html_escape)
        echo "    <li><a href=\"$name/\">${title:-$name}</a></li>" >>"$items"
    done
    sed -e "s|@AB_URL@|${ab_url:-https://github.com}|g" "$here/harness/index.html" \
        | awk -v f="$items" '/@DEMOS@/ { while ((getline l < f) > 0) print l; next } { print }' \
        >"$here/site/index.html"
}

build_web() {
    local out=$here/site/$demo
    mkdir -p "$out/a" "$out/b"
    local side ref sha lib
    for side in a b; do
        if [ $side = a ]; then ref=$ref_a sha=$sha_a; else ref=$ref_b sha=$sha_b; fi
        lib=$(build_gl4es "$sha" web)
        # Relative __FILE__ paths: the pages are published.
        emcc -O2 -ffile-prefix-map="$here/"= -I"$here/harness" -I"$here/out/gl4es/$sha/src/include" \
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
        -e "s|@SOURCE@|$(source_html)|g" \
        "$here/harness/ab.html" \
        | awk -v f="$commits" '/@COMMITS@/ { while ((getline l < f) > 0) print l; next } { print }' \
        >"$out/index.html"
    [ -f "$demo_dir/README.md" ] && cp "$demo_dir/README.md" "$out/README.md"
    site_index
    echo "web: $out/index.html"
    # Browsers won't load the .wasm from file://, so the page needs a server.
    echo "Serve it with Python's built-in server (any static server works):"
    echo "    python3 -m http.server --directory $here/site 8765"
    echo "then open http://127.0.0.1:8765/$demo/  (pick another port if 8765 is taken)"
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
