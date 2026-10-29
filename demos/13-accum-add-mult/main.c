/* 13-accum-add-mult: glAccum(GL_MULT) and glAccum(GL_ADD).
 *
 * GL_MULT multiplies the accumulation buffer by a value and GL_ADD adds a
 * value to it. Each panel puts a picture or a few passes in the buffer,
 * changes it with one of them, and returns it. Without them, as on master,
 * the buffer comes back unchanged. The scissor box bounds every
 * accumulation operation, so each panel accumulates on its own. Values
 * stay within [-1, 1], the range GL gives the buffer. Each panel is plain
 * OpenGL between a "test:" comment and "end test"; build.sh shows those
 * blocks on the page. */
#include <math.h>

#include "ab.h"

#define PW 240 /* panel size: four panels in a 2 x 2 grid */
#define PH 180

/* Panel 0 top left, 1 top right, 2 bottom left, 3 bottom right: its
 * viewport, and the scissor box that bounds glClear and glAccum to it. */
static void panel(int p)
{
    int x = (p % 2) * PW, y = (p < 2) * PH;
    glViewport(x, y, PW, PH);
    glScissor(x, y, PW, PH);
}

static void ortho(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0f * PW / PH, 1.0f * PW / PH, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void disc(float x, float y, float r)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 32; i++)
        glVertex2f(x + r * cosf(i * 6.2831853f / 32), y + r * sinf(i * 6.2831853f / 32));
    glEnd();
}

/* On black: eight gray bars from black to white, and red, green and blue
 * discs. The bars' edges fall between pixels, not on pixel centers, where
 * implementations may round either way. */
static void card(void)
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    for (int k = 0; k < 8; k++) {
        glColor3f(k / 7.0f, k / 7.0f, k / 7.0f);
        glRectf(-1.2f + 0.3f * k, 0.2f, -0.9f + 0.3f * k, 0.7f);
    }
    glColor3f(0.9f, 0.2f, 0.2f);
    disc(-0.7f, -0.4f, 0.28f);
    glColor3f(0.2f, 0.8f, 0.3f);
    disc(0, -0.4f, 0.28f);
    glColor3f(0.25f, 0.4f, 0.95f);
    disc(0.7f, -0.4f, 0.28f);
}

/* The red channel of a panel's pixel, at a point in ortho()'s coordinates. */
static int red_at(int p, float x, float y)
{
    GLubyte px[4] = {0};
    glReadPixels((p % 2) * PW + (int)((x + 1.0f * PW / PH) * PH / 2),
                 (p < 2) * PH + (int)((y + 1) * PH / 2), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    return px[0];
}

static GLint accum_bits;

void demo_init(void)
{
    glGetIntegerv(GL_ACCUM_RED_BITS, &accum_bits);
}

void demo_draw(double t)
{
    int gray, black, white;
    glEnable(GL_SCISSOR_TEST);

    /* test: TRAIL (MULT 0.6)
     * A ball circling every 3 s, drawn at its last 8 positions, oldest
     * first: before adding each one, glAccum(GL_MULT) fades what's there,
     * so the ball trails a fading tail. Without GL_MULT all 8 balls come
     * back equally bright. The background is black, so the weights don't
     * change it. */
    panel(0);
    ortho();
    glClear(GL_ACCUM_BUFFER_BIT);
    glClearColor(0, 0, 0, 1);
    for (int i = 7; i >= 0; i--) {
        float a = (float)(t - i * 0.05) * 6.2831853f / 3;
        glClear(GL_COLOR_BUFFER_BIT);
        glColor3f(1, 1, 1);
        disc(0.6f * cosf(a), 0.6f * sinf(a), 0.13f);
        glAccum(GL_MULT, 0.6f);
        glAccum(GL_ACCUM, 0.4f);
    }
    glAccum(GL_RETURN, 2.5f);
    /* end test */

    /* test: MULT 2
     * The card loaded at half brightness, then multiplied by 2: it comes
     * back as drawn. Without GL_MULT it comes back at half brightness. The
     * status line reads the gray bar 4/7 (146 of 255). */
    panel(1);
    ortho();
    card();
    glAccum(GL_LOAD, 0.5f);
    glAccum(GL_MULT, 2.0f);
    glAccum(GL_RETURN, 1.0f);
    /* end test */
    gray = red_at(1, 0.15f, 0.45f);

    /* test: ADD 0.2
     * The card loaded at half brightness, 0.2 added, and returned at twice
     * that: everything is 0.4 lighter, the black background gray (102 of
     * 255 in the status line). Without GL_ADD it comes back as drawn. */
    panel(2);
    ortho();
    card();
    glAccum(GL_LOAD, 0.5f);
    glAccum(GL_ADD, 0.2f);
    glAccum(GL_RETURN, 2.0f);
    /* end test */
    black = red_at(2, 0, 0.9f);

    /* test: ADD -0.2
     * The same with -0.2: everything is 0.4 darker, the dark bars black and
     * the white one gray (153 of 255 in the status line). The buffer holds
     * the negative values until GL_RETURN clamps them. Without GL_ADD it
     * comes back as drawn. */
    panel(3);
    ortho();
    card();
    glAccum(GL_LOAD, 0.5f);
    glAccum(GL_ADD, -0.2f);
    glAccum(GL_RETURN, 2.0f);
    /* end test */
    white = red_at(3, 1.05f, 0.45f);

    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, AB_W, AB_H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, AB_W, 0, AB_H, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(0.75f, 0.75f, 0.80f);
    ab_label(8, PH + 8, "TRAIL (MULT 0.6)");
    ab_label(PW + 8, PH + 8, "MULT 2");
    ab_label(8, 8, "ADD 0.2");
    ab_label(PW + 8, 8, "ADD -0.2");
    glColor3f(0.30f, 0.32f, 0.38f);
    glRectf(PW, 0, PW + 1, AB_H);
    glRectf(0, PH, AB_W, PH + 1);
    ab_report("GL_ACCUM_RED_BITS %d; gray after MULT 2: %d (should be 146); black after ADD 0.2: %d "
              "(should be 102); white after ADD -0.2: %d (should be 153)",
              accum_bits, gray, black, white);
}
