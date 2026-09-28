/* 04-pushattrib: state set inside glPushAttrib/glPopAttrib leaking out.
 *
 * One row per attribute group, drawing the same thing three times:
 *   BEFORE  with the state set before glPushAttrib(<group>)
 *   INSIDE  after changing that state inside the push
 *   AFTER   after glPopAttrib
 * The pop should put the state back as it was before the push, so AFTER
 * should always look like BEFORE. Where gl4es doesn't save the group, the
 * state leaks and AFTER looks like INSIDE instead.
 *
 *   row      BEFORE / AFTER (correct)       INSIDE
 *   POLYGON  green filled triangle          glFrontFace(GL_CW) and
 *            (culling keeps the CCW one)    glPolygonMode(GL_LINE): outlines
 *                                           (gl4es doesn't cull polygon-mode
 *                                           lines)
 *   STIPPLE  solid lines                    glEnable(GL_LINE_STIPPLE),
 *                                           glLineStipple(4, 0x0F0F): dashed
 *   POINTS   five points of the same size   GL_POINT_DISTANCE_ATTENUATION:
 *                                           points shrink left to right
 *   CLIP     bottom half                    glClipPlane keeping the top half
 *   TEXENV   red checkerboard (GL_MODULATE) GL_REPLACE: grey, colour ignored
 *
 * Each row puts its state back to the default when it's done, so rows
 * don't affect each other, and every frame starts from the defaults. */
/* glPointParameterfv is GL 1.4: Mesa's GL/gl.h declares it only with this
 * (gl4es's GL/gl.h, force-included in the web build, defines it already). */
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include "ab.h"

#define ROWS 5
#define BOX_W 100
#define BOX_H 48
#define X_BEFORE 110
#define X_INSIDE 232
#define X_AFTER 354

static GLuint checker;

