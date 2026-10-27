/* 10-accum: the accumulation buffer.
 *
 * Each panel draws its scene several times, a little differently each pass,
 * and averages the passes in the accumulation buffer with glAccum: the
 * classic fixed-function way to get antialiasing, motion blur and soft
 * shadows. The scissor box bounds every accumulation operation, so each
 * panel accumulates on its own. Without an accumulation buffer, glAccum does
 * nothing and a panel shows only its last pass. Each panel is plain OpenGL
 * between a "test:" comment and "end test"; build.sh shows those blocks on
 * the page. */
#include <math.h>

#include "ab.h"

#define PW 240 /* panel size: four panels in a 2 x 2 grid */
#define PH 180

static GLfloat passes = 16;

/* Halton sequence in [0, 1): spreads the first n samples evenly for any n. */
static float halton(int i, int base)
{
    float f = 1, r = 0;
    for (i++; i > 0; i /= base) {
        f /= base;
        r += f * (i % base);
    }
    return r;
}

/* Panel 0 top left, 1 top right, 2 bottom left, 3 bottom right: its
 * viewport, and the scissor box that bounds glClear and glAccum to it. */
static void panel(int p)
{
    int x = (p % 2) * PW, y = (p < 2) * PH;
    glViewport(x, y, PW, PH);
    glScissor(x, y, PW, PH);
}

static void clear_pass(void)
{
    glClearColor(0.10f, 0.11f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/* A perspective projection moved by (jx, jy) pixels. */
static void frustum(float jx, float jy)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glTranslatef(2 * jx / PW, 2 * jy / PH, 0);
    glFrustum(-0.5f * PW / PH, 0.5f * PW / PH, -0.5f, 0.5f, 1, 30);
    glMatrixMode(GL_MODELVIEW);
}

static void ortho(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0f * PW / PH, 1.0f * PW / PH, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/* A cube of half-size s; colored faces, or all in the current color. */
static void cube(float s, int colored)
{
    static const GLfloat color[6][3] = {
        {0.95f, 0.35f, 0.25f}, {0.25f, 0.70f, 0.95f}, {0.95f, 0.85f, 0.30f},
        {0.40f, 0.85f, 0.45f}, {0.85f, 0.45f, 0.90f}, {0.95f, 0.95f, 0.95f}};
    static const signed char v[6][4][3] = {
        {{1, -1, -1}, {1, 1, -1}, {1, 1, 1}, {1, -1, 1}},
        {{-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}, {-1, -1, -1}},
        {{-1, 1, -1}, {-1, 1, 1}, {1, 1, 1}, {1, 1, -1}},
        {{-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}},
        {{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}},
        {{1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}}};
    glBegin(GL_QUADS);
    for (int f = 0; f < 6; f++) {
        glNormal3f(f == 0 ? 1 : f == 1 ? -1 : 0, f == 2 ? 1 : f == 3 ? -1 : 0,
                   f == 4 ? 1 : f == 5 ? -1 : 0);
        if (colored)
            glColor3fv(color[f]);
        for (int k = 0; k < 4; k++)
            glVertex3f(s * v[f][k][0], s * v[f][k][1], s * v[f][k][2]);
    }
    glEnd();
}

static void disc(float x, float y, float r)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 32; i++)
        glVertex2f(x + r * cosf(i * 6.2831853f / 32), y + r * sinf(i * 6.2831853f / 32));
    glEnd();
}

/* A three-blade propeller turned by a degrees. */
static void propeller(float a)
{
    glLoadIdentity();
    glRotatef(a, 0, 0, 1);
    for (int b = 0; b < 3; b++) {
        glColor3f(b == 0 ? 0.95f : 0.30f, b == 1 ? 0.85f : 0.40f, b == 2 ? 0.95f : 0.30f);
        glRectf(0.12f, -0.07f, 0.85f, 0.07f);
        glRotatef(120, 0, 0, 1);
    }
    glColor3f(0.9f, 0.9f, 0.9f);
    disc(0, 0, 0.14f);
}

/* Flattens geometry onto plane p, seen from the point light l (w = 1). */
static void shadow_matrix(GLfloat m[16], const GLfloat p[4], const GLfloat l[4])
{
    float d = p[0] * l[0] + p[1] * l[1] + p[2] * l[2] + p[3] * l[3];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            m[c * 4 + r] = (r == c ? d : 0) - l[r] * p[c];
}

/* The soft-shadow scene, lit from l: a ground plane, the cube's shadow
 * flattened onto it, then the cube. */
static void shadow_scene(const GLfloat l[4], float spin)
{
    static const GLfloat ground[4] = {0, 1, 0, 0};
    GLfloat m[16];
    glLoadIdentity();
    glTranslatef(0, -1.2f, -6);
    glRotatef(28, 1, 0, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, l);
    glEnable(GL_LIGHTING);
    glColor3f(0.55f, 0.62f, 0.55f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-8, 0, 8);
    glVertex3f(8, 0, 8);
    glVertex3f(8, 0, -8);
    glVertex3f(-8, 0, -8);
    glEnd();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    shadow_matrix(m, ground, l);
    glPushMatrix();
    glMultMatrixf(m);
    glTranslatef(0, 1.1f, 0);
    glRotatef(spin, 0, 1, 0);
    glColor3f(0.14f, 0.16f, 0.14f);
    cube(0.5f, 0);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glTranslatef(0, 1.1f, 0);
    glRotatef(spin, 0, 1, 0);
    glColor3f(0.95f, 0.55f, 0.20f);
    cube(0.5f, 0);
    glDisable(GL_LIGHTING);
}

