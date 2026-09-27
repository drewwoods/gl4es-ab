/* gl4es-ab harness: what a demo implements and what it may call. */
#ifndef AB_HARNESS_H
#define AB_HARNESS_H

#include <GL/gl.h>

/* build.sh passes these; defaults keep editors quiet. */
#ifndef AB_SIDE
#define AB_SIDE "?"
#define AB_REF "?"
#define AB_SHA "?"
#endif

#define AB_W 480
#define AB_H 360

/* Implemented by each demo. */
void demo_init(void);
/* Draw one frame. t is seconds in [0, 60); both sides of an A/B page get
 * the same t. Loop in a period that divides 60 s. */
void demo_draw(double t);

/* Provided by the harness. */
double ab_now_ms(void);
/* One status line under the canvas (web) or on stdout (native). */
void ab_report(const char *fmt, ...);
/* Text at window pixel (x, y), bottom-left, in the current colour. Upper
 * case, digits 0-3 and ( ) = - : , . / only; 12 px per character. Leaves
 * the matrix mode as GL_MODELVIEW and the raster position after the text. */
void ab_label(int x, int y, const char *s);
/* The same, for a viewport of vw x vh other than the default AB_W x AB_H. */
void ab_label_vp(int vw, int vh, int x, int y, const char *s);

/* A value the viewer can adjust, registered from demo_init(). The A/B page
 * shows a slider for it and sets it on both sides at once (and keeps it in
 * the page URL); natively, --set <name>=<value>. At most 16. */
void ab_param(const char *name, GLfloat *value, float min, float max, float step);

/* Internal: shared between common.c and the web/native hosts. */
int ab_set_param(const char *name, float value);  /* 0 if there's no such param */
void ab_set_param_index(int i, float value);
const char *ab_params_json(void);
#ifndef AB_DESKTOP_GL
/* gl4es's swap hooks for hosts without glX/EGL (-DNOX11 -DNOEGL builds).
 * pre_swap flushes what gl4es still queues on the CPU, such as glBitmap
 * batches; glClear does not flush them. ab_frame() calls it, so the
 * frame time includes that flush; the hosts call post_swap. */
void gl4es_pre_swap(void);
void gl4es_post_swap(void);
#endif
/* Draws a frame (demo_draw, then gl4es_pre_swap) and times it. */
void ab_frame(double t);
const char *ab_status(void);
/* Frame times, averaged over windows of AB_WINDOW frames. -1 until the
 * first window completes. */
#define AB_WINDOW 30
int ab_windows(void);          /* windows completed since the last reset */
double ab_window_ms(void);     /* the latest window's average */
double ab_median_ms(void);     /* median of the windows since the reset */
void ab_reset_timing(void);

#endif