void demo_init(void)
{
    /* 4x4 white and grey checkerboard, so GL_MODULATE vs GL_REPLACE shows. */
    GLubyte px[4 * 4 * 3];
    for (int i = 0; i < 16; i++) {
        GLubyte v = ((i % 4) + (i / 4)) % 2 ? 255 : 120;
        px[i * 3] = px[i * 3 + 1] = px[i * 3 + 2] = v;
    }
    glGenTextures(1, &checker);
    glBindTexture(GL_TEXTURE_2D, checker);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static int row_y(int row) { return AB_H - 84 - row * 62; }

/* GL_POLYGON_BIT: a CCW (green) and a CW (red) triangle; culling GL_BACK
 * shows the green one with the default glFrontFace(GL_CCW). */
static void polygon_shape(int x, int y)
{
    glBegin(GL_TRIANGLES);
    glColor3f(0.3f, 0.9f, 0.4f);            /* CCW, lower left */
    glVertex2f(x, y);
    glVertex2f(x + BOX_W, y);
    glVertex2f(x, y + BOX_H);
    glColor3f(1.0f, 0.3f, 0.25f);           /* CW, upper right */
    glVertex2f(x + BOX_W, y);
    glVertex2f(x, y + BOX_H);
    glVertex2f(x + BOX_W, y + BOX_H);
    glEnd();
}

static void lines_shape(int x, int y)
{
    glColor3f(0.9f, 0.9f, 0.9f);
    glBegin(GL_LINES);
    for (int i = 0; i < 4; i++) {
        glVertex2f(x, y + 6 + i * 12);
        glVertex2f(x + BOX_W, y + 6 + i * 12);
    }
    glEnd();
}

/* Five points 20 px apart. Attenuation uses the eye-space distance, which
 * under this glOrtho is in pixels, so the projection is shifted to put the
 * eye at the first point: the points are 0, 20, 40, 60 and 80 px from it. */
static void points_shape(int x, int y)
{
    float ox = x + 10, oy = y + BOX_H / 2;
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-ox, AB_W - ox, -oy, AB_H - oy, -1, 10);
    glMatrixMode(GL_MODELVIEW);
    glColor3f(0.4f, 0.7f, 1.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 5; i++)
        glVertex2f(i * 20, 0);
    glEnd();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void rect_shape(int x, int y)
{
    glColor3f(1.0f, 0.75f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + BOX_W, y);
    glVertex2f(x + BOX_W, y + BOX_H);
    glVertex2f(x, y + BOX_H);
    glEnd();
}

static void textured_shape(int x, int y)
{
    glColor3f(1.0f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(x, y);
    glTexCoord2f(2, 0); glVertex2f(x + BOX_W, y);
    glTexCoord2f(2, 1); glVertex2f(x + BOX_W, y + BOX_H);
    glTexCoord2f(0, 1); glVertex2f(x, y + BOX_H);
    glEnd();
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
    glOrtho(0, AB_W, 0, AB_H, -1, 10);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0.6f, 0.6f, 0.65f);
    ab_label(X_BEFORE, AB_H - 26, "BEFORE");
    ab_label(X_INSIDE, AB_H - 26, "INSIDE");
    ab_label(X_AFTER, AB_H - 26, "AFTER");
    static const char *names[ROWS] = {
        "POLYGON", "STIPPLE", "POINTS", "CLIP", "TEXENV"
    };
    for (int r = 0; r < ROWS; r++)
        ab_label(8, row_y(r) + 18, names[r]);

    int y;

    /* POLYGON */
    y = row_y(0);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    polygon_shape(X_BEFORE, y);
    glPushAttrib(GL_POLYGON_BIT);
    glFrontFace(GL_CW);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    polygon_shape(X_INSIDE, y);
    glPopAttrib();
    polygon_shape(X_AFTER, y);
    glFrontFace(GL_CCW);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE);

    /* LINE STIPPLE */
    y = row_y(1);
    glDisable(GL_LINE_STIPPLE);
    glLineStipple(1, 0xFFFF);
    lines_shape(X_BEFORE, y);
    glPushAttrib(GL_LINE_BIT);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(4, 0x0F0F);
    lines_shape(X_INSIDE, y);
    glPopAttrib();
    lines_shape(X_AFTER, y);
    glDisable(GL_LINE_STIPPLE);
    glLineStipple(1, 0xFFFF);

    /* POINT PARAMS */
    static const GLfloat no_attenuation[3] = { 1, 0, 0 };
    static const GLfloat attenuation[3] = { 1, 0, 0.0005f };
    y = row_y(2);
    glPointSize(14);
    glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, no_attenuation);
    points_shape(X_BEFORE, y);
    glPushAttrib(GL_POINT_BIT);
    glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, attenuation);
    points_shape(X_INSIDE, y);
    glPopAttrib();
    points_shape(X_AFTER, y);
    glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, no_attenuation);
    glPointSize(1);

    /* CLIP PLANE: keep y <= mid before the push, y >= mid inside it. */
    y = row_y(3);
    GLdouble mid = y + BOX_H / 2;
    GLdouble keep_bottom[4] = { 0, -1, 0, mid };
    GLdouble keep_top[4] = { 0, 1, 0, -mid };
    glClipPlane(GL_CLIP_PLANE0, keep_bottom);
    glEnable(GL_CLIP_PLANE0);
    rect_shape(X_BEFORE, y);
    glPushAttrib(GL_TRANSFORM_BIT);
    glClipPlane(GL_CLIP_PLANE0, keep_top);
    rect_shape(X_INSIDE, y);
    glPopAttrib();
    rect_shape(X_AFTER, y);
    glDisable(GL_CLIP_PLANE0);

    /* TEXTURE ENV */
    y = row_y(4);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, checker);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    textured_shape(X_BEFORE, y);
    glPushAttrib(GL_TEXTURE_BIT);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    textured_shape(X_INSIDE, y);
    glPopAttrib();
    textured_shape(X_AFTER, y);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    /* Box outlines, to show where each shape should sit. */
    glColor3f(0.3f, 0.3f, 0.35f);
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < 3; c++) {
            static const int xs[3] = { X_BEFORE, X_INSIDE, X_AFTER };
            float x0 = xs[c] - 3.5f, y0 = row_y(r) - 3.5f;
            glBegin(GL_LINE_LOOP);
            glVertex2f(x0, y0);
            glVertex2f(x0 + BOX_W + 7, y0);
            glVertex2f(x0 + BOX_W + 7, y0 + BOX_H + 7);
            glVertex2f(x0, y0 + BOX_H + 7);
            glEnd();
        }

    ab_report("AFTER should look like BEFORE in every row: a green filled triangle, "
              "solid lines, equal points, the bottom half, a red checkerboard");
}
