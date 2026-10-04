/* 06-points: GL_POINT_SMOOTH ignored, and glPointSize applied too late.
 *
 * One row per case, each drawing four points left to right.
 *
 *   row         what it does                              correct
 *   SMOOTH      GL_POINT_SMOOTH and blending, sizes 6,    round points with a
 *               12, 24 and 40                             soft edge
 *   ALPHA TEST  as SMOOTH, with glAlphaFunc(GL_GREATER,   round points, the
 *               0.5)                                      soft edge cut off
 *   SIZE RESET  glPointSize(24), four points,             four 24 px squares
 *               glPointSize(1)
 *   TWO SIZES   glPointSize(8), two points,               two small squares,
 *               glPointSize(24), two points                two large ones
 *   LIST        the SIZE RESET calls compiled into a      four 24 px squares
 *               display list, then glCallList
 *
 * The status line reports GL_POINT_SIZE_MAX, which should start at the
 * largest point size the implementation draws (GL_ALIASED_POINT_SIZE_RANGE).
 *
 * gl4es draws immediate-mode geometry later, in a batch, and master reads
 * the point size when the batch is drawn: the last size set before then
 * wins for every point in it. Every row ends with glFlush, so rows don't
 * share a batch, and the SMOOTH and ALPHA TEST rows also flush before each
 * size change, so they don't depend on that fix.
 *
 * Each row puts its state back to the default when it's done. */
#include "ab.h"

#define ROWS 5
#define X0 150
#define DX 82

static GLuint list;

static int row_y(int row) { return AB_H - 70 - row * 64; }

static void points(int row, int first, int n, int x_step)
{
    glBegin(GL_POINTS);
    for (int i = first; i < first + n; i++)
        glVertex2f(X0 + i * x_step, row_y(row) + 6);
    glEnd();
}

void demo_init(void)
{
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glPointSize(24);
    points(4, 0, 4, DX);
    glPointSize(1);
    glEndList();
}

static void smooth_row(int row)
{
    /* 40 is above 32, the GL_POINT_SIZE_MAX master starts with. */
    static const GLfloat sizes[4] = { 6, 12, 24, 40 };
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < 4; i++) {
        glFlush();
        glPointSize(sizes[i]);
        points(row, i, 1, DX);
    }
    glFlush();
    glPointSize(1);
    glDisable(GL_BLEND);
    glDisable(GL_POINT_SMOOTH);
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
    static const char *names[ROWS] = {
        "SMOOTH", "ALPHA TEST", "SIZE RESET", "TWO SIZES", "LIST"
    };
    for (int r = 0; r < ROWS; r++)
        ab_label(8, row_y(r), names[r]);

    /* SMOOTH */
    glColor3f(0.4f, 0.8f, 1.0f);
    smooth_row(0);

    /* ALPHA TEST: GL applies the point's coverage to alpha before the alpha
     * test, so the soft edge (coverage under 0.5) is discarded. */
    glColor3f(1.0f, 0.75f, 0.2f);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    smooth_row(1);
    glDisable(GL_ALPHA_TEST);
    glAlphaFunc(GL_ALWAYS, 0);

    /* SIZE RESET: the trailing glPointSize(1) must not shrink these. */
    glColor3f(0.4f, 0.9f, 0.5f);
    glPointSize(24);
    points(2, 0, 4, DX);
    glPointSize(1);
    glFlush();

    /* TWO SIZES: two batches of points, each with its own size. */
    glColor3f(0.9f, 0.5f, 0.9f);
    glPointSize(8);
    points(3, 0, 2, DX);
    glPointSize(24);
    points(3, 2, 2, DX);
    glFlush();
    glPointSize(1);

    /* LIST: the SIZE RESET calls, compiled. */
    glColor3f(1.0f, 0.45f, 0.4f);
    glCallList(list);
    glFlush();
    glPointSize(1);

    GLfloat size_max = 0, range[2] = { 0, 0 };
    glGetFloatv(GL_POINT_SIZE_MAX, &size_max);
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, range);

    glColor3f(1, 1, 1);
    ab_report("SMOOTH: round, soft-edged points; ALPHA TEST: round, hard-edged; "
              "SIZE RESET and LIST: four large squares; TWO SIZES: two small, two large; "
              "GL_POINT_SIZE_MAX %g (largest point size %g)", size_max, range[1]);
}
