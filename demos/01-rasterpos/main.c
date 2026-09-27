/* 01-rasterpos: glRasterPos3f under a perspective projection.
 *
 * Every marked vertex is drawn twice: as a GL_POINTS dot (transformed by the
 * GLES vertex pipeline) and as a glBitmap arrow placed with glRasterPos3f
 * (transformed by gl4es on the CPU). Each arrow tip should touch its dot.
 *   white  8 corners of a spinning cube, always in view
 *   yellow a row of 2D points under glOrtho (w = 1), which master already
 *          gets right; labelled as such, and the labels themselves are
 *          glBitmap text under glOrtho
 *   red    a probe orbiting through, beside and behind the viewer, drawn
 *          right after the yellow row; while it is outside the view volume
 *          the raster position is invalid and its arrow must not be drawn at
 *          all (a stale position would put it on the last yellow point)
 *
 * The arrow extends only up and right of the raster position (origin -4,-4):
 * master's glBitmap mis-clips bitmaps that cross the window's left edge, a
 * separate bug this demo keeps out of the picture. */
#include <math.h>

#include "ab.h"

#define ARROW 12 /* ARROW x ARROW bitmap, tip at (0, 0) pointing down-left */
static GLubyte arrow[ARROW * 2];

static const GLfloat corners[8][3] = {
    {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
    {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1},
};
static const int edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
    {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
};

/* Frustum used below; the demo's own copy for classifying the probe. */
#define FR_X 0.8f
#define FR_Y 0.6f
#define FR_NEAR 1.0f
#define FR_FAR 30.0f

void demo_init(void)
{
    /* Shaft along the diagonal, head along the two axes. Row 0 is the
     * bottom row. */
    for (int y = 0; y < ARROW; y++)
        for (int x = 0; x < ARROW; x++)
            if (x == y || (y == 0 && x < 5) || (x == 0 && y < 5))
                arrow[y * 2 + x / 8] |= 0x80 >> (x % 8);
}

static void mark(const GLfloat *p)
{
    glRasterPos3fv(p);
    /* Tip at (+4, +4): the corner of the 7x7 dot. */
    glBitmap(ARROW, ARROW, -4, -4, 0, 0, arrow);
}

void demo_draw(double t)
{
    /* Both loop every 12 s. */
    float a = (float)fmod(t * 30.0, 360.0);               /* cube spin, degrees */
    float b = (float)fmod(t * (2 * M_PI / 12), 2 * M_PI); /* probe orbit, radians */
    GLfloat probe[3] = { 3.0f * sinf(b), -0.6f, -2.0f - 3.0f * cosf(b) };

    glViewport(0, 0, AB_W, AB_H);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPointSize(7.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-FR_X, FR_X, -FR_Y, FR_Y, FR_NEAR, FR_FAR);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Cube. */
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -5.0f);
    glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(a, 0.0f, 1.0f, 0.0f);

    glColor3f(0.35f, 0.35f, 0.4f);
    glBegin(GL_LINES);
    for (int i = 0; i < 12; i++) {
        glVertex3fv(corners[edges[i][0]]);
        glVertex3fv(corners[edges[i][1]]);
    }
    glEnd();

    glColor3f(0.3f, 0.9f, 0.4f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 8; i++)
        glVertex3fv(corners[i]);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < 8; i++)
        mark(corners[i]);
    glPopMatrix();

    glColor3f(0.6f, 0.6f, 0.65f);
    ab_label(10, 10, "3D PERSPECTIVE (GLFRUSTUM)");
    glColor3f(1.0f, 0.85f, 0.2f);
    ab_label(10, AB_H - 24, "2D ORTHO: CORRECT IN A AND B");

    /* 2D points in window coordinates. */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, AB_W, 0, AB_H, -1, 1);
    glColor3f(1.0f, 0.85f, 0.2f);
    for (int i = 0; i < 5; i++) {
        GLfloat p[3] = { 30.0f + 50.0f * i, AB_H - 50.0f, 0.0f };
        glBegin(GL_POINTS);
        glVertex3fv(p);
        glEnd();
        mark(p);
    }
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    /* Probe last, right after the yellow row: a stale raster position is the
     * last yellow point's, and nothing drawn later covers it. */
    glColor3f(1.0f, 0.25f, 0.2f);
    glBegin(GL_POINTS);
    glVertex3fv(probe);
    glEnd();
    mark(probe);

    /* Where the probe is, by the demo's own arithmetic (eye space = world
     * here), so the reader knows whether a red arrow should be visible. */
    float ez = -probe[2];
    const char *where = ez <= 0.0f ? "behind the viewer: no red arrow"
        : ez < FR_NEAR || ez > FR_FAR || fabsf(probe[0] / ez * FR_NEAR) > FR_X
            || fabsf(probe[1] / ez * FR_NEAR) > FR_Y
        ? "outside the view: no red arrow"
        : "in view: red arrow at the red dot";
    ab_report("t=%.2f · expect: probe %s", t, where);
}
