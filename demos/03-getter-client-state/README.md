# 03 · glGet* calls that wait for the GPU

A performance patch pair (PR #519): A and B draw the same picture, and the
result is in the **Frame time** panel. A is upstream master, which already
has the bitmap dirty-clear patch (demo 02), so this page shows only what the
two #519 commits add.

## The cost

gl4es answers most `glGet*` queries from its own state, but a few went to
the driver: `GL_COLOR_CLEAR_VALUE`, `GL_DEPTH_CLEAR_VALUE`, `GL_LINE_WIDTH`,
`GL_VIEWPORT`, `GL_SCISSOR_BOX` and `GL_GENERATE_MIPMAP_HINT`. On WebGL a
driver `glGet*` is `gl.getParameter()`, which is synchronous: the browser
finishes every queued GPU command before it answers.

`glPushAttrib` reads each of these for its attribute group. gl4es calls it
itself, too: `gl4es_blitTexture()` pushes `GL_COLOR_BUFFER_BIT`, so every
`glBitmap` batch reads the clear color from the driver.

`glGetBooleanv` had no gl4es version at all, so every call went to the
driver: a stall on WebGL, and for state only gl4es knows (`GL_LIGHTING`,
`GL_SHADE_MODEL`, `GL_MATRIX_MODE` on GLES2) the driver rejects the pname
and leaves the output unwritten.

The first commit keeps the missing values in gl4es's state; the second
gives `glGetBooleanv` the same local lookup as the other getters.

## What the page shows

The grid of labels from demo 02, over translucent bands that give the GPU
fill work to finish. Each label is wrapped in `glPushAttrib`/`glPopAttrib`,
as HUD code often does, and preceded by two `glGetBooleanv` checks
(`GL_DEPTH_WRITEMASK`, `GL_LIGHTING`). The status line also reports whether
`glGetBooleanv(GL_LIGHTING)` returns the value just set: "left unwritten"
on A, "correct" on B.

| Value | Range | Effect |
|---|---|---|
| batches | 1–200 | number of labels, so of batches |
| pushattrib | 0 or 1 | 0 drops the demo's own `glPushAttrib`, leaving only the one in each batch's blit |
| getbooleanv | 0 or 1 | 0 drops the two `glGetBooleanv` checks per label |

Enable flags other than `GL_LIGHTING` (`GL_BLEND`, `GL_DEPTH_TEST`, ...)
still go to the driver in every `glGet*`, on B as well, so the demo doesn't
query them; `glIsEnabled` answers them locally.

## Where to expect a difference

In the browser. Native drivers answer these queries without waiting for
the GPU, so natively A and B should be close; the patch saves the driver
round-trip, not a stall. On WebGL the size of the stall depends on the
browser and GPU.

Measured in headless Chrome on an Apple-silicon Mac, 60 batches, each side
alone:

| pushattrib | getbooleanv | A | B | B vs A |
|---|---|---|---|---|
| 1 | 1 | 18.6 ms | 0.62 ms | −97% |
| 0 | 1 | 7.5 ms | 0.56 ms | −92% |
| 1 | 0 | 16.2 ms | 0.63 ms | −96% |
| 0 | 0 | 5.1 ms | 0.52 ms | −90% |

With both off, what's left on A is the clear-color read in each batch's
blit. A single `glGetBooleanv(GL_DEPTH_WRITEMASK)` took about 40 µs on A
and under 0.1 µs on B.

The page measures on load; press **Measure** again for a fresh number. It
pauses one side while timing the other.
