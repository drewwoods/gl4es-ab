# 05 · GL_COLOR_MATERIAL tracking the wrong face

## The bug

With `GL_COLOR_MATERIAL` enabled, the current color (`glColor`) replaces
one material parameter of one face, chosen by `glColorMaterial(face, mode)`.
GL has a single tracked face: after `glColorMaterial(GL_BACK, ...)` only
the back material follows `glColor`, and the front material keeps the
color it last tracked. When tracking stops, through a face switch or
`glDisable(GL_COLOR_MATERIAL)`, the material keeps the last color.

gl4es's GLES2 fixed-function emulation got this wrong in three ways:

- It stored a mode for each face, and its generated shader used the vertex
  color for **both** faces whenever `GL_COLOR_MATERIAL` was on. With
  two-sided lighting, a color meant for the back faces also painted the
  front ones.
- It never wrote the tracked color into the material, so when tracking
  stopped the material went back to its `glMaterial` value.
- `glGet*` of `GL_COLOR_MATERIAL_FACE` and `GL_COLOR_MATERIAL_PARAMETER`
  went to the driver, which rejects them on GLES2 and leaves the output
  unwritten. `glPushAttrib(GL_LIGHTING_BIT)` didn't save them either.

The fix is two commits: the first tracks one face and writes its material,
the second answers `glGet*` for the face and mode and saves them in
`glPushAttrib`.

## What the page shows

Two-sided lighting, one row per case. Each row draws a front-facing quad
(**FRONT FACE**) and a back-facing one (**BACK FACE**); the **EXPECTED**
column shows the correct front and back colors, unlit. The light is set up
so a lit quad shows its material color exactly. Every row starts with both
materials grey and `GL_COLOR_MATERIAL` off.

| Row | What it does | Front | Back |
|---|---|---|---|
| ONE FACE | `glColorMaterial(GL_FRONT)`, orange | orange | grey |
| SWITCH | track the front with white, switch to `GL_BACK`, then cyan | white | cyan |
| DISABLE | track both faces with cyan, then `glDisable(GL_COLOR_MATERIAL)` | cyan | cyan |
| PUSH | track the front with white, switch to `GL_BACK` inside `glPushAttrib(GL_LIGHTING_BIT)`, pop, then orange | orange | grey |

On A (master) every row is wrong: both faces follow the last color in
ONE FACE, SWITCH and PUSH, and DISABLE falls back to grey. The status
line also shows `glGetIntegerv` leaving both values unwritten.

With the first commit alone, the first three rows are right. PUSH still
shows white and orange, because the pop leaves the face on `GL_BACK`, and
the `glGet` values are still unwritten. The second commit fixes both.

## Against the native Mesa reference

`./build.sh 05-color-material native` also renders the page with Mesa's
desktop GL (`ref.png`). B matches it on every row, and Mesa reports the
same `glGet` values.

Mesa (25.2.8) has a bug here. `glEnable(GL_COLOR_MATERIAL)` should copy
the current color into the tracked material, and Mesa's code does, but
Mesa also queues `glMaterial` calls made outside `glBegin`/`glEnd`. If one
is still queued at the `glEnable`, it's applied after the copy and
overwrites it, so the material keeps the `glMaterial` value until the next
`glColor` that changes the color. Each row here starts with a
`glMaterial`, so the demo sets the current color to black before each row
and sets its colors after enabling: every row's `glColor` is then a change,
and Mesa tracks it.

## The frame time

There's no performance change to show here: A and B draw at the same
speed. An earlier version of this demo cleared GL errors with a
`glGetError` loop every frame. On A the rejected `glGet` left an error
that Chrome answered without asking the GPU. On B there was no error, so
each `glGetError` waited for the GPU and B looked about 10 ms slower. The
demo no longer calls `glGetError`.
