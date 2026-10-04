/* 06-points: point smoothing and point size.
 *
 * Five rows of four points, each row a few lines of plain OpenGL; README.md
 * lists them with what each should look like. The points sit at
 * x = 150, 232, 314 and 396, one row every 64 px from y = 296 down.
 *
 * gl4es draws immediate-mode geometry later, in a batch. Every row ends with
 * glFlush, so rows don't share a batch, and the SMOOTH and ALPHA TEST rows
 * flush after each point, so they test smoothing alone, not the glPointSize
 * fix. Each row puts its state back to the default when it's done. */
#include "ab.h"

static GLuint list;

void demo_init(void)
{
    /* LIST: the SIZE RESET calls, compiled. */
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

    /* SMOOTH: round points with a one-pixel soft edge. 40 is above 32, the
     * GL_POINT_SIZE_MAX master starts with. */
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

    /* ALPHA TEST: as SMOOTH, with an alpha test. GL applies the coverage
     * to alpha before the test, so the soft edge (coverage under 0.5) is
     * cut off. */
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

    /* SIZE RESET: four 24 px squares; the glPointSize(1) after them must
     * not shrink them. */
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

    /* TWO SIZES: two 8 px squares, then two 24 px ones. */
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

    /* LIST: four 24 px squares, from the list compiled in demo_init. */
    glColor3f(1.0f, 0.45f, 0.4f);
    glCallList(list);
    glFlush();
    glPointSize(1);

    /* GL_POINT_SIZE_MAX should start at the largest point size. */
    GLfloat size_max = 0, range[2] = { 0, 0 };
    glGetFloatv(GL_POINT_SIZE_MAX, &size_max);
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, range);

    glColor3f(1, 1, 1);
    ab_report("SMOOTH: round, soft-edged points; ALPHA TEST: round, hard-edged; "
              "SIZE RESET and LIST: four large squares; TWO SIZES: two small, two large; "
              "GL_POINT_SIZE_MAX %g (largest point size %g)", size_max, range[1]);
}
