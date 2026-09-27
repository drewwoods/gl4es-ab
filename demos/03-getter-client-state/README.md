# 03 · glGet* calls that wait for the GPU

A performance patch: A and B draw the same picture, and the result is in
the **Frame time** panel. A is the bitmap-dirty-clear patch (demo 02), so
this page shows only what this patch adds.

## The cost

gl4es answers most `glGet*` queries from its own state, but a few went to
the driver: `GL_COLOR_CLEAR_VALUE`, `GL_DEPTH_CLEAR_VALUE`, `GL_LINE_WIDTH`,
`GL_VIEWPORT`, `GL_SCISSOR_BOX` and `GL_GENERATE_MIPMAP_HINT`. On WebGL a
driver `glGet*` is `gl.getParameter()`, which is synchronous: the browser
finishes every queued GPU command before it answers.

`glPushAttrib` reads each of these for its attribute group. gl4es calls it
itself, too: `gl4es_blitTexture()` pushes `GL_COLOR_BUFFER_BIT`, so every
`glBitmap` batch reads the clear color from the driver.

The patch keeps these values in gl4es's state and answers from there.

## What the page shows

The grid of labels from demo 02, over translucent bands that give the GPU
fill work to finish. Each label is wrapped in `glPushAttrib`/`glPopAttrib`,
as HUD code often does.

| Value | Range | Effect |
|---|---|---|
| batches | 1–200 | number of labels, so of batches |
| pushattrib | 0 or 1 | 0 drops the demo's own `glPushAttrib`, leaving only the one in each batch's blit |

## Where to expect a difference

In the browser. Native drivers answer these queries without waiting for
the GPU, so natively A and B should be close; the patch saves the driver
round-trip, not a stall. On WebGL the size of the stall depends on the
browser and GPU.

Measured in headless Chrome on an Apple-silicon Mac, each side alone:

| batches | A | B | B vs A |
|---|---|---|---|
| 60 | 16.3–21.3 ms | 0.71–0.75 ms | −95% to −97% |
| 200 | 62.4 ms | 2.1 ms | −97% |

The page measures on load; press **Measure** again for a fresh number. It
pauses one side while timing the other.
