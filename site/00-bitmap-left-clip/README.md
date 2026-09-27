# 00 · glBitmap clipping at the window's left edge

## The bug

`gl4es_glBitmap()` renders bitmaps into a CPU buffer the size of the
viewport. When a bitmap starts left of the window (`rx < 0`), the clip skips
its first `sx = -rx` columns, but each row's write pointer still starts at
`rx`. Every row lands `sx` pixels too far left, and the last pixels of each
row wrap onto the end of the row below. On the bottom row of the buffer, the
writes land before the start of the allocation: a heap underflow. Natively
that can segfault; in wasm it silently corrupts the heap.

A bitmap *entirely* left of the window (or below it) takes a second wrong
path. The clip code swaps the start and end columns when they cross, then
draws columns past the bitmap's width: reading beyond each bitmap row and
writing pixels that shouldn't be drawn.

## What the page shows

The same 32×32 "F" is drawn four times. A grey outline, drawn with GL lines,
marks the full box each bitmap would cover:

| Colour | Case | Expected |
|---|---|---|
| green | fully inside (control) | the whole F |
| cyan | crosses x = 0 | the right 20 columns of the F, in place |
| yellow | crosses x = 0 and y = 0 | the top-right 20×20 of the F, in place |
| red | entirely left of x = 0 | nothing |

**A (master):** the cyan and yellow Fs are shifted 12 px left inside their
outlines, with pixels wrapped to the right edge. Red pixels may appear. The
native build can crash on the yellow case.
**B:** every F is clipped in place, there's no red, and B matches the Mesa
desktop-GL reference.

Raster positions are set under `glOrtho` (w = 1) and never negative, so only
glBitmap's clipping is being tested, not `glRasterPos`.

## Running it

A and B default to the SHAs in `refs`; pass `[ref-A] [ref-B]` to override.

```
./build.sh 00-bitmap-left-clip web
./build.sh 00-bitmap-left-clip native
./crashtest.sh 00-bitmap-left-clip          # Linux: ASan A/B verdict
```

`crashtest.sh` builds both sides with AddressSanitizer and passes when A
reports a memory error and B runs clean. On Ubuntu 24.04 (gcc 14, Mesa 25.2.8, Intel Alder Lake-N),
master reports a heap-buffer-overflow in `gl4es_glBitmap`: a write 48 bytes
(12 pixels, the yellow case's `sx`) before the CPU bitmap buffer. A plain build
of master aborts with `free(): invalid pointer`.
