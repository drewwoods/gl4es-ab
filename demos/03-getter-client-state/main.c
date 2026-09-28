/* 03-getter-client-state: glGet* calls that wait for the GPU.
 *
 * The 02 grid of labels, each after a quad, with a heavier fill behind the
 * grid so the GPU has work queued. Each label is also wrapped in
 * glPushAttrib/glPopAttrib, as HUD code often does. A glGet* that gl4es
 * passes to the driver is, on WebGL, a synchronous gl.getParameter(): the
 * browser finishes every queued GPU command before it answers. On A,
 * glPushAttrib reads the clear color, the line width, the viewport, the
 * scissor box and the mipmap hint from the driver, and so does every
 * glBitmap batch, whose blit pushes GL_COLOR_BUFFER_BIT. On B, gl4es
 * answers from its own state.
 *
 * The picture is the same in A and B: compare the frame times. Native
 * drivers answer glGet* without waiting, so expect little difference
 * there; the stall is a WebGL one.
 *
 * glGetBooleanv had no gl4es version at all: every call went to the
 * driver, a stall on WebGL, and state only gl4es knows (GL_LIGHTING on
 * GLES2) came back unwritten. getbooleanv = 1 checks two flags with it
 * before each label, as HUD code does; the status line reports whether
 * glGetBooleanv(GL_LIGHTING) returns the value just set.
 *
 * batches (1..200) sets the number of labels; pushattrib (0 or 1) turns
 * the explicit glPushAttrib off, leaving only the blit's; getbooleanv (0
 * or 1) turns the glGetBooleanv checks off. */
#include <math.h>

#include "ab.h"

static GLfloat batches = 60;
static GLfloat pushattrib = 1;
static GLfloat getbooleanv = 1;

void demo_init(void)
{
    ab_param("batches", &batches, 1, 200, 1);
    ab_param("pushattrib", &pushattrib, 0, 1, 1);
    ab_param("getbooleanv", &getbooleanv, 0, 1, 1);
}

void demo_draw(double t)
{
    glViewport(0, 0, AB_W, AB_H);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    /* Does glGetBooleanv see state only gl4es tracks? 2 = left unwritten. */
    GLboolean lit[2] = { 2, 2 };
    glEnable(GL_LIGHTING);
    glGetBooleanv(GL_LIGHTING, &lit[0]);
    glDisable(GL_LIGHTING);
    glGetBooleanv(GL_LIGHTING, &lit[1]);
    const char *lighting = lit[0] == GL_TRUE && lit[1] == GL_FALSE ? "correct"
                         : lit[0] == 2 ? "left unwritten" : "wrong";

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, AB_W, 0, AB_H, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Background: overlapping translucent bands, to give the GPU some fill
     * work that a waiting getParameter has to sit through. */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < 24; i++) {
        float y = fmodf((float)(t * 40.0) + i * 15.0f, AB_H);
        glColor4f(0.2f, 0.3f + 0.02f * i, 0.5f, 0.08f);
        glBegin(GL_QUADS);
        glVertex2f(0, y - 60);
        glVertex2f(AB_W, y - 60);
        glVertex2f(AB_W, y + 60);
        glVertex2f(0, y + 60);
        glEnd();
    }
    glDisable(GL_BLEND);

    /* 6 columns of 20 rows; past 120 labels the grid starts over. */
    int n = (int)batches;
    for (int i = 0; i < n; i++) {
        int col = i % 6, row = (i / 6) % 20;
        int x = 8 + col * 78, y = AB_H - 40 - row * 16;
        float phase = (float)(t * 2.0 + i * 0.4);
        glColor3f(0.5f + 0.5f * sinf(phase), 0.6f, 0.5f + 0.5f * cosf(phase));
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + 10, y);
        glVertex2f(x + 10, y + 10);
        glVertex2f(x, y + 10);
        glEnd();
        if (getbooleanv) {
            /* Both are answered by gl4es_commonGet() in B. Enable flags
             * such as GL_BLEND still go to the driver in every glGet*. */
            GLboolean depth, lighting;
            glGetBooleanv(GL_DEPTH_WRITEMASK, &depth);
            glGetBooleanv(GL_LIGHTING, &lighting);
        }
        if (pushattrib)
            glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LINE_BIT
                         | GL_VIEWPORT_BIT | GL_SCISSOR_BIT | GL_HINT_BIT);
        glColor3f(0.9f, 0.9f, 0.9f);
        ab_label(x + 16, y, "TEXT");
        if (pushattrib)
            glPopAttrib();
    }
    ab_report("%d glBitmap batches%s%s; glGetBooleanv(GL_LIGHTING): %s; "
              "the frame is the same in A and B, compare the frame times",
              n, pushattrib ? ", each in glPushAttrib" : "",
              getbooleanv ? " and after 2 glGetBooleanv" : "", lighting);
}
