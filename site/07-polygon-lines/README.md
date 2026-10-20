# 07 · Polygon-mode lines and edge flags

## The bugs

Three commits, all about `glPolygonMode(GL_LINE)`, which draws each
polygon's edges instead of filling it:

| Commit | Before |
|---|---|
| Draw polygon-mode lines with glDrawArrays (Emscripten builds only) | gl4es generated an edge index array and called `glDrawElements`; on Emscripten every such draw uploads the indices and scans them in JavaScript, so wireframe scenes with many objects slowed down badly |
| Outline quads and quad strips under polygon-mode lines | a quad drawn on its own, and every quad of a quad strip, also showed its diagonal, from the triangles gl4es draws it as |
| Implement glEdgeFlag for polygon-mode lines | `glEdgeFlag` was a stub, so edges an application hides (the GLU tessellator does, for concave polygons) were drawn anyway, and `glGetBooleanv(GL_EDGE_FLAG)` raised `GL_INVALID_ENUM` |

All three are gl-repl's patches, with two changes: the glDrawArrays
expansion applies to Emscripten builds only, since natively it only adds
copying (6-17% slower on this demo), and the edge-flag patch drops two
test-only hooks gl-repl's own bench used. A fourth commit only adds
TODOs for three edge-flag gaps gl-repl's patch leaves (see TODO.md).

## What the page shows

Each row's OpenGL is listed on the page under the frames. In short:

- **ONE QUAD**: one `GL_QUADS` quad. Its four sides; A adds the diagonal.
- **QUAD STRIP**: two quads in a `GL_QUAD_STRIP`. Two boxes; A adds a
  diagonal to each.
- **TRIANGLES**: a square from two `GL_TRIANGLES`, with `glEdgeFlag(GL_FALSE)`
  on the two vertices that start the shared diagonal. Four sides; A draws
  the diagonal.
- **POLYGON**: a `GL_POLYGON` pentagon with its top-right side hidden by an
  edge flag. Four sides; A draws all five.
- **GL_EDGE_FLAG**: `glGetBooleanv(GL_EDGE_FLAG)` after `glEdgeFlag(GL_FALSE)`.
  The status line says "correct" on B, "left unwritten" on A.
- **OBJECTS**: many small wireframe grids, each placed with its own
  `glPushMatrix`/`glTranslatef`, so each is its own draw, as in a scene.
  The `objects` slider sets how many.

The **Native desktop GL** section shows the same demo on desktop OpenGL;
hover one to mark where B differs from it. In Chrome, B matches Apple's
and Mesa radeonsi's frames exactly (0 px) and differs from NVIDIA's by 12
px and Mesa iris's by 66, of line rasterization; A differs from each by
about 2,450 px. Natively, gl4es with these commits matches each machine's
desktop GL to the pixel (NVIDIA, radeonsi, iris and llvmpipe).

## The frame time

The OBJECTS row is where the first commit shows. Measured in headless
Chrome on a Mac, each side alone:

| objects | A | B |
|---|---|---|
| 0 | 0.31 ms | 0.25 ms |
| 160 | 1.21 ms | 1.04 ms |
| 240 (default) | 1.58 ms | 1.18 ms |
| 420 | 468 ms | 4.6 ms |
| 600 | 398 ms | 2.8 ms |

Per frame, each object is four draws. A makes two buffer uploads per draw,
the indices and the vertices they reach, and B one, the expanded vertices:
at 480 objects, 3,850 uploads on A and 1,926 on B (B uploads more bytes,
0.98 MB against 0.44 MB). Past about 1,000 draws per frame A falls off a
cliff, likely Emscripten's pool of temporary buffers for client-side
arrays being reused while the GPU still reads them; B stays at a few
milliseconds. Where the cliff starts depends on the machine and browser:
in desktop Chrome on another Mac, A already took 113 ms per frame at 160
objects, against B's 2.0 ms. In gl-repl, a wireframe scene with about 130
such draws per frame went from 28.5 to 60 FPS.

Natively, drivers read client-side arrays in place, so the expansion would
only add copying: with it, B was 6-17% slower than A on this demo on Intel
and AMD. Native builds therefore keep the indexed draw, and with that B is
within noise of A there (-3% to +5%).
