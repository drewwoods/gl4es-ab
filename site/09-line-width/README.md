# 09 · Wide lines on WebGL

## The bug

WebGL and ANGLE report `GL_ALIASED_LINE_WIDTH_RANGE` as [1, 1], so
`glLineWidth` changed nothing in the browser: every line was one pixel
wide.

| Commit | |
|---|---|
| Draw lines wider than the driver allows as quads | each wide line segment becomes a screen-space quad, drawn with a temporary identity transform; `glGet` of the range reports [1, 64] (gl-repl's patch) |
| Widen emulated wide lines along the minor axis, as GL does | the quads were widened perpendicular to the line; GL widens a non-antialiased wide line along its minor axis (GL 2.1 section 3.4.2), without extending its ends |
| Emulate wide lines only where the driver draws none | the quads were also used above the largest width of drivers that do draw wide lines (Mesa iris: 7.375), where GL clamps the width instead |

## What the page shows

Each row's OpenGL is listed on the page under the frames:

- **LINES**: widths 1, 3, 6 and 10, each as a horizontal and a 45-degree
  segment.
- **LINE STRIP**: a zigzag at width 5.
- **LINE LOOP**: a square at width 4.
- **POLYGON MODE**: two quads as polygon-mode lines at width 3.
- **RANGE**: `glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE)`, in the status
  line: [1, 1] on A, [1, 64] on B.

Odd widths are centered on pixel centers and even widths on pixel edges, so
the edges of the horizontal lines miss pixel centers, where implementations
may round either way. The edges of the even-width 45-degree lines, and the
ends of every line, still run through pixel centers.

## Against native desktop GL

In Chrome, B matches Apple's OpenGL exactly (0 px) and differs from Mesa
radeonsi by 342 px, NVIDIA by 462 and Mesa iris by 524, exactly where
Apple's GL differs from them:

- 332 px in each is the POLYGON MODE label, one pixel lower on Apple and
  gl4es. In float, its raster position comes out at 79.999992, which both
  floor to 79; Mesa adds 0.0001 first (to match SGI's OpenGL, its source
  says) and gets 80, and NVIDIA agrees with Mesa.
- 10 to 12 px are the polygon-mode boxes' corners, where the edges' ends
  fall on pixel centers.
- NVIDIA draws the 45-degree lines of widths 6 and 10 one pixel further
  right, deciding the pixel centers on their edges the other way (120 px).
- Mesa iris's desktop GL draws the 10-pixel lines 7.375 wide, its largest
  line width (180 px).

A differs from each by about 3,400 to 3,700 px. With only the first commit, B was 622 px from
Apple: a 45-degree line drawn perpendicular-wide is about 1.4 times thicker
than GL's, with its ends cut across the line.

Natively the drivers draw wide lines themselves, so A and B are identical
there. Both differ from the same machine's desktop GL by 332 px: the POLYGON
MODE label again, which predates these commits.

## The frame time

No performance change to show: B draws more triangles than A's one-pixel
lines, but the page is small. A's apparent slowdown in Chrome comes from
its `glGetFloatv` of the line-width range going to WebGL.
