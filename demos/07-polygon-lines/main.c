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
    ab_label(8, 290, "ONE QUAD");
    ab_label(8, 226, "QUAD STRIP");
    ab_label(8, 162, "TRIANGLES");
    ab_label(8, 98, "POLYGON");
    ab_label(8, 34, "OBJECTS");

    /* test: ONE QUAD
     * One quad on its own: its four sides. Master also draws the diagonal. */
    glColor3f(0.4f, 0.8f, 1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_QUADS);
    glVertex2f(150.5f, 270.5f);
    glVertex2f(250.5f, 270.5f);
    glVertex2f(250.5f, 314.5f);
    glVertex2f(150.5f, 314.5f);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* end test */

    /* test: QUAD STRIP
     * Two quads in a strip: two boxes side by side, no diagonals. Master
     * draws each quad's diagonal. */
    glColor3f(1.0f, 0.75f, 0.2f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_QUAD_STRIP);
    glVertex2f(150.5f, 206.5f); glVertex2f(150.5f, 250.5f);
    glVertex2f(250.5f, 206.5f); glVertex2f(250.5f, 250.5f);
    glVertex2f(350.5f, 206.5f); glVertex2f(350.5f, 250.5f);
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
    glEdgeFlag(GL_TRUE);  glVertex2f(150.5f, 142.5f);
    glEdgeFlag(GL_TRUE);  glVertex2f(250.5f, 142.5f);
    glEdgeFlag(GL_FALSE); glVertex2f(250.5f, 186.5f);   /* hides 250,186 -> 150,142 */
    glEdgeFlag(GL_FALSE); glVertex2f(150.5f, 142.5f);   /* hides 150,142 -> 250,186 */
    glEdgeFlag(GL_TRUE);  glVertex2f(250.5f, 186.5f);
    glEdgeFlag(GL_TRUE);  glVertex2f(150.5f, 186.5f);
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
    glVertex2f(160.5f, 78.5f);
    glVertex2f(240.5f, 78.5f);
    glEdgeFlag(GL_FALSE);
    glVertex2f(260.5f, 104.5f);                         /* hides 260,104 -> 200,122 */
    glEdgeFlag(GL_TRUE);
    glVertex2f(200.5f, 122.5f);
    glVertex2f(140.5f, 104.5f);
    glEnd();
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
    ab_report("ONE QUAD and TRIANGLES: a box; QUAD STRIP: two boxes; POLYGON: four sides; "
              "glGetBooleanv(GL_EDGE_FLAG): %s; %d objects",
              edge == GL_FALSE ? "correct" : edge == 2 ? "left unwritten" : "wrong",
              (int)objects);
}
