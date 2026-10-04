# 06 · Point smoothing and point size

## The bugs

Five commits, all about how gl4es draws `GL_POINTS`:

| Commit | Before |
|---|---|
| Draw GL_POINT_SMOOTH points round | `GL_POINT_SMOOTH` was tracked but ignored: smooth points were squares |
| Apply glPointSize to the points drawn after it only | gl4es draws immediate-mode geometry later, in a batch, and read the point size when it drew it, so a later `glPointSize` resized points already in the batch |
| Match GL's point-smooth coverage and alpha-test order | the first commit faded the outer 15% of the radius and applied the coverage after the alpha test; GL covers each pixel by how much of it is inside the circle, and applies that before the alpha test |
| Compute the fixed-function eye-space position in highp | the eye-space position was mediump; drivers that run mediump as 16-bit floats (NVIDIA, AMD's radeonsi) overflowed past 256 units, and drew those points at the maximum size or not at all |
| Default GL_POINT_SIZE_MAX to the largest point size | `GL_POINT_SIZE_MAX` started at 32, so larger points were drawn at 32; GL starts it at the implementation's largest point size |

The first two are gl-repl's patches; the last three are new.

## What the page shows

One row per case, each drawing four points left to right:

| Row | What it does | Correct |
|---|---|---|
| SMOOTH | `GL_POINT_SMOOTH` and blending, sizes 6, 12, 24 and 40 | round points, a one-pixel soft edge, the full size |
| ALPHA TEST | as SMOOTH, with `glAlphaFunc(GL_GREATER, 0.5)` | round points with a hard edge: the coverage under 0.5 fails the test |
| SIZE RESET | `glPointSize(24)`, four points, `glPointSize(1)` | four 24 px squares |
| TWO SIZES | `glPointSize(8)`, two points, `glPointSize(24)`, two points | two small squares, two large |
| LIST | the SIZE RESET calls compiled into a display list, then `glCallList` | four 24 px squares |

On A (master), the smooth points are squares and the 40 px ones are 32 px.
SIZE RESET and LIST are one-pixel dots, and TWO SIZES draws all four at 24 px.
The status line reports `GL_POINT_SIZE_MAX`, which should equal the largest
point size: 32 on A, the driver's maximum (511 in Chrome on a Mac) on B.

Every row ends with `glFlush`, so rows don't share a batch. The SMOOTH and
ALPHA TEST rows also flush before each size change, so they don't depend
on the `glPointSize` fix.

## Against native desktop GL

The **Native desktop GL** section shows the same demo built against
desktop OpenGL instead of gl4es, on four implementations: NVIDIA's driver,
Mesa's radeonsi (AMD) and iris (Intel), and Apple's OpenGL on an M2. B
should look like all of them.

Natively, gl4es with these commits (drawing through each machine's GLES2
driver) differs from the same machine's desktop GL by 0 pixels on the AMD
card, 192 on the NVIDIA card and 372 on the Intel GPU, all on the
antialiased point edges; master differs by 6,680 to 11,667. The desktop
implementations differ from each other by about as much (Apple and the
Mesa ones by 400 to 550 pixels).

Before the highp commit, gl4es's points were badly wrong on the NVIDIA and
AMD machines even on master: sizes past 256 units from the eye came out at
the maximum size (32 px then; the whole window once the cap was raised) or
went missing. Intel's iris and Chrome run mediump at 32 bits, which hid it.

## The frame time

No performance change: natively, B is within 1% of A on all three Linux
machines.
