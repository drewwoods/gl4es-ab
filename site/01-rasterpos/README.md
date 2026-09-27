# 01 · glRasterPos3f under a perspective projection

## The bug

`gl4es_glRasterPos3f()` transforms the point to clip space and then maps
x and y straight to the viewport, without dividing by w. That is right only
when w = 1 (glOrtho, identity projection). Under `glFrustum` or
`gluPerspective`, a bitmap drawn there lands off by a factor of w, usually
somewhere off-screen.

When the mapped position falls outside the window, master leaves the old raster
position unchanged. The next `glBitmap` then draws at whatever position was set
last. The spec (GL 2.1 §2.13) says a position outside the clip volume makes
the raster position *invalid*, and `glBitmap` with an invalid raster position
draws nothing (§3.7).

## What the page shows

Each marked vertex is drawn twice: once as a `GL_POINTS` dot, which the GLES
vertex pipeline transforms, and once as a `glBitmap` arrow placed with
`glRasterPos3f`, which gl4es transforms on the CPU. Correct output puts every
arrow tip on the upper-right corner of its dot.

| Marker | Where | Expected |
|---|---|---|
| white | the 8 corners of a spinning cube, drawn first | an arrow at every green dot |
| yellow | an anchor set under `glOrtho` (w = 1) | an arrow at the dot, in A as well: master handles w = 1 |
| red | a probe orbiting through, beside and behind the viewer, drawn right after the anchor | an arrow at the dot while the probe is in view; **no red arrow at all** otherwise |

The status line under each side says where the probe is, from the demo's own
arithmetic, so you know whether a red arrow should be visible.

**A (master):** the white arrows are scattered away from the cube or missing.
While the probe is outside the view (for example, freeze at t = 6), the
anchor's arrow is red instead of yellow: the probe's arrow was drawn at the
stale raster position the anchor left behind.
**B:** every arrow points at its dot, and there is no red arrow while the
probe is out of view. B matches the native Mesa desktop-GL reference image.

## Not covered (known gaps in the patch)

The patch adds a valid flag and checks it in `glBitmap` only. These gaps are
marked with TODOs in the patch, and this demo does not claim they're fixed:

- `glDrawPixels` still draws at the last valid position when the raster
  position is invalid (`render_raster_list()`).
- `glWindowPos3f` with negative coordinates marks the position invalid. The
  spec never invalidates a window position.
- `GL_CURRENT_RASTER_POSITION_VALID` can't be queried.

Found while building this demo, on master and unchanged by the patch:
`gl4es_glBitmap()` clips a bitmap that crosses the window's left edge by
advancing `sx`, but starts each row's write pointer at `rx`, not `rx + sx`.
The row lands `sx` pixels too far left, and on the bottom row it writes before
the start of the CPU bitmap buffer (a native segfault in an earlier version of
this demo). The arrow marker extends only up and right of its raster position,
so this demo never crosses the left edge.

## Running it

```
./build.sh 01-rasterpos master <patch-ref> web      # site/01-rasterpos/
./build.sh 01-rasterpos master <patch-ref> native   # Linux + Mesa; out/native/01-rasterpos/
```

The native build renders one frame headless (EGL surfaceless + pbuffer) at
`AB_FRAME_T` (default t = 1) and writes `ref.png` (Mesa desktop GL, no gl4es),
`a.png` and `b.png`.
