/* 09-line-width: glLineWidth > 1 where the driver only draws 1-pixel lines.
 *
 * WebGL (ANGLE) reports GL_ALIASED_LINE_WIDTH_RANGE [1, 1]: every line is
 * one pixel wide whatever glLineWidth says. Each row is plain OpenGL between
 * a "test:" comment and "end test"; build.sh shows those blocks on the
 * page. Natively the drivers draw wide lines themselves, so there A and B
 * should already look alike.
 *
 * Odd-width lines are centered on pixel centers (x.5) and even-width ones on
 * pixel edges, so their edges never fall exactly on a pixel center, where
 * implementations may round either way. */
#include "ab.h"

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
    ab_label(8, 300, "LINES");
    ab_label(8, 220, "LINE STRIP");
    ab_label(8, 150, "LINE LOOP");
    ab_label(8, 80, "POLYGON MODE");

    /* test: LINES
     * Widths 1, 3, 6 and 10, each as a horizontal and a 45-degree segment.
     * On WebGL master draws them all one pixel wide. */
    glColor3f(0.4f, 0.8f, 1.0f);
    glLineWidth(1);
    glBegin(GL_LINES);
    glVertex2f(170.5f, 330.5f); glVertex2f(200.5f, 330.5f);
    glVertex2f(170.5f, 290.5f); glVertex2f(200.5f, 320.5f);
    glEnd();
    glLineWidth(3);
    glBegin(GL_LINES);
    glVertex2f(240.5f, 330.5f); glVertex2f(270.5f, 330.5f);
    glVertex2f(240.5f, 290.5f); glVertex2f(270.5f, 320.5f);
    glEnd();
    glLineWidth(6);
    glBegin(GL_LINES);
    glVertex2f(310, 330); glVertex2f(340, 330);
    glVertex2f(310, 290); glVertex2f(340, 320);
    glEnd();
    glLineWidth(10);
    glBegin(GL_LINES);
    glVertex2f(380, 330); glVertex2f(410, 330);
    glVertex2f(380, 290); glVertex2f(410, 320);
    glEnd();
    glLineWidth(1);
    /* end test */

    /* test: LINE STRIP
     * A zigzag at width 5. */
    glColor3f(1.0f, 0.75f, 0.2f);
    glLineWidth(5);
    glBegin(GL_LINE_STRIP);
    glVertex2f(170.5f, 210.5f);
    glVertex2f(220.5f, 250.5f);
    glVertex2f(270.5f, 210.5f);
    glVertex2f(320.5f, 250.5f);
    glVertex2f(370.5f, 210.5f);
    glEnd();
    glLineWidth(1);
    /* end test */

    /* test: LINE LOOP
     * A square at width 4, the loop closed. */
    glColor3f(0.4f, 0.9f, 0.5f);
    glLineWidth(4);
    glBegin(GL_LINE_LOOP);
    glVertex2f(170, 130);
    glVertex2f(230, 130);
    glVertex2f(230, 180);
    glVertex2f(170, 180);
    glEnd();
    glLineWidth(1);
    /* end test */

    /* test: POLYGON MODE
     * Two quads drawn as polygon-mode lines at width 3: two boxes. */
    glColor3f(0.9f, 0.5f, 0.9f);
    glLineWidth(3);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_QUADS);
    glVertex2f(170.5f, 60.5f); glVertex2f(250.5f, 60.5f); glVertex2f(250.5f, 100.5f); glVertex2f(170.5f, 100.5f);
    glVertex2f(290.5f, 60.5f); glVertex2f(370.5f, 60.5f); glVertex2f(370.5f, 100.5f); glVertex2f(290.5f, 100.5f);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1);
    /* end test */

    /* test: RANGE
     * glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE); the status line shows it.
     * Where gl4es emulates wide lines it reports [1, 64]. */
    GLfloat range[2] = { 0, 0 };
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, range);
    /* end test */

    glColor3f(1, 1, 1);
    ab_report("LINES: widths 1, 3, 6, 10; LINE STRIP: 5; LINE LOOP: 4; POLYGON MODE: 3; "
              "GL_ALIASED_LINE_WIDTH_RANGE [%g, %g]", range[0], range[1]);
}
