# 08 · Polygon offset for polygon-mode lines

## The bug

`glPolygonOffset` with `GL_POLYGON_OFFSET_LINE` moves the edges drawn by
`glPolygonMode(GL_LINE)` toward the eye, so outlines drawn over their own
faces aren't hidden by them. GLES only has `GL_POLYGON_OFFSET_FILL`, which
doesn't apply to lines, so on gl4es the LINE enable raised
`GL_INVALID_ENUM`, the outlines z-fought their faces, and
`glPushAttrib(GL_POLYGON_BIT)` didn't save the offset.

One commit: gl4es tracks the enable and the offset itself, restores them
on `glPopAttrib`, and applies the offset's units to polygon-mode line draws
by shifting the projection's depth row. It is gl-repl's patch with two
changes: the offset values live in `glstate` rather than two globals, and a
startup error-drain loop is bounded.

## What the page shows

Each row's OpenGL is listed on the page under the frames. Every row draws a
grid of quads filled, then the same quads as polygon-mode lines over them,
the grid sloping in depth:

- **NO OFFSET**: no offset; the outlines z-fight their faces and most are
  hidden, on every implementation.
- **OFFSET LINE**: `glEnable(GL_POLYGON_OFFSET_LINE)` and
  `glPolygonOffset(0, -32)`: the whole grid shows. On A most lines stay
  hidden.
- **PUSH / POP**: the offset is set inside `glPushAttrib(GL_POLYGON_BIT)`;
  after the pop the lines are hidden again, and the status line reads the
  enable and units back: off and 0 on B, units -32 on A.

Only `units` is used: gl4es doesn't apply the factor, the slope part of the
offset, to lines.

## Against native desktop GL

In the OFFSET LINE row, B matches all four references in Chrome (Apple,
NVIDIA and Mesa radeonsi 0 px, Mesa iris 1); A differs by about 740 px.
Natively, B matches NVIDIA's desktop GL to the pixel. The two rows
without an offset z-fight, and that pattern differs between
implementations (39 to 287 px), so the whole-frame counts on hover include
it.

## The frame time

There's no performance change to show. A looks slower in Chrome only
because the PUSH / POP row's `glIsEnabled` and `glGetFloatv` go to WebGL on
A, where `GL_POLYGON_OFFSET_LINE` is unknown and the query waits for the
GPU; B answers them itself. Natively A and B take the same time.
