/* Web host: a WebGL2 context on #canvas with gl4es on top, the same setup
 * gl-repl ships (gl4es built -DNOX11 -DNOEGL -DSTATICLIB, linked with
 * -sFULL_ES2; gl4es's GL/gl.h maps gl* to gl4es_gl* under Emscripten). */
#include <math.h>

#include <emscripten.h>
#include <emscripten/html5.h>

#include <gl4esinit.h>

#include "ab.h"

/* < 0: follow the wall clock; otherwise frozen at this t (seconds). */
static double frozen_t = -1.0;

EMSCRIPTEN_KEEPALIVE void ab_set_time(double t) { frozen_t = t; }
EMSCRIPTEN_KEEPALIVE void ab_web_set_param(int i, double v) { ab_set_param_index(i, (float)v); }

static void tick(void)
{
    /* Wall-clock time keeps the A and B iframes on the same pose. Demos
     * should loop in a period dividing 60 s so the wrap is seamless. */
    double t = frozen_t >= 0 ? frozen_t : fmod(emscripten_date_now() / 1000.0, 60.0);
    ab_frame(t);
    /* The browser presents when tick() returns; this is our swap. */
    gl4es_pre_swap();
    gl4es_post_swap();
    EM_ASM({ document.getElementById('status').textContent = UTF8ToString($0); },
           ab_status());
}

int main(void)
{
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.majorVersion = 2;
    attr.stencil = 1;
    attr.antialias = 0;
    attr.preserveDrawingBuffer = 1; /* so the page can be screenshotted */
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attr);
    if (ctx <= 0) {
        attr.majorVersion = 1;
        ctx = emscripten_webgl_create_context("#canvas", &attr);
    }
    emscripten_webgl_make_context_current(ctx);
    emscripten_set_canvas_element_size("#canvas", AB_W, AB_H);

    initialize_gl4es();
    demo_init();

    /* ?t=<seconds> freezes the frame; the A/B parent page sends {t} messages. */
    frozen_t = EM_ASM_DOUBLE({
        var t = new URLSearchParams(location.search).get('t');
        window.addEventListener('message', function(e) {
            if (e.data && typeof e.data.t === 'number') Module._ab_set_time(e.data.t);
        });
        return t === null ? -1 : parseFloat(t);
    });
    /* Adjustable values: tell the A/B page what they are, and take {param, value}
     * messages from it. The page answers with its current values, so both
     * sides start from the same state. */
    EM_ASM({
        var params = JSON.parse(UTF8ToString($0));
        window.addEventListener('message', function(e) {
            if (e.data && typeof e.data.param === 'number')
                Module._ab_web_set_param(e.data.param, e.data.value);
        });
        if (params.length && window.parent !== window)
            window.parent.postMessage({ abParams: params }, '*');
    }, ab_params_json());
    emscripten_set_main_loop(tick, 0, 0);
    return 0;
}
