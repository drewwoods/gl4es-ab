/* 02-bitmap-dirty-clear: many short glBitmap batches in one frame.
 *
 * A grid of labels, each drawn right after a small quad. The quad is a
 * draw call, so it flushes gl4es's pending glBitmap batch: every label
 * starts a batch of its own, as text between geometry does in a HUD or an
 * immediate-mode UI. On master each new batch clears gl4es's whole
 * viewport-sized bitmap buffer to draw a label of a few hundred pixels;
 * with the patch only the last batch's dirty rectangle is cleared. The
 * picture is the same in A and B: compare the frame times.
 *
 * The buffer is the size of the viewport, so the cost on A grows with the
 * window. "window" sets the viewport to one of the sizes below; the canvas
 * stays 480x360 and shows the viewport's bottom-left corner, where all the
 * drawing is.
 *
 * batches (1..200) sets the number of labels. */
#include <math.h>

#include "ab.h"

static const struct { int w, h; } sizes[] = {
    { AB_W, AB_H }, { 1280, 720 }, { 1920, 1080 }, { 2560, 1440 },
};
static GLfloat batches = 60;
static GLfloat window = 2;

void demo_init(void)
{
    ab_param("batches", &batches, 1, 200, 1);
    ab_param("window", &window, 0, 3, 1);
}

void demo_draw(double t)
{
    int s = (int)window;
    int vw = sizes[s].w, vh = sizes[s].h;
    glViewport(0, 0, vw, vh);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    /* One unit per viewport pixel, so (x, y) below are canvas pixels. */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, vw, 0, vh, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* 6 columns of 20 rows; past 120 labels the grid starts over. */
    int n = (int)batches;
    for (int i = 0; i < n; i++) {
        int col = i % 6, row = (i / 6) % 20;
        int x = 8 + col * 78, y = AB_H - 40 - row * 16;
        float phase = (float)(t * 2.0 + i * 0.4);
        glColor3f(0.5f + 0.5f * sinf(phase), 0.6f, 0.5f + 0.5f * cosf(phase));
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + 10, y);
        glVertex2f(x + 10, y + 10);
        glVertex2f(x, y + 10);
        glEnd();
        glColor3f(0.9f, 0.9f, 0.9f);
        ab_label_vp(vw, vh, x + 16, y, "TEXT");
    }
    ab_report("%d glBitmap batches, %dx%d viewport: A clears %.1f MB per batch; "
              "the frame is the same in A and B, compare the frame times",
              n, vw, vh, vw * vh * 4 / 1e6);
}