static GLint accum_bits, clear_int[4];

void demo_init(void)
{
    ab_param("passes", &passes, 1, 16, 1);

    static const GLfloat ambient[4] = {0.25f, 0.25f, 0.28f, 1};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);

    /* test: CLEAR VALUE
     * The status line's numbers. GL_ACCUM_RED_BITS is the buffer's depth
     * (0: there's none, as on master), and glGetIntegerv maps the clear
     * value's [-1, 1] onto [-INT_MAX, INT_MAX], so 0.5 reads back as
     * 1073741823. */
    glGetIntegerv(GL_ACCUM_RED_BITS, &accum_bits);
    glClearAccum(0.5f, 0, 0, 0);
    glGetIntegerv(GL_ACCUM_CLEAR_VALUE, clear_int);
    glClearAccum(0, 0, 0, 0);
    /* end test */
    while (glGetError() != GL_NO_ERROR)
        ;
}

void demo_draw(double t)
{
    int n = (int)passes;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    /* test: JITTER AA
     * A spinning cube drawn n times, the projection moved by a different
     * fraction of a pixel each pass, and averaged: its edges come out
     * smooth. The first pass loads the buffer, the rest add to it. With no
     * accumulation buffer, only the last pass shows, its edges jagged. */
    panel(0);
    for (int i = 0; i < n; i++) {
        clear_pass();
        frustum(halton(i, 2) - 0.5f, halton(i, 3) - 0.5f);
        glLoadIdentity();
        glTranslatef(0, 0, -4);
        glRotatef(25, 1, 0, 0);
        glRotatef((float)t * 18, 0, 1, 0);
        glRotatef(12, 0, 0, 1);
        cube(0.8f, 1);
        glAccum(i ? GL_ACCUM : GL_LOAD, 1.0f / n);
    }
    glAccum(GL_RETURN, 1.0f);
    /* end test */

    /* test: MOTION BLUR
     * A propeller turning twice a second, drawn at n moments across a
     * 1/24 s shutter and averaged: the blades smear into fans. This one
     * clears the buffer and adds every pass. With no accumulation buffer,
     * only the last pass shows: three sharp blades. */
    panel(1);
    ortho();
    glClear(GL_ACCUM_BUFFER_BIT);
    for (int i = 0; i < n; i++) {
        clear_pass();
        propeller(-720 * (float)(t + (i + 0.5) / n / 24));
        glAccum(GL_ACCUM, 1.0f / n);
    }
    glAccum(GL_RETURN, 1.0f);
    /* end test */

    /* test: SOFT SHADOW
     * A cube over the ground, its shadow flattened onto the ground with a
     * shadow matrix (a planar projected shadow), lit from n points across an
     * area light and averaged: the shadow gets a soft penumbra. With no
     * accumulation buffer, only the last pass shows: a hard shadow. */
    panel(2);
    frustum(0, 0);
    for (int i = 0; i < n; i++) {
        GLfloat light[4] = {2.2f + 1.6f * (halton(i, 2) - 0.5f), 4.5f,
                            1.8f + 1.6f * (halton(i, 3) - 0.5f), 1};
        clear_pass();
        shadow_scene(light, (float)t * 6);
        glAccum(i ? GL_ACCUM : GL_LOAD, 1.0f / n);
    }
    glAccum(GL_RETURN, 1.0f);
    /* end test */

    /* test: TRAIL (MULT)
     * A ball circling every 3 s, drawn at its last 8 positions, oldest
     * first: before adding each one, glAccum(GL_MULT) fades what's there,
     * so the ball trails a fading tail. GL_MULT is a known gap in gl4es: it
     * does nothing, so B draws all 8 balls equally bright. With no
     * accumulation buffer, only the last ball shows. The background is
     * black, so the weights don't change it. */
    panel(3);
    ortho();
    glClear(GL_ACCUM_BUFFER_BIT);
    glClearColor(0, 0, 0, 1);
    for (int i = 7; i >= 0; i--) {
        float a = (float)(t - i * 0.05) * 6.2831853f / 3;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glColor3f(1, 1, 1);
        disc(0.6f * cosf(a), 0.6f * sinf(a), 0.13f);
        glAccum(GL_MULT, 0.6f);
        glAccum(GL_ACCUM, 0.4f);
    }
    glAccum(GL_RETURN, 2.5f);
    /* end test */

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, AB_W, AB_H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, AB_W, 0, AB_H, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(0.75f, 0.75f, 0.80f);
    ab_label(8, PH + 8, "JITTER AA");
    ab_label(PW + 8, PH + 8, "MOTION BLUR");
    ab_label(8, 8, "SOFT SHADOW");
    ab_label(PW + 8, 8, "TRAIL (MULT)");
    glColor3f(0.30f, 0.32f, 0.38f);
    glBegin(GL_LINES);
    glVertex2f(PW + 0.5f, 0);
    glVertex2f(PW + 0.5f, AB_H);
    glVertex2f(0, PH + 0.5f);
    glVertex2f(AB_W, PH + 0.5f);
    glEnd();
    ab_report("%d passes; GL_ACCUM_RED_BITS %d; clear value 0.5 as an integer: %d (should be 1073741823)",
              n, accum_bits, clear_int[0]);
}
