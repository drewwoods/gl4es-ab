#!/usr/bin/env bash
# build.sh <demo> <ref-A> <ref-B> [web|native|all]
#
# Builds gl4es at two refs of $GL4ES_REPO and links demos/<demo>/main.c
# against each.
#   web     (default)  site/<demo>/{index.html,a/,b/}       needs emcc
#   native             out/native/<demo>/{a,b,ref} + PNGs    needs Linux + Mesa
# gl4es builds are cached per commit under out/gl4es/<sha>/<target>.
# SANITIZE=address (native only) builds gl4es and the demo with ASan.
# AB_BUILD_ONLY=1 (native only) links the binaries without running them.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
demo=${1:?usage: build.sh <demo> <ref-A> <ref-B> [web|native|all]}
ref_a=${2:?ref-A}
ref_b=${3:?ref-B}
target=${4:-web}
GL4ES_REPO=${GL4ES_REPO:-$here/../gl4es}
JOBS=${JOBS:-2}

demo_dir=$here/demos/$demo
[ -f "$demo_dir/main.c" ] || { echo "no such demo: $demo_dir/main.c" >&2; exit 1; }

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
title=$(sed -n 's/^# *//p;q' "$demo_dir/README.md" 2>/dev/null || echo "$demo")

build_web() {
    local out=$here/site/$demo
    mkdir -p "$out/a" "$out/b"
    local side ref sha lib
    for side in a b; do
        if [ $side = a ]; then ref=$ref_a sha=$sha_a; else ref=$ref_b sha=$sha_b; fi
        lib=$(build_gl4es "$sha" web)
        emcc -O2 -I"$here/harness" -I"$here/out/gl4es/$sha/src/include" \
            -include "$here/out/gl4es/$sha/src/include/GL/gl.h" \
            -DAB_SIDE="\"$side\"" -DAB_REF="\"$ref\"" -DAB_SHA="\"${sha:0:10}\"" \
            "$here/harness/web.c" "$here/harness/common.c" "$demo_dir/main.c" \
            "$lib" -sUSE_WEBGL2=1 -sFULL_ES2=1 \
            -sALLOW_MEMORY_GROWTH=1 --shell-file "$here/harness/shell.html" \
            -o "$out/$side/index.html"
    done
    sed -e "s|@TITLE@|$title|g" -e "s|@DEMO@|$demo|g" \
        -e "s|@REF_A@|$ref_a|g" -e "s|@SHA_A@|${sha_a:0:10}|g" \
        -e "s|@REF_B@|$ref_b|g" -e "s|@SHA_B@|${sha_b:0:10}|g" \
        "$here/harness/ab.html" >"$out/index.html"
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
            -DAB_SIDE="\"$side\"" -DAB_REF="\"$ref\"" -DAB_SHA="\"${sha:0:10}\"" \
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

case $target in
web)    build_web ;;
native) build_native ;;
all)    build_web; build_native ;;
*)      echo "unknown target: $target" >&2; exit 1 ;;
esac
