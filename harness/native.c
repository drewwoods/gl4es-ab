/* Native host: Mesa through EGL, either headless (surfaceless platform and a
 * pbuffer) or in an X11 window. On macOS, Apple's OpenGL through CGL, headless
 * only, and only the -DAB_DESKTOP_GL reference: a legacy (2.1) context
 * drawing into a framebuffer object, as there is no window.
 *   gl4es build:        GLES2 context; gl4es (-DNOX11 -DNOEGL -DSTATICLIB,
 *                       statically linked) dlopens libGLESv2 and draws on it.
 *   -DAB_DESKTOP_GL:    desktop GL compatibility context, no gl4es; the
 *                       reference for what the GL spec expects.
 *
 * usage: <demo> [--t <seconds>] [--frames <n>] [--screenshot <file.ppm>] [--set <name>=<value>]...
 *        <demo> --window [--x <pos>] [--t <seconds>] [--frames <n>] [--set <name>=<value>]...
 * Headless: draws <n> frames (default 1) at time t (default 1.0), reports the
 * average draw time, and optionally saves the last frame.
 * --window: an AB_W x AB_H X11 window at x = <pos> on $DISPLAY, animated by
 * the wall clock (like the web page, so several windows stay in step) unless
 * --t fixes the time. Runs until closed, Esc or q, or for <n> frames.
 * --set changes a value the demo registered with ab_param(). */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/glext.h>
#else
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#endif

#ifndef AB_DESKTOP_GL
#include <gl4esinit.h>
#endif

#include "ab.h"

#ifdef __APPLE__
static void die(const char *what)
{
    fprintf(stderr, "%s failed\n", what);
    exit(1);
}

static void make_context(int win_x)
{
    if (win_x >= 0) {
        fprintf(stderr, "--window isn't supported on macOS\n");
        exit(1);
    }
    CGLPixelFormatAttribute attr[] = {
        kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)kCGLOGLPVersion_Legacy,
        kCGLPFAAccelerated, kCGLPFAColorSize, 24, kCGLPFAAlphaSize, 8,
        kCGLPFADepthSize, 24, kCGLPFAStencilSize, 8, 0 };
    CGLPixelFormatObj pf;
    GLint npf;
    CGLContextObj ctx;
    if (CGLChoosePixelFormat(attr, &pf, &npf) != kCGLNoError || !pf)
        die("CGLChoosePixelFormat");
    if (CGLCreateContext(pf, NULL, &ctx) != kCGLNoError)
        die("CGLCreateContext");
    CGLDestroyPixelFormat(pf);
    CGLSetCurrentContext(ctx);
    GLuint fbo, rb[2];
    glGenFramebuffersEXT(1, &fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    glGenRenderbuffersEXT(2, rb);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb[0]);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, AB_W, AB_H);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                 GL_RENDERBUFFER_EXT, rb[0]);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb[1]);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, AB_W, AB_H);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, rb[1]);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, rb[1]);
    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
        die("framebuffer");
    glViewport(0, 0, AB_W, AB_H);
}

static int pump_events(void) { return 1; }
#else
static EGLDisplay egl_dpy;
static EGLSurface egl_surf;
static Display *x_dpy;
static Atom wm_delete;

static void die(const char *what)
{
    fprintf(stderr, "%s failed (EGL error 0x%x)\n", what, eglGetError());
    exit(1);
}

/* win_x < 0: headless pbuffer; otherwise an X11 window at that x. */
static void make_context(int win_x)
{
    if (win_x < 0) {
        egl_dpy = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, NULL, NULL);
    } else {
        if (!(x_dpy = XOpenDisplay(NULL))) {
            fprintf(stderr, "cannot open X display (DISPLAY=%s)\n", getenv("DISPLAY"));
            exit(1);
        }
        egl_dpy = eglGetPlatformDisplay(EGL_PLATFORM_X11_KHR, x_dpy, NULL);
    }
    if (egl_dpy == EGL_NO_DISPLAY || !eglInitialize(egl_dpy, NULL, NULL))
        die("eglInitialize");
#ifdef AB_DESKTOP_GL
    EGLint api = EGL_OPENGL_API, renderable = EGL_OPENGL_BIT;
    const EGLint ctx_attr[] = {
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT,
        EGL_NONE };
#else
    EGLint api = EGL_OPENGL_ES_API, renderable = EGL_OPENGL_ES2_BIT;
    const EGLint ctx_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
