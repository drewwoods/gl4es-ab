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
| point-smooth, point-size-batch | #522, merged | same code, plus a comment commit (TODOs) and the highp fix below, both new to gl-repl |
| accum-fbo | PR10 (`pr-10-accum`), prepared | **diverges**: drops the stray `names.txt` the patch adds, braces the probe's SHUT_LOGD if/else (gcc 14 -Wdangling-else: the else bound to SHUT_LOGD's inner if), then three new commits: `glClearAccum` clamps to [-1, 1] and `glGetIntegerv(GL_ACCUM_CLEAR_VALUE)` maps [-1, 1] onto [-INT_MAX, INT_MAX]; the RGBA16F probe no longer samples the texture it draws into (WebGL rejects the feedback loop, so in Chrome the buffer was always RGBA8 and `GL_ACCUM_RED_BITS` read 8; visible to gl-repl only where its deferred path falls back to the FBO); TODO comments (under Known gaps). Demo 10 with B1 (PR10 plus gl-repl's other four patches) is pixel-identical to PR10 alone. gl-repl A/B on 2026-10-27 (B1 = e3acd7c6; B2 = c779d056, PR10 without the deferred patches): no oracle regressions in either; the standard catalog (accumulation off) unchanged, 0 of 43; with 16 passes, B1 pixel-identical to gl-repl on all 43 AA and 12 Blur and Blur Cam captures, B2 different only at quantization (at most 889 px over 2.5 levels, peak 13), the RGBA16F FBO sum against the deferred shader sum; `GL_ACCUM_RED_BITS` 8 on gl-repl, 16 on B1 and B2. FPS (headless Chrome, Metal on M2, 16 passes, 3 to 5 runs): B1 equal to gl-repl within noise; B2 5-9% slower in every effect (logo AA 27.47 against 30.22, -9.1% or +3.3 ms/frame; cube AA -6.2%, Blur -7.9%, Blur Cam -5.1%), its best run below gl-repl's worst |
| accum-deferred-return, accum-deferred-scissor | not yet | performance only; follow-up PRs after PR10. deferred-return conflicts with PR10 at the probe: its `realize_accumenv(a, a->program, a->uscale, 0.f)` goes after PR10's `gles_glBindTexture(GL_TEXTURE_2D, 0)` (resolved in cc25f3d0); scissor and the other patches apply cleanly. gl-repl measured +9.05% and +4.08% FPS (2026-08-24), but that baseline's FBO was RGBA8 in Chrome (the probe bug above). Against PR10's RGBA16F (2026-10-27), together they're worth 5-9% (+2.5 to 4 ms/frame on M2; see the accum-fbo row) |
| polygon-line-drawarrays, polygon-line-quad-edges, edge-flag | #524, merged | **diverges**: a new fourth commit draws every quad-strip edge whatever the edge flag, as GL does (gl-repl A/B on 2026-10-20: 0 of 43 differ, no oracle regressions; a latent fix there, since gl-repl installs glEdgeFlag as the GLU tessellator's edge-flag callback and never resets it, so a quad strip drawn under vertex outlines or hidden-line mode after a tessellation would lose its edges), and a fifth adds TODO comments only (edge-flag gaps, under Known gaps); drawarrays expansion is Emscripten-only and no longer zeroes a renderlist_t per draw (both invisible to gl-repl, which is web-only); edge-flag drops the `gl4es_edgeflag_test_*` hooks, so gl-repl's `packaging/web/bench/gl4es_edge_flag.c` needs another way to check the edge-flag allocation when it fast-forwards: only `test_fill_prebegin_capacity` uses the hooks; plan is to drop its capacity assertion and keep the coverage check (user decides at the fast-forward). gl-repl A/B with its remaining stack on PR07: 0 of 43 catalog examples differ, no oracle regressions |
| polygon-offset-line | #525, merged | **diverges**: a second commit adds TODO comments only (four gaps, under Known gaps); factor/units live in `glstate` instead of the `gl4es_mirror_polygon_offset_*` globals, the startup glGetError drain is bounded (16 tries), and its glLineWidth hunk and `gl4es_mvp_load_identity()` moved to PR09 (combined code after PR09 is unchanged); none of it is visible to gl-repl. gl-repl A/B (with PR09): 0 of 43 catalog examples differ from gl-repl's patch |
| line-width-quads | PR09 (`pr-09-line-width`, rebased onto c1d7d014 after #525 to #530, patch-identical), next | gl-repl's code as its first commit (plus PR08's moved glLineWidth hunk), then **two new commits**: wide lines widened along the minor axis as GL specifies (visible in gl-repl: diagonal wide lines get thinner, ends axis-aligned), and quads used only where the driver's largest width is 1 (invisible on the web). gl-repl A/B on 2026-10-15 (B = PR09 tip plus the accum patches): no oracle regressions, render bench unchanged; commit 1 alone is pixel-identical to gl-repl's patch (0 of 43); the tip changes 27 of 43 examples, all from commit 2: diagonal wide lines thinner by about cos(angle) (0.83 measured against 0.87 expected on the logo's 30° outline), horizontal and vertical lines unchanged. Example 3's red X axis is coplanar with the disc, so its depth tie flips; neither is more correct. Port note from PR07 still applies: it didn't apply on top of PR07: its `} /* !wide_drew */` closing brace in listdraw.c must now go just before PR07's `if (use_line_arrays && line_arrays == &transient_line_arrays)` guard around `clear_line_arrays()`, so the clear stays outside the wide-line block; regenerate gl-repl's patch with that when it moves past PR07 |

#522 adds code gl-repl doesn't have: `src/gl: Compute the fixed-function
eye-space position in highp`. It changes nothing in gl-repl's web catalog
(Chrome runs mediump at 32 bits).

## Deferred fixes, with code ready

- **GL_POINT_SIZE_MAX starts at 32** instead of the implementation's largest
  point size; a TODO at the default in `glstate.c` says so. Branch
  `todo/point-size-max` (61f1dc62, on PR06's tip 05b7cc4b, so it
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
- #524 polygon-mode lines: a batch that mixes separate triangles or quads
  with a triangle strip or fan loses all its edge flags (it falls back to
  the edge list without them), and glEdgeFlagPointer (edge flags in vertex arrays) is still a stub.
  Two more from a review of the edge-flag commit, each reproduced in
  Chrome against desktop GL and shown in demo 07 (rows LIST FLAG and LIST
  POP), left as gl-repl has them, with a TODO at each in PR07's last
  commit:
  - A display list with geometry but no glEdgeFlag records the flag current
    at compile time (`listrl.c`), so glEdgeFlag before glCallList has no
    effect: compiled under GL_FALSE, it stays invisible, and compiled under
    GL_TRUE it can't be hidden.
  - glCallList sets GL_EDGE_FLAG to the list's last flag after it runs
    (`gl4es.c`), overwriting a glPopAttrib(GL_CURRENT_BIT) restore inside
    the list, which also hides edges in later draws. Flag changes should be
    replayed in command order instead.
  A third, quad strips hiding their edges under glEdgeFlag(GL_FALSE), is
  fixed in PR07 (new to gl-repl). Mesa's desktop GL (radeonsi, iris,
  llvmpipe) has the same bug; Apple's and NVIDIA's (Quadro K1200, 580.173)
  don't.
- PR08 polygon offset for lines: only `units` is applied to lines, not the
  slope-dependent `factor`; GL_POLYGON_OFFSET_POINT isn't tracked. Four
  more from a review, each reproduced in WebGL2 against desktop GL and left
  as gl-repl has them, with a TODO at each in PR08's second commit:
  - The bias is applied to clip-space z, so it also moves the near and far
    clip planes: a triangle on the near plane loses its outline under
    glPolygonOffset(0, -32) (0 px vs 268). GL offsets depth after clipping,
    and clamps it (GL 2.1 3.5.5). Only lines within the offset of a plane
    are affected.
  - The bias is in the projection matrix, so a program that writes
    gl_Position without it (gl_Position = gl_Vertex) gets no offset.
  - The depth bits are probed once, from the first framebuffer. On another
    with a different depth size the offset is wrong by the difference: 256
    times too small going from 24 to 16 bits (the outlines z-fight again),
    256 times too large from 16 to 24 (hidden edges show through).
  - The probe drains the driver's error flags first, so on Emscripten,
    where it runs on the first glViewport, an error the application hadn't
    read yet is lost (glDepthFunc(0), glViewport, glGetError gives
    GL_NO_ERROR). gl-repl's drain is unbounded but has the same effect.
- PR10 accumulation buffer (TODOs in the code, in its last commit):
  GL_ADD and GL_MULT do nothing (demo 10's TRAIL panel); glAccum and
  glClearAccum aren't recorded in display lists, and while a list is
  compiled glAccum draws at once with its own state calls recorded into the
  list; the framebuffer copy is RGB, so a read buffer's alpha is lost; GLES
  3 refuses to copy from a multisampled read framebuffer, so natively a
  multisampled window gets nothing from GL_ACCUM and GL_LOAD (WebGL
  resolves it); the color mask applies when the accumulation buffer is
  cleared, though GL doesn't mask it.
- PR09 wide lines: emulated widths stop at 64 px, GL_LINE_SMOOTH isn't
  applied to the quads, and with a clip plane enabled wide lines are still
  one pixel wide on WebGL. Natively, gl4es's wide lines (drawn by the GLES
  driver) differ from the same machine's desktop GL by about 330 px on demo
  09, before and after PR09.

## Found while testing, not fixed

- **mediump in fixed-function vertex shaders.** gl4es gives its generated
  shaders a mediump default (`wanthighp = !fpeShader` in shaderconv.c),
  vertex shaders included. PR06 makes the eye-space position and the point
  distance highp; other values computed from eye-space positions (lighting
  vectors, fog distance, clip-plane distances) can still overflow past 256
  units on drivers that run mediump as fp16 (NVIDIA, radeonsi, mobile GPUs).
  Making the whole vertex shader highp needs care: uniforms shared with the
  fragment shader must keep matching precision.

## gl4es-ab harness

- On zen3 (NVIDIA), the native target left A's and B's screenshots as PPM
  while the reference converted to PNG.
- gl4es A/B natively on macOS would need a GLES2 driver: ANGLE's libEGL and
  libGLESv2 over Metal. Only the desktop-GL reference runs there now.
- An NVIDIA reference for demo 10 needs a GLX context with an accumulation
  buffer (EGL configs have none) and an X display on zen3 or pentiumd. Mesa
  can't give one: none of iris's 992 GLX configs has an accumulation buffer.
- Each A/B page times both sides on load, pausing one at a time for about
  20 s. A paused side keeps its last frame, so freezing or hovering a
  native reference in that time compares a stale frame of an animated
  demo. Static demos (01-09) don't show it.
