/* macOS desktop GL reference with an accumulation buffer (refs: ACCUM=1).
 * Only a window-system framebuffer has an accumulation buffer, not the
 * framebuffer object native.c draws into otherwise, and CGL pbuffers are
 * gone, so this draws into an AB_W x AB_H window placed off screen. It is
 * never activated and doesn't show in the Dock. */
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#import <Cocoa/Cocoa.h>

#include "ab.h"

void ab_mac_window_context(void)
{
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
    NSOpenGLPixelFormatAttribute attr[] = {
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy, NSOpenGLPFAAccelerated,
        NSOpenGLPFAColorSize, 24, NSOpenGLPFAAlphaSize, 8, NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAStencilSize, 8, NSOpenGLPFAAccumSize, 64, 0};
    NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attr];
    if (!pf) {
        fprintf(stderr, "no pixel format with an accumulation buffer\n");
        exit(1);
    }
    NSRect r = NSMakeRect(0, 0, AB_W, AB_H);
    NSWindow *win = [[NSWindow alloc] initWithContentRect:NSOffsetRect(r, -20000, -20000)
                                                styleMask:NSWindowStyleMaskBorderless
                                                  backing:NSBackingStoreBuffered
                                                    defer:NO];
    NSOpenGLView *view = [[NSOpenGLView alloc] initWithFrame:r pixelFormat:pf];
    [view setWantsBestResolutionOpenGLSurface:NO]; /* AB_W x AB_H pixels */
    [win setContentView:view];
    [win orderFrontRegardless];
    NSOpenGLContext *ctx = [view openGLContext];
    /* NSOpenGLView would attach its context when drawn, which this never is;
     * setView: is the only other way, and it's deprecated like all of GL. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [ctx setView:view];
#pragma clang diagnostic pop
    [ctx makeCurrentContext];
    [ctx update];
    glViewport(0, 0, AB_W, AB_H);
}