#endif
    if (!eglBindAPI(api))
        die("eglBindAPI");
    const EGLint cfg_attr[] = {
        EGL_SURFACE_TYPE, win_x < 0 ? EGL_PBUFFER_BIT : EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, renderable,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8, EGL_NONE };
    EGLConfig cfg;
    EGLint n;
    if (!eglChooseConfig(egl_dpy, cfg_attr, &cfg, 1, &n) || n < 1)
        die("eglChooseConfig");

    if (win_x < 0) {
        const EGLint pb_attr[] = { EGL_WIDTH, AB_W, EGL_HEIGHT, AB_H, EGL_NONE };
        egl_surf = eglCreatePbufferSurface(egl_dpy, cfg, pb_attr);
    } else {
        Window win = XCreateSimpleWindow(x_dpy, DefaultRootWindow(x_dpy), win_x, 0,
                                         AB_W, AB_H, 0, 0, 0);
        char title[128];
        snprintf(title, sizeof title, "%s %s@%s", AB_SIDE, AB_REF, AB_SHA);
        XStoreName(x_dpy, win, title);
        /* Fixed size, and ask the window manager to honour the position. */
        XSizeHints hints = { .flags = PPosition | USPosition | PMinSize | PMaxSize,
                             .x = win_x, .min_width = AB_W, .max_width = AB_W,
                             .min_height = AB_H, .max_height = AB_H };
        XSetWMNormalHints(x_dpy, win, &hints);
        wm_delete = XInternAtom(x_dpy, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(x_dpy, win, &wm_delete, 1);
        XSelectInput(x_dpy, win, KeyPressMask);
        XMapWindow(x_dpy, win);
        egl_surf = eglCreatePlatformWindowSurface(egl_dpy, cfg, &win, NULL);
    }
    if (egl_surf == EGL_NO_SURFACE)
        die("eglCreate*Surface");
    EGLContext ctx = eglCreateContext(egl_dpy, cfg, EGL_NO_CONTEXT, ctx_attr);
    if (ctx == EGL_NO_CONTEXT)
        die("eglCreateContext");
    if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, ctx))
        die("eglMakeCurrent");
}

/* Returns 0 once the window was closed or Esc/q pressed. */
static int pump_events(void)
{
    while (XPending(x_dpy)) {
        XEvent ev;
        XNextEvent(x_dpy, &ev);
        if (ev.type == ClientMessage && (Atom)ev.xclient.data.l[0] == wm_delete)
            return 0;
        if (ev.type == KeyPress) {
            KeySym k = XLookupKeysym(&ev.xkey, 0);
            if (k == XK_Escape || k == XK_q)
                return 0;
        }
    }
    return 1;
}
#endif

static void write_ppm(const char *path)
{
    unsigned char *px = malloc(AB_W * AB_H * 4);
    glReadPixels(0, 0, AB_W, AB_H, GL_RGBA, GL_UNSIGNED_BYTE, px);
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror(path);
        exit(1);
    }
    fprintf(f, "P6\n%d %d\n255\n", AB_W, AB_H);
    for (int y = AB_H - 1; y >= 0; y--)
        for (int x = 0; x < AB_W; x++)
            fwrite(px + (y * AB_W + x) * 4, 1, 3, f);
    fclose(f);
    free(px);
}

int main(int argc, char **argv)
{
    double t = -1.0; /* < 0: headless default 1.0, windowed wall clock */
    int frames = -1, win_x = -1;
    const char *shot = NULL;
    const char *sets[16];
    int nsets = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--window"))
            win_x = win_x < 0 ? 0 : win_x;
        else if (i + 1 >= argc)
            break;
        else if (!strcmp(argv[i], "--x"))
            win_x = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--t"))
            t = atof(argv[++i]);
        else if (!strcmp(argv[i], "--frames"))
            frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--screenshot"))
            shot = argv[++i];
        else if (!strcmp(argv[i], "--set") && nsets < 16)
            sets[nsets++] = argv[++i];
    }
    int windowed = win_x >= 0;
    if (!windowed && t < 0)
        t = 1.0;
    if (!windowed && frames < 0)
        frames = 1;

    make_context(win_x);
#ifndef AB_DESKTOP_GL
    initialize_gl4es(); /* no-op if gl4es's constructor already ran */
#else
    /* build.sh keeps this line as the caption of the native reference. */
    printf("renderer: %s, %s\n", (const char *)glGetString(GL_RENDERER),
           (const char *)glGetString(GL_VERSION));
#endif
    demo_init();
    for (int i = 0; i < nsets; i++) {
        char name[64];
        const char *eq = strchr(sets[i], '=');
        int len = eq ? (int)(eq - sets[i]) : 0;
        if (len > 0 && len < (int)sizeof name) {
            memcpy(name, sets[i], len);
            name[len] = 0;
            if (ab_set_param(name, (float)atof(eq + 1)))
                continue;
        }
        fprintf(stderr, "--set %s: no such value; this demo has %s\n", sets[i], ab_params_json());
        return 1;
    }
    for (int i = 0; frames < 0 || i < frames; i++) {
        if (windowed && !pump_events())
            break;
        double ft = t;
        if (ft < 0) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            ft = fmod(tv.tv_sec + tv.tv_usec / 1e6, 60.0);
        }
        ab_frame(ft);  /* includes gl4es_pre_swap() */
        if (shot && i == frames - 1)
            write_ppm(shot);
#ifndef __APPLE__
        if (windowed)
            eglSwapBuffers(egl_dpy, egl_surf);
#endif
#ifndef AB_DESKTOP_GL
        gl4es_post_swap();
#endif
        if (windowed && i % 60 == 59) {
            printf("\r%s   ", ab_status());
            fflush(stdout);
        }
    }
    glFinish();
    printf("%s%s\n", windowed ? "\r" : "", ab_status());
    if (ab_windows())
        printf("timing: %.4f ms/frame, median of %d windows of %d frames\n",
               ab_median_ms(), ab_windows(), AB_WINDOW);
    return 0;
}
