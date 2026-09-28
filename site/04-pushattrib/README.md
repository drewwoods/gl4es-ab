# 04 · State leaking out of glPushAttrib/glPopAttrib

## The bug

`glPushAttrib(mask)` saves the state groups in `mask` and `glPopAttrib`
puts them back, so code can change state inside the pair without
affecting what's drawn after it. gl4es left several groups, or parts of
them, as TODOs: whatever was set inside the pair stayed set after the pop.

| Group | Missing on master |
|---|---|
| `GL_POLYGON_BIT` | the whole group: front face, cull face mode, polygon mode, cull and offset-fill enables |
| `GL_LINE_BIT` | the line stipple: enable, factor, pattern |
| `GL_POINT_BIT` | the point parameters: size min/max, fade threshold, distance attenuation |
| `GL_TRANSFORM_BIT` | the clip-plane equations (the enables were restored) |
| `GL_TEXTURE_BIT` | the texture environment: `GL_TEXTURE_ENV_MODE`, `GL_TEXTURE_ENV_COLOR` |

The fix is five commits, one per group.

## What the page shows

One row per group, drawing the same thing three times: **BEFORE** the
push, with the state as it was set before `glPushAttrib(group)`;
**INSIDE**, after changing that state inside the pair; and **AFTER** the
`glPopAttrib`. The pop should put everything back as it was, so AFTER
should always look like BEFORE:

| Row | BEFORE, and AFTER when correct | INSIDE |
|---|---|---|
| POLYGON | a green filled triangle: culling keeps the CCW one | `glFrontFace(GL_CW)` and `glPolygonMode(GL_LINE)`: outlines (gl4es doesn't cull polygon-mode lines, a separate issue) |
| STIPPLE | solid lines | stipple enabled, pattern `0x0F0F`: dashed lines |
| POINTS | five points of the same size | distance attenuation: points shrink left to right, away from the eye |
| CLIP | the bottom half, cut by the plane set before the push | a plane keeping the top half |
| TEXENV | a red checkerboard (`GL_MODULATE`) | `GL_REPLACE`: a grey checkerboard |

On A (master), every AFTER box looks like its INSIDE box instead. On B it
matches BEFORE.

Each row puts its state back to the default when it's done, so rows don't
affect each other.

## Against the native Mesa reference

`./build.sh 04-pushattrib native` also renders the page with Mesa's desktop
GL (`ref.png`). In the AFTER column B matches it on every row except
CLIP PLANE, where Mesa (25.2.8, Intel) shows the top half, like A. That's
a Mesa bug: it restores the plane's equation on `glPopAttrib` but keeps
clipping with the old one while the plane's enable doesn't change;
toggling `GL_CLIP_PLANE0` off and on after the pop makes Mesa show the
bottom half too. The GL 2.1 spec puts the clip-plane coefficients in
`GL_TRANSFORM_BIT`, so B is right.

The INSIDE column differs from Mesa in ways that have nothing to do
with push/pop: gl4es doesn't cull polygon-mode lines (so it outlines both
triangles), its emulated line stipple starts the dash pattern at a
different phase, and Mesa's reference didn't shrink the attenuated points.

## The frame time

The Frame time panel shows B faster than A on this page. That isn't a
speedup from the patches, which add a little work to each pop: it's a side
effect of the leak. On A the leaked stipple makes the AFTER lines go
through gl4es's line-stipple emulation, which uses a texture and costs far
more on WebGL than plain lines; B draws them plain. Stippling B's AFTER
lines on purpose makes B the slower one. Don't quote this page's timing
as a performance result.
