# 06 · Point smoothing and point size

## The bugs

Four commits, all about how gl4es draws `GL_POINTS`:

| Commit | Before |
|---|---|
| Draw GL_POINT_SMOOTH points round | `GL_POINT_SMOOTH` was tracked but ignored: smooth points were squares |
| Apply glPointSize to the points drawn after it only | gl4es draws immediate-mode geometry later, in a batch, and read the point size when it drew it, so a later `glPointSize` resized points already in the batch |
| Note where point smoothing and GL_POINT_SIZE_MAX differ from GL | comments only: TODOs where the smooth-point coverage is a fixed 15% fade applied after the alpha test (GL covers each pixel by its part inside the circle, before the alpha test), and where `GL_POINT_SIZE_MAX` starts at 32 (GL starts it at the largest point size) |
| Compute the fixed-function eye-space position in highp | the eye-space position was mediump; drivers that run mediump as 16-bit floats (NVIDIA, AMD's radeonsi) overflowed past 256 units, and drew those points at the maximum size or not at all |

The first two are gl-repl's patches. Two fixes tested here are kept for
later, with their code on branches: the smooth-point coverage, and
`GL_POINT_SIZE_MAX` starting at the largest point size instead of 32. See
TODO.md in this repo.

## What the page shows

Five rows of four points. Each row is the OpenGL below (the points sit at
x = 150, 232, 314 and 396), and should look as described.

**SMOOTH**: four round points, 6 to 40 px wide. On A they're squares. GL
antialiases about one pixel at the edge; gl4es fades the outer 15% of the
radius (see below), and A and B both draw the 40 px point 32 px wide,
because gl4es starts `GL_POINT_SIZE_MAX` at 32.

```c
glEnable(GL_POINT_SMOOTH);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glPointSize(6);  glBegin(GL_POINTS); glVertex2f(150, 296); glEnd(); glFlush();
glPointSize(12); glBegin(GL_POINTS); glVertex2f(232, 296); glEnd(); glFlush();
glPointSize(24); glBegin(GL_POINTS); glVertex2f(314, 296); glEnd(); glFlush();
glPointSize(40); glBegin(GL_POINTS); glVertex2f(396, 296); glEnd(); glFlush();
```

**ALPHA TEST**: the same, with an alpha test. GL applies a smooth point's
coverage to alpha before the alpha test, so the soft edge, where coverage
is under 0.5, is cut off: round points with a hard edge. On A, squares; on
B, round points that keep their soft edge (see below).

```c
glEnable(GL_POINT_SMOOTH);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glEnable(GL_ALPHA_TEST);
glAlphaFunc(GL_GREATER, 0.5f);
glPointSize(6);  glBegin(GL_POINTS); glVertex2f(150, 232); glEnd(); glFlush();
/* ... 12, 24 and 40, as in SMOOTH */
```

**SIZE RESET**: four 24 px squares. The `glPointSize(1)` after them must
not change them; on A it shrinks them to one-pixel dots.

```c
glPointSize(24);
glBegin(GL_POINTS);
glVertex2f(150, 168); glVertex2f(232, 168); glVertex2f(314, 168); glVertex2f(396, 168);
glEnd();
glPointSize(1);
glFlush();
```

**TWO SIZES**: two 8 px squares, then two 24 px ones. On A, all four are
24 px.

```c
glPointSize(8);
glBegin(GL_POINTS); glVertex2f(150, 104); glVertex2f(232, 104); glEnd();
glPointSize(24);
glBegin(GL_POINTS); glVertex2f(314, 104); glVertex2f(396, 104); glEnd();
glFlush();
```

**LIST**: the SIZE RESET calls, compiled into a display list: four 24 px
squares. On A, one-pixel dots.

```c
glNewList(list, GL_COMPILE);   /* once, in demo_init */
glPointSize(24);
glBegin(GL_POINTS);
glVertex2f(150, 40); glVertex2f(232, 40); glVertex2f(314, 40); glVertex2f(396, 40);
glEnd();
glPointSize(1);
glEndList();

glCallList(list);              /* every frame */
glFlush();
```

The status line reports `glGetFloatv(GL_POINT_SIZE_MAX)`, which should
start at the largest point size, `GL_ALIASED_POINT_SIZE_RANGE`'s maximum
(511 in Chrome on a Mac). gl4es starts it at 32, on A and B; the fix is on
the `todo/point-size-max` branch.

gl4es draws immediate-mode geometry later, in a batch. Each row ends with
`glFlush`, so rows don't share a batch, and SMOOTH and ALPHA TEST flush
after each point, so they test smoothing alone, not the `glPointSize` fix.

## Against native desktop GL

The **Native desktop GL** section shows the same demo built against
desktop OpenGL instead of gl4es, on four implementations: NVIDIA's driver,
Mesa's radeonsi (AMD) and iris (Intel), and Apple's OpenGL on an M2. B
should look like all of them.

Hover a reference to mark in magenta where B's frame differs from it, or
hold Shift for A's. In Chrome on a Mac:

| Native reference | B differs by | A differs by |
|---|---|---|
| Apple M2 | 1,552 px | 6,704 px |
| Mesa iris (Intel UHD) | 1,404 px | 6,680 px |
| Mesa radeonsi (RX 5700 XT) | 1,376 px | 6,760 px |
| NVIDIA RTX 5050, driver 610.43 | 1,430 px | 6,774 px |

B's differences are all in the smooth points: the 40 px ones are drawn
32 px wide (`GL_POINT_SIZE_MAX`), and gl4es fades the outer 15% of the
radius, so its large points look slightly smaller and softer than desktop
GL's, and keep that soft edge under the alpha test.
The TODO in `fpe_shader.c` describes the GL behaviour and a fix: GL's
coverage is the part of each pixel inside the circle, about
`clamp(0.5 + r - d, 0, 1)` in pixels, applied before the alpha test. With both
deferred fixes, B came within 308 to 564 px of the four references. Measured as a point's
total brightness over its area, πr²:

| | 2 px | 3 px | 5 px | 8 px | 12 px |
|---|---|---|---|---|---|
| Apple M2 | 1.06 | 1.04 | 1.01 | 1.00 | 1.00 |
| NVIDIA | 1.25 | 1.14 | 0.91 | 0.87 | 0.89 |
| Mesa iris | 0.32 | 0.81 | 0.87 | 0.86 | 0.90 |
| Mesa radeonsi | 0.32 | 0.47 | 0.67 | 0.77 | 0.85 |
| gl4es with that fix | 0.66 | 1.04 | 1.01 | 0.95 | 0.97 |

It changes the antialiased edge of every smooth point, so it's left for a
separate change.

Before the highp commit, gl4es's points were badly wrong on the NVIDIA and
AMD machines even on master: sizes past 256 units from the eye came out at
the maximum size (32 px then; the whole window once the cap was raised) or
went missing. Intel's iris and Chrome run mediump at 32 bits, which hid it.

## The frame time

No performance change: natively, B is within 1% of A on all three Linux
machines.
