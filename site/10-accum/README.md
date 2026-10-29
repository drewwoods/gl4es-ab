# 10 · The accumulation buffer

## The bug

GLES has no accumulation buffer, so in gl4es `glAccum` and `glClearAccum`
were stubs. A program that draws its scene several times and averages the
passes with `glAccum`, the fixed-function way to get antialiasing, motion
blur or soft shadows, showed only its last pass. `GL_ACCUM_RED_BITS` and
the other bit counts went to the driver, which doesn't know them.

gl4es now keeps the accumulation buffer in a texture attached to a
framebuffer object of its own: `GL_ACCUM` and `GL_LOAD` copy the
framebuffer and add or load it with a small shader, `GL_RETURN` draws it
back, and `glClear(GL_ACCUM_BUFFER_BIT)` clears it. It is half-float where
the driver can render and blend into that, else 8-bit, and
`GL_ACCUM_RED_BITS` says which (16 or 8). This is gl-repl's accum-fbo patch
with three more commits:

- `glClearAccum` clamps to [-1, 1], and `glGetIntegerv` maps
  `GL_ACCUM_CLEAR_VALUE` like the clear color, so 0.5 reads back as
  1073741823 rather than 0.
- The half-float probe no longer samples the texture it draws into. WebGL
  rejects that as a feedback loop, so in a browser the buffer was always
  8-bit: each pass's share of the average was rounded to 8 bits, leaving
  16-pass averages about 4 levels (of 255) off, up to 10.
- TODO comments for the gaps (see TODO.md).

gl-repl also speeds up its 16-pass antialiasing by deferring the passes to
`GL_RETURN` and copying only the scissored area. Those are separate
performance patches, left for later PRs.

## What the page shows

Each panel's OpenGL is listed on the page under the frames. Each draws its
scene with the scissor box set to the panel, which bounds every
accumulation operation, so the panels accumulate separately. On A,
`glAccum` does nothing and each panel shows its last pass.

- **JITTER AA**: a spinning cube drawn once per pass with the projection
  moved by a fraction of a pixel, averaged with `GL_LOAD` then `GL_ACCUM`:
  smooth edges on B, jagged ones on A.
- **MOTION BLUR**: a propeller drawn at moments across a 1/24 s shutter,
  after `glClear(GL_ACCUM_BUFFER_BIT)`: B smears the blades into fans, A
  shows three sharp blades.
- **SOFT SHADOW**: a planar projected shadow (the cube flattened onto the
  ground by a shadow matrix), lit from points across an area light: B's
  shadow has a soft penumbra, A's is hard.
- **TRAIL (MULT)**: a known gap. The ball's last 8 positions, each added
  after `glAccum(GL_MULT, 0.6)` fades what's there: desktop GL draws a
  fading tail. gl4es doesn't implement `GL_MULT` (or `GL_ADD`), so B draws
  all 8 balls equally bright, and A shows only the last.

The status line reads `GL_ACCUM_RED_BITS` (0 on A, 16 on B, 32 on Apple)
and `GL_ACCUM_CLEAR_VALUE` through `glGetIntegerv`. The `passes` slider
sets the number of passes for the first three panels.

## Against native desktop GL

The Apple reference draws into an off-screen window, as only a window has
an accumulation buffer (`ACCUM=1` in `refs`). Mesa's desktop GL has none
to compare against: none of iris's GLX configs has one, and EGL configs
never do. An NVIDIA reference needs a GLX context in the harness.

Against Apple's GL, B differs by 0 px in JITTER AA and SOFT SHADOW and 1 px
in MOTION BLUR in Chrome, and by 0 px in all three natively on Mesa iris's
GLES, every channel within 5 levels. A differs by 347, 6353 and 2187 px.
TRAIL differs by 813 px on B (784 on A), from the missing `GL_MULT`;
PR13 implements it, with `GL_ADD` (demo 13).

## The frame time

B is slower because it does the work: at 16 passes, 56 framebuffer
copies and 60 quad draws per frame, where A's `glAccum` does nothing. In Chrome
A takes 1.21 ms and B 1.95 ms; natively on Mesa iris, 2.1 and 8.0 ms.
