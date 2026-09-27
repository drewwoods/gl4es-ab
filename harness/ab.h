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

/* Internal: shared between common.c and the web/native hosts. */
#ifndef AB_DESKTOP_GL
/* gl4es's swap hooks for hosts without glX/EGL (-DNOX11 -DNOEGL builds).
 * pre_swap flushes what gl4es still queues on the CPU, such as glBitmap
 * batches; glClear does not flush them. */
void gl4es_pre_swap(void);
void gl4es_post_swap(void);
#endif
void ab_frame(double t);
const char *ab_status(void);

#endif
