# 02 · glBitmap batches clearing the whole viewport

A performance patch: A and B draw the same picture, and the result is in
the **Frame time** panel.

## The cost

`gl4es_glBitmap()` draws into a CPU-side RGBA buffer the size of the
viewport, and `bitmap_flush()` uploads and blits the part that was drawn.
Any draw call flushes the pending batch, so text drawn between geometry
starts a new batch each time. On master, every new batch clears the whole
buffer first: 1920 × 1080 × 4 = 8.3 MB for a full-HD window, even for one
short label.

The patch clears only the last batch's dirty rectangle, after its blit, and
clears the whole buffer only when it is allocated.

## What the page shows

A grid of "TEXT" labels, each drawn right after a small coloured quad. The
quad is a draw call, so every label is a batch of its own.

The buffer is the size of the viewport, so A's cost grows with the window.
The demo sets gl4es's viewport to a real window size (1920×1080 by default)
while the canvas stays 480×360, showing the viewport's bottom-left corner,
where all the drawing is. At 1920×1080, A clears 8.3 MB per batch, about
500 MB per frame at 60 batches; B clears what the labels cover.

| Value | Range | Effect |
|---|---|---|
| batches | 1–200 | number of labels, so of batches |
| window | 0–3 | viewport size: 480×360, 1280×720, 1920×1080, 2560×1440 |

Measured in headless Chrome on an Apple-silicon Mac, 60 batches, each side alone:

| Viewport | A | B | B vs A |
|---|---|---|---|
| 480×360 | 6.0 ms | 5.8–6.0 ms | no clear difference |
| 1280×720 | 7.9 ms | 5.5 ms | −31% |
| 1920×1080 | 11.8–17.2 ms | 5.4–8.0 ms | −53% to −54% |
| 2560×1440 | 19.3 ms | 5.9 ms | −69% |

At 480×360 the whole buffer fits in the CPU's cache, and clearing it costs a
few microseconds; most of the frame is the `getParameter` stall that demo 03
removes, which A and B both have here.

## Measuring

The page measures on load; press **Measure** again for a fresh number. It
pauses one side while timing the other. Natively,
`./build.sh 02-bitmap-dirty-clear native` ends with the same comparison for
ref, A and B.
