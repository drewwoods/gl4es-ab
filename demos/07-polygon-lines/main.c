/* 07-polygon-lines: glPolygonMode(GL_LINE) outlines and glEdgeFlag.
 *
 * Four rows of shapes drawn as polygon-mode lines, each a few lines of plain
 * OpenGL between a "test:" comment and "end test"; build.sh shows those
 * blocks on the page. Below them, OBJECTS draws many small wireframe grids,
 * each its own draw, for the frame time. */
#include "ab.h"

static GLfloat objects = 240;

void demo_init(void)
{
    ab_param("objects", &objects, 0, 600, 20);
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
    ab_label(8, 329, "ONE QUAD");
    ab_label(8, 289, "QUAD STRIP");
    ab_label(8, 249, "TRIANGLES");
    ab_label(8, 209, "POLYGON");
    ab_label(8, 169, "FLAG OFF");
    ab_label(8, 129, "LIST FLAG");
    ab_label(8, 89, "LIST POP");
    ab_label(8, 34, "OBJECTS");

    /* test: ONE QUAD
     * One quad on its own: its four sides. Master also draws the diagonal. */
    glColor3f(0.4f, 0.8f, 1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_QUADS);
    glVertex2f(150.5f, 322.5f);
    glVertex2f(250.5f, 322.5f);
    glVertex2f(250.5f, 348.5f);
    glVertex2f(150.5f, 348.5f);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* end test */

    /* test: QUAD STRIP
     * Two quads in a strip: two boxes side by side, no diagonals. Master
     * draws each quad's diagonal. */
    glColor3f(1.0f, 0.75f, 0.2f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_QUAD_STRIP);
    glVertex2f(150.5f, 282.5f); glVertex2f(150.5f, 308.5f);
    glVertex2f(250.5f, 282.5f); glVertex2f(250.5f, 308.5f);
    glVertex2f(350.5f, 282.5f); glVertex2f(350.5f, 308.5f);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* end test */

    /* test: TRIANGLES
     * A square from two triangles, with edge flags hiding the shared
     * diagonal: an edge is drawn when the flag of the vertex it starts at is
     * GL_TRUE. Just the square's four sides. Master draws the diagonal. */
    glColor3f(0.4f, 0.9f, 0.5f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_TRIANGLES);
    glEdgeFlag(GL_TRUE);  glVertex2f(150.5f, 242.5f);
    glEdgeFlag(GL_TRUE);  glVertex2f(250.5f, 242.5f);
    glEdgeFlag(GL_FALSE); glVertex2f(250.5f, 268.5f);   /* hides 250,268 -> 150,242 */
    glEdgeFlag(GL_FALSE); glVertex2f(150.5f, 242.5f);   /* hides 150,242 -> 250,268 */
    glEdgeFlag(GL_TRUE);  glVertex2f(250.5f, 268.5f);
    glEdgeFlag(GL_TRUE);  glVertex2f(150.5f, 268.5f);
    glEnd();
    glEdgeFlag(GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* end test */

    /* test: POLYGON
     * A pentagon with its top-right side hidden by an edge flag: four
     * sides. Master draws all five. */
    glColor3f(0.9f, 0.5f, 0.9f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_POLYGON);
    glVertex2f(160.5f, 202.5f);
    glVertex2f(240.5f, 202.5f);
    glEdgeFlag(GL_FALSE);
    glVertex2f(260.5f, 218.5f);                         /* hides 260,218 -> 200,228 */
    glEdgeFlag(GL_TRUE);
    glVertex2f(200.5f, 228.5f);
    glVertex2f(140.5f, 218.5f);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* end test */

    /* test: FLAG OFF
     * The quad strip again, with the edge flag off: GL ignores edge flags
     * for strips, so two boxes. Before the quad-strip commit B drew nothing;
     * master draws the diagonals. */
    glColor3f(1.0f, 0.55f, 0.35f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEdgeFlag(GL_FALSE);
    glBegin(GL_QUAD_STRIP);
    glVertex2f(150.5f, 162.5f); glVertex2f(150.5f, 188.5f);
    glVertex2f(250.5f, 162.5f); glVertex2f(250.5f, 188.5f);
    glVertex2f(350.5f, 162.5f); glVertex2f(350.5f, 188.5f);
    glEnd();
    glEdgeFlag(GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* end test */

    /* test: LIST FLAG
     * Two display lists with a quad and no glEdgeFlag: the quad takes the
     * flag current when the list is called, not when it was compiled. Left,
     * compiled with the flag off and called with it on: four sides. Right,
     * compiled on and called off: nothing. gl4es keeps the flag from compile
     * time (a TODO), so B draws the right one only. Master draws both, with
     * diagonals. */
    static GLuint compiled_off, compiled_on;
    if (!compiled_off) {
        compiled_off = glGenLists(2);
        compiled_on = compiled_off + 1;
        glEdgeFlag(GL_FALSE);
        glNewList(compiled_off, GL_COMPILE);
        glBegin(GL_QUADS);
        glVertex2f(150.5f, 122.5f); glVertex2f(230.5f, 122.5f);
        glVertex2f(230.5f, 148.5f); glVertex2f(150.5f, 148.5f);
        glEnd();
        glEndList();
        glEdgeFlag(GL_TRUE);
        glNewList(compiled_on, GL_COMPILE);
        glBegin(GL_QUADS);
        glVertex2f(270.5f, 122.5f); glVertex2f(350.5f, 122.5f);
        glVertex2f(350.5f, 148.5f); glVertex2f(270.5f, 148.5f);
        glEnd();
        glEndList();
    }
    glColor3f(0.95f, 0.85f, 0.4f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEdgeFlag(GL_TRUE);
    glCallList(compiled_off);
    glEdgeFlag(GL_FALSE);
    glCallList(compiled_on);
    glEdgeFlag(GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* end test */

    /* test: LIST POP
     * A display list that hides a quad's edges inside glPushAttrib and
     * glPopAttrib: nothing on the left. After the list the flag is back to
     * GL_TRUE, so the quad drawn next, on the right, has four sides, and
     * glGetBooleanv(GL_EDGE_FLAG) is GL_TRUE (status line). gl4es keeps the
     * list's last flag instead (a TODO), so B draws neither. Master draws
     * both, with diagonals. */
    static GLuint pop_list;
    if (!pop_list) {
        pop_list = glGenLists(1);
        glNewList(pop_list, GL_COMPILE);
        glPushAttrib(GL_CURRENT_BIT);
        glEdgeFlag(GL_FALSE);
        glBegin(GL_QUADS);
        glVertex2f(150.5f, 82.5f); glVertex2f(230.5f, 82.5f);
        glVertex2f(230.5f, 108.5f); glVertex2f(150.5f, 108.5f);
        glEnd();
        glPopAttrib();
        glEndList();
    }
    glColor3f(0.5f, 0.85f, 0.95f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glCallList(pop_list);
    glBegin(GL_QUADS);
    glVertex2f(270.5f, 82.5f); glVertex2f(350.5f, 82.5f);
    glVertex2f(350.5f, 108.5f); glVertex2f(270.5f, 108.5f);
    glEnd();
    GLboolean after_pop = 2;
    glGetBooleanv(GL_EDGE_FLAG, &after_pop);
    glEdgeFlag(GL_TRUE);   /* so a wrong flag doesn't carry into the next frame */
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* end test */

    /* test: GL_EDGE_FLAG
     * glGetBooleanv(GL_EDGE_FLAG) returns the flag just set; the status line
     * shows it. Master leaves it unwritten. */
    GLboolean edge = 2;
    glEdgeFlag(GL_FALSE);
    glGetBooleanv(GL_EDGE_FLAG, &edge);
    glEdgeFlag(GL_TRUE);
    /* end test */

    /* test: OBJECTS
     * Many small wireframe grids, each placed with its own matrix, so each
     * is a separate draw as in a scene: 4 x 4 quads of 3 x 3 px per object.
     * The objects slider sets how many; compare the frame times. */
    glColor3f(0.5f, 0.55f, 0.6f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (int i = 0; i < (int)objects; i++) {
        glPushMatrix();
        glTranslatef(150.5f + (i % 20) * 15, 14.5f + (i / 20 % 3) * 15, 0);
        for (int row = 0; row < 4; row++) {
            glBegin(GL_QUAD_STRIP);
            for (int col = 0; col <= 4; col++) {
                glVertex2f(col * 3, row * 3);
                glVertex2f(col * 3, row * 3 + 3);
            }
            glEnd();
        }
        glPopMatrix();
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* end test */

    glColor3f(1, 1, 1);
    ab_report("ONE QUAD and TRIANGLES: a box; QUAD STRIP and FLAG OFF: two boxes; "
              "POLYGON: four sides; LIST FLAG: the left box; LIST POP: the right box; "
              "glGetBooleanv(GL_EDGE_FLAG): %s, after LIST POP: %s; %d objects",
              edge == GL_FALSE ? "correct" : edge == 2 ? "left unwritten" : "wrong",
              after_pop == GL_TRUE ? "correct" : after_pop == 2 ? "left unwritten" : "wrong",
              (int)objects);
}
