/* bitmap-left-clip: glBitmap bitmaps that cross the window's left edge.
 *
 * The same 32x32 "F" is drawn four times with glBitmap. A grey outline,
 * drawn with GL_LINE_LOOP, marks the full box each bitmap would cover.
 * Correct output shows the part of each F inside the window, in place
 * within its outline:
 *   green   fully inside (control)
 *   cyan    crossing the left edge: the right 20 columns show
 *   yellow  crossing the bottom-left corner: the top-right 20x20 shows
 *   red     entirely left of the window: nothing shows
 * Raster positions are set under glOrtho (w = 1) and are never negative, so
 * only glBitmap's clipping is exercised, not glRasterPos. */
#include "ab.h"

#define F 32
static GLubyte glyph[F * F / 8];

static const struct {
    GLfloat x, y, xorig, yorig;
    GLfloat r, g, b;
} cases[] = {
    { 200, 160,  0,  0, 0.3f, 0.9f, 0.4f },  /* inside */
    {   8, 200, 20,  0, 0.2f, 0.8f, 0.9f },  /* crosses x = 0 */
    {   8,   8, 20, 20, 1.0f, 0.85f, 0.2f }, /* crosses x = 0 and y = 0 */
    {   8, 100, 60,  0, 1.0f, 0.25f, 0.2f }, /* entirely left of x = 0 */
};
#define NCASES (int)(sizeof cases / sizeof cases[0])

void demo_init(void)
{
    /* An "F", 6 px strokes: stem on the left, bars at the top and middle.
     * Row 0 is the bottom row. Asymmetric, so a shifted copy is obvious. */
    for (int y = 0; y < F; y++)
        for (int x = 0; x < F; x++)
            if (x < 6 || y >= F - 6 || (y >= 13 && y < 19 && x < 24))
                glyph[y * (F / 8) + x / 8] |= 0x80 >> (x % 8);
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

    ab_report("expect: each F clipped in place inside its outline; no red");
}
