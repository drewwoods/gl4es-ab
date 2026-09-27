/* 00-bitmap-left-clip: glBitmap bitmaps that cross the window's left edge.
 *
 * The same 32x32 "F" is drawn five times with glBitmap. A grey outline,
 * drawn with GL_LINE_LOOP, marks the full box each bitmap would cover.
 * Correct output shows the part of each F inside the window, in place
 * within its outline:
 *   green   fully inside (control)
 *   cyan    crossing the left edge: the right 20 columns show
 *   yellow  crossing only the right edge: the left 20 columns show, in A as
 *           well: only the left edge is mis-clipped
 *   orange  crossing the bottom-left corner: the top-right 20x20 shows (on
 *           master this one writes before the start of the bitmap buffer)
 *   red     adjustable (red.x, red.y, red.xorig, red.yorig); starts entirely
 *           left of the window, where nothing should show
 * Raster positions are set under glOrtho (w = 1) and are never negative, so
 * only glBitmap's clipping is exercised, not glRasterPos. */
#include "ab.h"

#define F 32
static GLubyte glyph[F * F / 8];

enum { INSIDE, LEFT, RIGHT, CORNER, MOVABLE, NCASES };

/* x, y: raster position; xorig, yorig: glBitmap origin, so the F's box
 * starts at (x - xorig, y - yorig). */
static struct {
    GLfloat x, y, xorig, yorig;
    GLfloat r, g, b;
} cases[NCASES] = {
    [INSIDE]  = { .x = 200, .y = 160,                            .r = 0.3f, .g = 0.9f,  .b = 0.4f },
    [LEFT]    = { .x =   8, .y = 200, .xorig = 20,               .r = 0.2f, .g = 0.8f,  .b = 0.9f },
    [RIGHT]   = { .x = 460, .y = 200,              .yorig = 20,  .r = 1.0f, .g = 0.85f, .b = 0.2f },
    [CORNER]  = { .x =   8, .y =   8, .xorig = 20, .yorig = 20,  .r = 1.0f, .g = 0.55f, .b = 0.1f },
    [MOVABLE] = { .x =   8, .y = 100, .xorig = 60,               .r = 1.0f, .g = 0.25f, .b = 0.2f },
};

void demo_init(void)
{
    /* An "F", 6 px strokes: stem on the left, bars at the top and middle.
     * Row 0 is the bottom row. Asymmetric, so a shifted copy is obvious. */
    for (int y = 0; y < F; y++)
        for (int x = 0; x < F; x++)
            if (x < 6 || y >= F - 6 || (y >= 13 && y < 19 && x < 24))
                glyph[y * (F / 8) + x / 8] |= 0x80 >> (x % 8);

    /* x and y stay >= 0 (master's glRasterPos rejects negative window
     * coordinates); the origins move the box past the edges. */
    ab_param("red.x", &cases[MOVABLE].x, 0, AB_W, 1);
    ab_param("red.y", &cases[MOVABLE].y, 0, AB_H, 1);
    ab_param("red.xorig", &cases[MOVABLE].xorig, -40, 120, 1);
    ab_param("red.yorig", &cases[MOVABLE].yorig, -40, 120, 1);
}

void demo_draw(double t)
{
    (void)t;
    glViewport(0, 0, AB_W, AB_H);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, AB_W, 0, AB_H, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Per case: the outline of the full box (through pixel centres), then
     * the bitmap. The outline is a draw call, so it also flushes gl4es's
     * pending glBitmap batch: each case lands in a batch of its own. */
    for (int i = 0; i < NCASES; i++) {
        float x0 = cases[i].x - cases[i].xorig - 0.5f;
        float y0 = cases[i].y - cases[i].yorig - 0.5f;
        glColor3f(0.4f, 0.4f, 0.45f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x0, y0);
        glVertex2f(x0 + F + 1, y0);
        glVertex2f(x0 + F + 1, y0 + F + 1);
        glVertex2f(x0, y0 + F + 1);
        glEnd();

        glColor3f(cases[i].r, cases[i].g, cases[i].b);
        glRasterPos2f(cases[i].x, cases[i].y);
        glBitmap(F, F, cases[i].xorig, cases[i].yorig, 0, 0, glyph);
    }

    /* What the red F should look like where it is now. */
    float rx = cases[MOVABLE].x - cases[MOVABLE].xorig;
    float ry = cases[MOVABLE].y - cases[MOVABLE].yorig;
    const char *red =
        rx >= AB_W || ry >= AB_H || rx + F <= 0 || ry + F <= 0
            ? "outside the window: nothing shows"
        : rx < 0 && ry < 0 ? "crosses the bottom-left corner: A mis-clips it and writes before its buffer"
        : rx < 0 ? "crosses the left edge: A mis-clips it"
        : rx + F > AB_W || ry < 0 || ry + F > AB_H ? "clipped away from the left edge: A and B agree"
        : "inside: A and B agree";
    ab_report("expect: each F clipped in place in its outline; red %s", red);
}
