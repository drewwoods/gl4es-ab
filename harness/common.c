#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ab.h"

static char report[256];
static char status[512];

/* Frame timing: CPU time from the start of demo_draw() to the end of
 * gl4es_pre_swap(), which draws what gl4es still has queued (glBitmap
 * batches, for one). GPU work done later isn't counted, but a driver call
 * that waits for the GPU (a WebGL getParameter, say) is. Frames are
 * averaged in windows of AB_WINDOW; the headline number is the median of
 * the windows so far, which shrugs off the odd slow one. */
#define AB_MAX_WINDOWS 512
static double win_sum;
static int win_n;
static double windows[AB_MAX_WINDOWS];  /* ring of window averages */
static int nwindows;                     /* windows completed since the reset */

double ab_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

void ab_report(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(report, sizeof report, fmt, ap);
    va_end(ap);
}

void ab_frame(double t)
{
    double t0 = ab_now_ms();
    demo_draw(t);
#ifndef AB_DESKTOP_GL
    gl4es_pre_swap();
#endif
    win_sum += ab_now_ms() - t0;
    if (++win_n == AB_WINDOW) {
        windows[nwindows++ % AB_MAX_WINDOWS] = win_sum / win_n;
        win_sum = 0;
        win_n = 0;
    }
}

int ab_windows(void) { return nwindows; }

double ab_window_ms(void)
{
    return nwindows ? windows[(nwindows - 1) % AB_MAX_WINDOWS] : -1;
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

double ab_median_ms(void)
{
    static double sorted[AB_MAX_WINDOWS];
    int n = nwindows < AB_MAX_WINDOWS ? nwindows : AB_MAX_WINDOWS;
    if (!n)
        return -1;
    memcpy(sorted, windows, n * sizeof *sorted);
    qsort(sorted, n, sizeof *sorted, cmp_double);
    return n % 2 ? sorted[n / 2] : (sorted[n / 2 - 1] + sorted[n / 2]) / 2;
}

void ab_reset_timing(void)
{
    nwindows = 0;
    win_sum = 0;
    win_n = 0;
}

#define AB_MAX_PARAMS 16
static struct {
    const char *name;
    GLfloat *value;
    float min, max, step;
} params[AB_MAX_PARAMS];
static int nparams;

void ab_param(const char *name, GLfloat *value, float min, float max, float step)
{
    if (nparams == AB_MAX_PARAMS)
        return;
    params[nparams].name = name;
    params[nparams].value = value;
    params[nparams].min = min;
    params[nparams].max = max;
    params[nparams].step = step;
    nparams++;
}

int ab_set_param(const char *name, float value)
{
    for (int i = 0; i < nparams; i++)
        if (!strcmp(params[i].name, name)) {
            *params[i].value = value;
            return 1;
        }
    return 0;
}

void ab_set_param_index(int i, float value)
{
    if (i >= 0 && i < nparams)
        *params[i].value = value;
}

/* [{"name":..,"value":..,"min":..,"max":..,"step":..}, ...] for the web page.
 * Names are plain identifiers, so they need no escaping. */
const char *ab_params_json(void)
{
    static char json[AB_MAX_PARAMS * 128];
    int n = snprintf(json, sizeof json, "[");
    for (int i = 0; i < nparams; i++)
        n += snprintf(json + n, sizeof json - n,
                      "%s{\"name\":\"%s\",\"value\":%g,\"min\":%g,\"max\":%g,\"step\":%g}",
                      i ? "," : "", params[i].name, *params[i].value,
                      params[i].min, params[i].max, params[i].step);
    snprintf(json + n, sizeof json - n, "]");
    return json;
}

const char *ab_status(void)
{
    /* "A (original) · master@81547d9867", or just "... · 81547d9867" when
     * the ref is a SHA; then the latest window's frame time. */
    int named = strcmp(AB_REF, AB_SHA) != 0;
    char ms[32] = "…";
    if (nwindows)
        snprintf(ms, sizeof ms, "%.3f", ab_window_ms());
    snprintf(status, sizeof status, "%s · %s%s%s · frame %s ms%s%s",
             AB_SIDE, named ? AB_REF : "", named ? "@" : "", AB_SHA,
             ms, report[0] ? " · " : "", report);
    return status;
}

/* ab_label: 5x7 font, drawn at 2x with glBitmap under a window-space
 * glOrtho (w = 1, a raster position gl4es master already handles). Rows are
 * top first; bit 4 is the leftmost column. */
static const struct { char c; unsigned char rows[7]; } font[] = {
    {'A', {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}}, {'B', {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
    {'C', {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}}, {'D', {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C}},
    {'E', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}}, {'F', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
    {'G', {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}}, {'H', {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'I', {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}}, {'J', {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}},
    {'K', {0x11,0x12,0x14,0x18,0x14,0x12,0x11}}, {'L', {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'M', {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}}, {'N', {0x11,0x11,0x19,0x15,0x13,0x11,0x11}},
    {'O', {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}}, {'P', {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'Q', {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}}, {'R', {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S', {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}}, {'T', {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'U', {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}}, {'V', {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
    {'W', {0x11,0x11,0x11,0x15,0x15,0x15,0x0A}}, {'X', {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
    {'Y', {0x11,0x11,0x11,0x0A,0x04,0x04,0x04}}, {'Z', {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},
    {'0', {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}}, {'1', {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'2', {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}}, {'3', {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}},
    {'(', {0x02,0x04,0x08,0x08,0x08,0x04,0x02}}, {')', {0x08,0x04,0x02,0x02,0x02,0x04,0x08}},
    {'=', {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00}}, {'-', {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}},
    {':', {0x00,0x0C,0x0C,0x00,0x0C,0x0C,0x00}}, {',', {0x00,0x00,0x00,0x00,0x0C,0x04,0x08}},
    {'.', {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C}}, {'/', {0x01,0x01,0x02,0x04,0x08,0x10,0x10}},
};
#define GW 10 /* glyph size at 2x */
#define GH 14

/* Glyph c at 2x as a glBitmap image: GH rows, bottom first, 2 bytes each. */
static const GLubyte *glyph(char c)
{
    static GLubyte img[sizeof font / sizeof font[0]][GH * 2];
    static int ready;
    if (!ready) {
        for (unsigned i = 0; i < sizeof font / sizeof font[0]; i++)
            for (int y = 0; y < GH; y++)
                for (int x = 0; x < GW; x++)
                    if (font[i].rows[6 - y / 2] & (0x10 >> (x / 2)))
                        img[i][y * 2 + x / 8] |= 0x80 >> (x % 8);
        ready = 1;
    }
    for (unsigned i = 0; i < sizeof font / sizeof font[0]; i++)
        if (font[i].c == c)
            return img[i];
    return NULL;
}

void ab_label(int x, int y, const char *s)
{
    ab_label_vp(AB_W, AB_H, x, y, s);
}

void ab_label_vp(int vw, int vh, int x, int y, const char *s)
{
    GLint align;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &align);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, vw, 0, vh, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glRasterPos2i(x, y);
    for (; *s; s++) {
        const GLubyte *g = glyph(*s);
        /* Unknown characters (and space) just advance. */
        glBitmap(g ? GW : 0, g ? GH : 0, 0, 0, GW + 2, 0, g);
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPixelStorei(GL_UNPACK_ALIGNMENT, align);
}
