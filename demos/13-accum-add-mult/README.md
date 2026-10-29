# 13 · glAccum(GL_ADD) and glAccum(GL_MULT)

## The bug

PR10's accumulation buffer left `GL_ADD` and `GL_MULT` doing nothing, so a
program couldn't fade, brighten or offset what it had accumulated: demo
10's TRAIL panel draws every ball of the trail at full weight.

| Commit | |
|---|---|
| Implement GL_ADD and GL_MULT for the accumulation buffer | `GL_MULT` scales the weights of a pending deferred batch (`GL_LOAD`, then `GL_ACCUM`s, kept until `GL_RETURN` by PR11), or else blends a quad of the constant value into the buffer as dst × value. `GL_ADD` commits a pending batch, then blends the constant in as dst + value, subtracting for a negative value |

A is PR12's tip: PR10 plus gl-repl's deferred-return and deferred-scissor
patches, which PR11 and PR12 bring upstream first.

## What the page shows

Each panel's OpenGL is listed on the page under the frames:

- **TRAIL (MULT 0.6)**: demo 10's trail. The ball's last 8 positions, each
  added after `glAccum(GL_MULT, 0.6)` fades what's there: a fading tail on
  B, 8 equally bright balls on A. With no batch pending, this is the
  blended quad.
- **MULT 2**: a test card loaded at half brightness and multiplied by 2:
  as drawn on B, half brightness on A. `GL_MULT` scales the pending batch.
- **ADD 0.2**: the card loaded at half brightness, 0.2 added, returned at
  twice that: 0.4 lighter on B, as drawn on A.
- **ADD -0.2**: the same, subtracting: 0.4 darker on B, the dark bars
  black. The buffer holds the negative values until `GL_RETURN` clamps
  them.

The status line reads one pixel from each of the last three panels, with
the value desktop GL gives.

## Against native desktop GL

The Apple reference draws into an off-screen window, as for demo 10
(`ACCUM=1` in `refs`). Mesa's desktop GL has no accumulation buffer to
compare against.

B matches Apple's GL to the pixel, in Chrome and natively on Mesa iris's
GLES (every channel within 1 level there). A differs by 71,412 px, 813 of
them in TRAIL.

Not shown: the 8-bit fallback, used where the driver can't render to a
half-float texture, where `GL_MULT`'s value clamps to [0, 1] outside a
batch. Chrome and Mesa iris both get the half-float buffer.

## The frame time

B does work A skips: the trail's 8 `GL_MULT` quads, and in each `GL_ADD`
panel, committing the batch and a quad. In Chrome A takes 6.64 ms and B
7.33 ms; natively on Mesa iris, 7.0 and 9.8 ms.
