/* 06-points: point smoothing and point size.
 *
 * Five rows of four points, each row a few lines of plain OpenGL between a
 * "test:" comment and "end test"; build.sh shows those blocks on the page.
 * The points sit at x = 150, 232, 314 and 396, one row every 64 px from
 * y = 296 down.
 *
 * gl4es draws immediate-mode geometry later, in a batch. Every row ends with
 * glFlush, so rows don't share a batch, and the SMOOTH and ALPHA TEST rows
 * flush after each point, so they test smoothing alone, not the glPointSize
 * fix. Each row puts its state back to the default when it's done. */
#include "ab.h"

static GLuint list;

void demo_init(void)
{
}

void demo_draw(double t)
{
    (void)t;
    glViewport(0, 0, AB_W, AB_H);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, AB_W, 0, AB_H, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0.6f, 0.6f, 0.65f);
    ab_label(8, 290, "SMOOTH");
    ab_label(8, 226, "ALPHA TEST");
    ab_label(8, 162, "SIZE RESET");
    ab_label(8, 98, "TWO SIZES");
    ab_label(8, 34, "LIST");

    /* test: SMOOTH
     * Round points, 6 to 40 px wide. Master draws squares. GL antialiases
     * about one pixel at the edge; gl4es fades the outer 15% of the radius,
     * and draws the 40 px one 32 px wide (see GL_POINT_SIZE_MAX): known gaps,
     * in TODO.md. */
    glColor3f(0.4f, 0.8f, 1.0f);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(6);
    glBegin(GL_POINTS); glVertex2f(150, 296); glEnd(); glFlush();
    glPointSize(12);
    glBegin(GL_POINTS); glVertex2f(232, 296); glEnd(); glFlush();
    glPointSize(24);
    glBegin(GL_POINTS); glVertex2f(314, 296); glEnd(); glFlush();
    glPointSize(40);
    glBegin(GL_POINTS); glVertex2f(396, 296); glEnd(); glFlush();
    glDisable(GL_BLEND);
    glDisable(GL_POINT_SMOOTH);
    glPointSize(1);
    /* end test */

    /* test: ALPHA TEST
     * As SMOOTH, with an alpha test. GL applies the coverage to alpha before
     * the test, so the soft edge, where coverage is under 0.5, is cut off:
     * round points with a hard edge. Master draws squares; with the patches
     * gl4es draws round points but applies the coverage after the test, so
     * the soft edge stays, a known gap (a TODO in fpe_shader.c). */
    glColor3f(1.0f, 0.75f, 0.2f);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glPointSize(6);
    glBegin(GL_POINTS); glVertex2f(150, 232); glEnd(); glFlush();
    glPointSize(12);
    glBegin(GL_POINTS); glVertex2f(232, 232); glEnd(); glFlush();
    glPointSize(24);
    glBegin(GL_POINTS); glVertex2f(314, 232); glEnd(); glFlush();
    glPointSize(40);
    glBegin(GL_POINTS); glVertex2f(396, 232); glEnd(); glFlush();
    glDisable(GL_ALPHA_TEST);
    glAlphaFunc(GL_ALWAYS, 0);
    glDisable(GL_BLEND);
    glDisable(GL_POINT_SMOOTH);
    glPointSize(1);
    /* end test */

    /* test: SIZE RESET
     * Four 24 px squares. The glPointSize(1) after them must not change
     * them; master shrinks them to one-pixel dots. */
    glColor3f(0.4f, 0.9f, 0.5f);
    glPointSize(24);
    glBegin(GL_POINTS);
    glVertex2f(150, 168);
    glVertex2f(232, 168);
    glVertex2f(314, 168);
    glVertex2f(396, 168);
    glEnd();
    glPointSize(1);
    glFlush();
    /* end test */

    /* test: TWO SIZES
     * Two 8 px squares, then two 24 px ones. Master draws all four at
     * 24 px. */
    glColor3f(0.9f, 0.5f, 0.9f);
    glPointSize(8);
    glBegin(GL_POINTS);
    glVertex2f(150, 104);
    glVertex2f(232, 104);
    glEnd();
    glPointSize(24);
    glBegin(GL_POINTS);
    glVertex2f(314, 104);
    glVertex2f(396, 104);
    glEnd();
    glFlush();
    glPointSize(1);
    /* end test */

    /* test: LIST
     * The SIZE RESET calls compiled into a display list: four 24 px squares.
     * Master draws one-pixel dots. */
    if (!list) {
        list = glGenLists(1);
        glNewList(list, GL_COMPILE);
        glPointSize(24);
        glBegin(GL_POINTS);
        glVertex2f(150, 40);
        glVertex2f(232, 40);
        glVertex2f(314, 40);
        glVertex2f(396, 40);
        glEnd();
        glPointSize(1);
        glEndList();
    }
    glColor3f(1.0f, 0.45f, 0.4f);
    glCallList(list);
    glFlush();
    glPointSize(1);
    /* end test */

    /* test: GL_POINT_SIZE_MAX
     * Should start at the largest point size the implementation draws; the
     * status line shows both. gl4es starts it at 32, a known gap (in
     * TODO.md, with a fix on a branch). */
    GLfloat size_max = 0, range[2] = { 0, 0 };
    glGetFloatv(GL_POINT_SIZE_MAX, &size_max);
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, range);
    /* end test */

    glColor3f(1, 1, 1);
    ab_report("SMOOTH and ALPHA TEST: round points; "
              "SIZE RESET and LIST: four large squares; TWO SIZES: two small, two large; "
              "GL_POINT_SIZE_MAX %g (largest point size %g)", size_max, range[1]);
}
