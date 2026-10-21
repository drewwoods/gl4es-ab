# 07 · Polygon-mode lines and edge flags

## The bugs

Four commits, all about `glPolygonMode(GL_LINE)`, which draws each
polygon's edges instead of filling it, and one with TODOs:

| Commit | Before |
|---|---|
| Draw polygon-mode lines with glDrawArrays (Emscripten builds only) | gl4es generated an edge index array and called `glDrawElements`; on Emscripten every such draw uploads the indices and scans them in JavaScript, so wireframe scenes with many objects slowed down badly |
| Outline quads and quad strips under polygon-mode lines | a quad drawn on its own, and every quad of a quad strip, also showed its diagonal, from the triangles gl4es draws it as |
| Implement glEdgeFlag for polygon-mode lines | `glEdgeFlag` was a stub, so edges an application hides (the GLU tessellator does, for concave polygons) were drawn anyway, and `glGetBooleanv(GL_EDGE_FLAG)` raised `GL_INVALID_ENUM` |
| Draw every quad-strip edge whatever the edge flag | new with the commit above: after `glEdgeFlag(GL_FALSE)` a quad strip lost all its edges, though GL applies edge flags only to separate triangles, separate quads and polygons |
| Add TODOs for display-list edge-flag gaps | comments only, for two bugs left as gl-repl has them (rows LIST FLAG and LIST POP) |

The first three are gl-repl's patches, with two changes: the glDrawArrays
expansion applies to Emscripten builds only, since natively it only adds
copying (6-17% slower on this demo), and the edge-flag patch drops two
test-only hooks gl-repl's own bench used. The quad-strip commit is new to
gl-repl.

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
- **FLAG OFF**: the quad strip again, after `glEdgeFlag(GL_FALSE)`. Two
  boxes; A adds the diagonals, and before the quad-strip commit B drew
  nothing.
- **LIST FLAG**: two display lists of one quad each, with no `glEdgeFlag`
  in them. The quad should take the flag current at `glCallList`: the left
  list, compiled with the flag off and called with it on, draws four sides;
  the right one, compiled on and called off, draws nothing. B keeps the
  flag from compile time (a TODO), so it draws the right one only.
- **LIST POP**: a display list that hides a quad's edges between
  `glPushAttrib(GL_CURRENT_BIT)` and `glPopAttrib`, then a quad drawn after
  it. Only the second should show, and `glGetBooleanv(GL_EDGE_FLAG)` after
  the list should be `GL_TRUE`. B sets the list's last flag after it runs
  (a TODO), so it draws neither and the status line says "wrong".
- **GL_EDGE_FLAG**: `glGetBooleanv(GL_EDGE_FLAG)` after `glEdgeFlag(GL_FALSE)`.
  The status line says "correct" on B, "left unwritten" on A.
- **OBJECTS**: many small wireframe grids, each placed with its own
  `glPushMatrix`/`glTranslatef`, so each is its own draw, as in a scene.
  The `objects` slider sets how many.

The **Native desktop GL** section shows the same demo on desktop OpenGL;
hover one to mark where B differs from it. In Chrome, off the two TODO rows
(LIST FLAG 424 px, LIST POP 212), B matches Apple's and Mesa radeonsi's frames
exactly and differs from NVIDIA's by 20 px and Mesa iris's by 64, of line
rasterization, except for FLAG OFF on Mesa; A differs from each by 3,357 to
3,898 px.

Mesa doesn't follow GL on the FLAG OFF row: its desktop GL (radeonsi, iris
and llvmpipe) drops the quad strip's edges after `glEdgeFlag(GL_FALSE)` too,
while the GL reference page says "connected quadrilaterals are always marked
as boundary". Apple's and NVIDIA's GL draw them, and so does B; against Mesa
B differs by 477 px there.

Natively, gl4es with these commits matches each machine's desktop GL to the
pixel (NVIDIA, radeonsi, iris and llvmpipe), apart from the two TODO rows
and, on Mesa, FLAG OFF.

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
