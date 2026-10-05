# gl4es upstreaming: running list

What's left out of the PRs to ptitSeb/gl4es on purpose, what's known to be
missing, and where each of gl-repl's gl4es patches stands. Branches named
here live in the gl4es fork (drewwoods/gl4es); `todo/*` branches are local
until pushed.

## gl-repl patch status

gl-repl applies its gl4es patches on a pinned upstream commit
(`scripts/web-deps.sh`). When a PR lands, gl-repl can move the pin to the
merge and drop the patches it carries.

| gl-repl patch | Upstream | Notes |
|---|---|---|
| rasterpos-perspective-divide | #517, merged | |
| bitmap-dirty-clear | #518, merged | |
| getter-client-state | #519, merged | rewritten: state kept in glstate, clamping and [0, INT_MAX] mapping fixed |
| getbooleanv-local-state | #519, merged | |
| pushattrib-gaps, pushattrib-texenv | #520, merged | split into five commits, same code |
| color-material-face | #521, merged | |
| point-smooth, point-size-batch | PR06 (`pr-06-points`), open | same code, plus a comment commit and the highp fix below, both new to gl-repl |
| accum-fbo, accum-deferred-return, accum-deferred-scissor | not yet | needs braces for gcc-14 -Wdangling-else in accum_probe's SHUT_LOGD |
| polygon-line-drawarrays, polygon-line-quad-edges, edge-flag | not yet | |
| polygon-offset-line | not yet | after #519 it keeps two mirror globals in getter.c; move them into glstate for upstream |
| line-width-quads | not yet | |

PR06 adds code gl-repl doesn't have: `src/gl: Compute the fixed-function
eye-space position in highp`. It changes nothing in gl-repl's web catalog
(Chrome runs mediump at 32 bits).

## Deferred fixes, with code ready

- **GL_POINT_SIZE_MAX starts at 32** instead of the implementation's largest
  point size; a TODO at the default in `glstate.c` says so. Branch
  `todo/point-size-max` (1a5ea5b4, on PR06's tip d1325c4f, so it
  fast-forwards): the default becomes the driver's GL_ALIASED_POINT_SIZE_RANGE maximum, asked for
  lazily without the hardware test, falling back to 32. Checked against
  NVIDIA, Mesa radeonsi/iris and Apple; 0 px change in gl-repl's catalog.
  Dropped from PR06 to keep it to gl-repl's tested behaviour. Needs the
  highp fix first: before it, NaN point sizes were hidden by the 32 cap.
- **GL_POINT_SMOOTH coverage** fades a fixed 15% of the radius and is applied
  after the alpha test; GL covers each pixel by its part inside the circle,
  before the alpha test (GL 2.1 3.12, 4.1.4). A TODO in `fpe_shader.c`
  describes it. Branch `todo/point-smooth-coverage` (953d72ad, on d1cb948b,
  before PR06's comment commit, so rebasing it means dropping that TODO)
  has the fix: `clamp(0.5 + r - d, 0, 1)` in pixels, from the point size in
  a varying. Measured coverage matches Apple's GL; demo 06's README has the
  numbers. Dropped because it changes the edge of every smooth point in
  gl-repl's scenes (23 of 43 examples). Don't use Mesa's
  `nir_lower_point_smooth` formula (`clamp(r - d)`): it makes 2-5 px points
  30-70% too faint.

## Known gaps in landed PRs

- #517 rasterpos (TODOs in the code): glDrawPixels ignores the raster-position
  valid flag; glWindowPos3f still invalidates negative coordinates (#495);
  GL_CURRENT_RASTER_POSITION_VALID isn't queryable.
- #519 getters: glViewport/glScissor drop a negative origin from the tracked
  state (#495), so glGet returns the previous box. glGetBooleanv's
  multi-value pnames, and most enable flags (GL_BLEND, GL_DEPTH_TEST, ...) in
  every glGet*, still go to the driver, though glIsEnabled answers them
  locally.
- #520 glPushAttrib: GL_POLYGON_BIT's polygon smooth/stipple enables and
  offset factor/units aren't tracked; GL_COMBINE texenv settings aren't saved;
  on a GLES1 backend clip-plane equations aren't restored.
- #521 color material: glColor inside glBegin/glEnd while compiling a display
  list already updates the current color, and now the tracked material too.

## Found while testing, not fixed

- **mediump in fixed-function vertex shaders.** gl4es gives its generated
  shaders a mediump default (`wanthighp = !fpeShader` in shaderconv.c),
  vertex shaders included. PR06 makes the eye-space position and the point
  distance highp; other values computed from eye-space positions (lighting
  vectors, fog distance, clip-plane distances) can still overflow past 256
  units on drivers that run mediump as fp16 (NVIDIA, radeonsi, mobile GPUs).
  Making the whole vertex shader highp needs care: uniforms shared with the
  fragment shader must keep matching precision.
- **GL_ACCUM_CLEAR_VALUE** from glGetIntegerv casts to GLint instead of
  mapping [0, 1] to [0, INT_MAX] like the other clear values (accum PR).

## gl4es-ab harness

- On zen3 (NVIDIA), the native target left A's and B's screenshots as PPM
  while the reference converted to PNG.
- gl4es A/B natively on macOS would need a GLES2 driver: ANGLE's libEGL and
  libGLESv2 over Metal. Only the desktop-GL reference runs there now.
