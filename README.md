# gl4es-ab

A/B demos for gl4es patches proposed upstream (ptitSeb/gl4es#515). Each demo
is a small GL 1.x program built twice: against gl4es **A** (`master`, or the
parent patch for a dependent patch) and against gl4es **B** (the patch). The
only difference between the two is the patch under review.

```
harness/           shared host code
  ab.h               what a demo implements (demo_init, demo_draw) and may call
  common.c           frame timing and status line
  web.c              WebGL2 context + gl4es, as gl-repl ships it
  native.c           headless Mesa (EGL surfaceless + pbuffer): GLES2 under
                     gl4es, or desktop GL with -DAB_DESKTOP_GL as a reference
  shell.html         one side's page
  ab.html            the side-by-side A/B page
demos/NN-<patch>/  main.c + README.md (the bug, what to look for, expected result)
build.sh           build.sh <demo> <ref-A> <ref-B> [web|native|all]
site/NN-<patch>/   the built A/B page (index.html, a/, b/)
```

## Building

gl4es is built from `$GL4ES_REPO` (default `../gl4es`) at each ref with
`git archive`, so the checkout's working tree is never touched. Builds are
cached per commit under `out/gl4es/<sha>/`.

Web (needs `emcc`; macOS is fine):

```
source ~/src/emsdk/emsdk_env.sh
./build.sh 01-rasterpos master <patch-ref> web
python3 -m http.server -d site 8765    # then open /01-rasterpos/
```

Native (Linux + Mesa with EGL surfaceless; gl4es can't be built on macOS):

```
JOBS=2 ./build.sh 01-rasterpos master <patch-ref> native
```

For memory bugs, `./crashtest.sh <demo> <ref-A> <ref-B>` builds both sides
with AddressSanitizer and passes when A reports a memory error and B runs
clean.

The native build writes `out/native/<demo>/{ref,a,b}.png`, rendered at `AB_FRAME_T`
(default t = 1). `ref` is Mesa desktop GL with no gl4es: what the spec
expects.

## How the hosts drive gl4es

gl4es is built `-DNOX11=ON -DNOEGL=ON -DSTATICLIB=ON` for both hosts. The host
owns the context and calls `initialize_gl4es()`. At the end of each frame it
calls `gl4es_pre_swap()` and `gl4es_post_swap()`, the swap hooks gl4es exports
for NOX11/NOEGL builds. Without them, work gl4es still queues on the CPU
(glBitmap batches, for example) is drawn a frame late, because `glClear` does
not flush it.

Each side of the web page follows the wall clock, wrapped to 60 s, so A and B
show the same pose. "Freeze" sends a fixed t to both iframes. A and B are
separate pages because each links its own gl4es.
