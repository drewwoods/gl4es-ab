/* 08-polygon-offset-line: GL_POLYGON_OFFSET_LINE for polygon-mode outlines.
 *
 * Each row draws a grid of quads filled, then draws the same quads again as
 * polygon-mode lines over them, with the depth test on. The grid slopes in
 * depth, so the lines and the faces under them have nearly the same depth:
 * without an offset the lines z-fight with their faces and most of them
 * are hidden. Each row is plain OpenGL between a "test:" comment and
 * "end test"; build.sh shows those blocks on the page. */
#include "ab.h"

/* test: THE GRID
 * Every row draws this: a 10 x 2 grid of 20 px quads, its depth rising from
 * -0.5 at the left to 0.5 at the right, filled and then as lines. Rows start
 * at half-pixel coordinates so 1-pixel lines fall on pixel centers, where
 * every implementation puts them in the same row. */
static void grid(float x, float y)
{
    glBegin(GL_QUADS);
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 10; i++) {
            float x0 = x + i * 20, x1 = x0 + 20, y0 = y + j * 20, y1 = y0 + 20;
            float z0 = -0.5f + i * 0.1f, z1 = z0 + 0.1f;
            glVertex3f(x0, y0, z0);
            glVertex3f(x1, y0, z1);
            glVertex3f(x1, y1, z1);
            glVertex3f(x0, y1, z0);
        }
    glEnd();
}

static void faces(float x, float y)
{
    glColor3f(0.20f, 0.24f, 0.32f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    grid(x, y);
}
/* end test */

void demo_init(void)
{
}

void demo_draw(double t)
{
    (void)t;
    glViewport(0, 0, AB_W, AB_H);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, AB_W, 0, AB_H, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glColor3f(0.6f, 0.6f, 0.65f);
    ab_label(8, 300, "NO OFFSET");
    ab_label(8, 220, "OFFSET LINE");
    ab_label(8, 140, "PUSH / POP");
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    /* test: NO OFFSET
     * The control: outlines at their faces' depth, no offset. They z-fight
     * with the faces and most of them are hidden, on every implementation. */
    faces(160.5f, 286.5f);
    glColor3f(0.4f, 0.8f, 1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    grid(160.5f, 286.5f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* end test */

    /* test: OFFSET LINE
     * The same, with GL_POLYGON_OFFSET_LINE pulling the outlines toward the
     * eye: the whole grid shows. Master rejects the enable (GL_INVALID_ENUM)
     * and most lines stay hidden. Only units are used: gl4es doesn't apply the factor,
     * the slope part, to lines. */
    faces(160.5f, 206.5f);
    glColor3f(1.0f, 0.75f, 0.2f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(0, -32);
    grid(160.5f, 206.5f);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(0, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* end test */

    /* test: PUSH / POP
     * Offset set inside glPushAttrib(GL_POLYGON_BIT) is undone by the pop:
     * these outlines have no offset and are mostly hidden, and the status line
     * reads the enable and units back. Master keeps the units after the
     * pop, and can't answer for the enable. */
    glPolygonOffset(0, 0);
    glPushAttrib(GL_POLYGON_BIT);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(0, -32);
    glPopAttrib();
    faces(160.5f, 126.5f);
    glColor3f(0.4f, 0.9f, 0.5f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    grid(160.5f, 126.5f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    GLboolean enabled = glIsEnabled(GL_POLYGON_OFFSET_LINE);
    GLfloat units = 99;
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &units);
    /* end test */

    glDisable(GL_DEPTH_TEST);
    glColor3f(1, 1, 1);
    ab_report("OFFSET LINE: the whole grid; NO OFFSET and PUSH / POP: mostly hidden; "
              "after the pop GL_POLYGON_OFFSET_LINE is %s, units %g (should be off, 0)",
              enabled ? "on" : "off", units);
}
