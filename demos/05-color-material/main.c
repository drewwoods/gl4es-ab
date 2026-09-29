/* 05-color-material: GL_COLOR_MATERIAL tracking the wrong face.
 *
 * Two-sided lighting, one row per case, each drawing a front-facing quad
 * (CCW) and a back-facing one (CW). The light and materials are set up so
 * a lit quad shows exactly its material's diffuse colour, and the EXPECTED
 * column draws the correct front and back colours unlit, for comparison.
 * Every row starts from both materials grey, GL_COLOR_MATERIAL off and the
 * current colour black, and sets its colours after enabling
 * GL_COLOR_MATERIAL: in Mesa, a glMaterial still queued at the glEnable
 * overwrites the colour the glEnable copies into the material.
 *
 *   row       what it does                              FRONT    BACK
 *   ONE FACE  glColorMaterial(GL_FRONT), orange         orange   grey
 *   SWITCH    track GL_FRONT with white, switch to      white    cyan
 *             GL_BACK, then cyan
 *   DISABLE   track both faces with cyan, then          cyan     cyan
 *             glDisable(GL_COLOR_MATERIAL)
 *   PUSH      track GL_FRONT with white, glPushAttrib   orange   grey
 *             (GL_LIGHTING_BIT), switch to GL_BACK,
 *             glPopAttrib, then orange
 *
 * The status line reports what glGetIntegerv returns for
 * GL_COLOR_MATERIAL_FACE and GL_COLOR_MATERIAL_PARAMETER. */
#include "ab.h"

#define ROWS 4
#define BOX_W 100
#define BOX_H 48
#define X_FRONT 130
#define X_BACK 250
#define X_EXPECT 370

static const GLfloat grey[4] = { 0.45f, 0.45f, 0.50f, 1.0f };
static const GLfloat orange[4] = { 1.0f, 0.55f, 0.10f, 1.0f };
static const GLfloat white[4] = { 0.95f, 0.95f, 0.95f, 1.0f };
static const GLfloat cyan[4] = { 0.10f, 0.85f, 0.95f, 1.0f };

void demo_init(void)
{
}

static int row_y(int row) { return AB_H - 100 - row * 72; }

/* Both materials grey, colour material off and back to its defaults, and
 * the current colour black, so each row's glColor changes it. */
static void reset_material(void)
{
    glDisable(GL_COLOR_MATERIAL);
    glColor3f(0, 0, 0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, grey);
}

/* A front-facing quad in the FRONT column and a back-facing one in the
 * BACK column. The back one's normal points away from the viewer, so
 * two-sided lighting flips it towards the light. */
static void draw_quads(int y)
{
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex2f(X_FRONT, y);
    glVertex2f(X_FRONT + BOX_W, y);
    glVertex2f(X_FRONT + BOX_W, y + BOX_H);
    glVertex2f(X_FRONT, y + BOX_H);
    glNormal3f(0, 0, -1);
    glVertex2f(X_BACK, y);
    glVertex2f(X_BACK, y + BOX_H);
    glVertex2f(X_BACK + BOX_W, y + BOX_H);
    glVertex2f(X_BACK + BOX_W, y);
    glEnd();
}

static void rect(float x, float y, float w, float h, const GLfloat *c)
{
    glColor4fv(c);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

static const char *get_result(GLenum pname, GLint want)
{
    GLint v = -1;
    glGetIntegerv(pname, &v);
    return v == want ? "correct" : v == -1 ? "left unwritten" : "wrong";
}

void demo_draw(double t)
{
    (void)t;
    glViewport(0, 0, AB_W, AB_H);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, AB_W, 0, AB_H, -1, 10);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0.6f, 0.6f, 0.65f);
    ab_label(X_FRONT - 6, AB_H - 26, "FRONT FACE");
    ab_label(X_BACK, AB_H - 26, "BACK FACE");
    ab_label(X_EXPECT, AB_H - 26, "EXPECTED");
    static const char *names[ROWS] = { "ONE FACE", "SWITCH", "DISABLE", "PUSH" };
    for (int r = 0; r < ROWS; r++)
        ab_label(8, row_y(r) + 18, names[r]);

    /* The expected colours, unlit: front half, back half. */
    static const GLfloat *expect[ROWS][2] = {
        { orange, grey }, { white, cyan }, { cyan, cyan }, { orange, grey },
    };
    for (int r = 0; r < ROWS; r++) {
        rect(X_EXPECT, row_y(r), BOX_W / 2, BOX_H, expect[r][0]);
        rect(X_EXPECT + BOX_W / 2, row_y(r), BOX_W / 2, BOX_H, expect[r][1]);
    }

    /* One directional light towards -z. Light ambient 0, diffuse 0.8, and
     * scene ambient 0.2: a quad facing the light shows its material's
     * ambient-and-diffuse colour as is. */
    static const GLfloat light_pos[4] = { 0, 0, 1, 0 };
    static const GLfloat light_diffuse[4] = { 0.8f, 0.8f, 0.8f, 1 };
    static const GLfloat black[4] = { 0, 0, 0, 1 };
    static const GLfloat scene_ambient[4] = { 0.2f, 0.2f, 0.2f, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, black);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, black);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, scene_ambient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    /* ONE FACE: only the front tracks the colour. */
    reset_material();
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glColor4fv(orange);
    draw_quads(row_y(0));

    /* SWITCH: the front keeps white once the back takes over. */
    reset_material();
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glColor4fv(white);
    glColorMaterial(GL_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor4fv(cyan);
    draw_quads(row_y(1));

    /* DISABLE: the materials keep the last tracked colour. */
    reset_material();
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glColor4fv(cyan);
    glDisable(GL_COLOR_MATERIAL);
    glColor4fv(white);
    draw_quads(row_y(2));

    /* PUSH: the pop puts the face back to GL_FRONT. */
    reset_material();
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glColor4fv(white);
    glPushAttrib(GL_LIGHTING_BIT);
    glColorMaterial(GL_BACK, GL_AMBIENT_AND_DIFFUSE);
    glPopAttrib();
    glColor4fv(orange);
    draw_quads(row_y(3));

    /* glGet of the colour-material face and mode. */
    reset_material();
    glColorMaterial(GL_BACK, GL_DIFFUSE);
    const char *face = get_result(GL_COLOR_MATERIAL_FACE, GL_BACK);
    const char *mode = get_result(GL_COLOR_MATERIAL_PARAMETER, GL_DIFFUSE);
    reset_material();

    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    glColor3f(1, 1, 1);

    ab_report("FRONT FACE and BACK FACE should match EXPECTED; "
              "glGet GL_COLOR_MATERIAL_FACE: %s, _PARAMETER: %s", face, mode);
}
