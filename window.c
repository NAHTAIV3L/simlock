
#include "./window.h"
#include "./state.h"
#include "./glad/glad.h"
#include <stdlib.h>
#include <wayland-egl-core.h>

extern PFNEGLCREATEPLATFORMWINDOWSURFACEPROC eglCreatePlatformWindowSurfaceEXT;

void window_init(window_t *win) {
    win->egl_window = wl_egl_window_create(win->surface, win->width, win->height);
    if (!win->egl_window) {
        fprintf(stderr, "Failed to create EGL window\n");
        exit(1);
    }
    win->egl_surface = eglCreatePlatformWindowSurfaceEXT(win->state->egl_display,
                                                         win->state->egl_config,
                                                         win->egl_window, NULL);
    if (!win->egl_surface) {
        fprintf(stderr, "Failed to create EGL surface\n");
        exit(1);
    }
}

void window_destroy(window_t *win) {
    eglDestroySurface(win->state->egl_display, win->egl_surface);
    wl_egl_window_destroy(win->egl_window);
    ext_session_lock_surface_v1_destroy(win->ext_session_lock_surface);
    wl_surface_destroy(win->surface);
    wl_output_destroy(win->output);
}

void window_redraw(window_t *win) {
    eglMakeCurrent(win->state->egl_display, win->egl_surface, win->egl_surface, win->state->egl_context);
    glViewport(0, 0, win->width, win->height);
    switch (win->state->clear_color) {
        case CLEAR_BLACK: {
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
        } break;
        case CLEAR_GREEN: {
            glClearColor(0.0, 0.42, 0.20, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
        } break;
        case CLEAR_RED: {
            glClearColor(0.33, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
        } break;
    }

    eglSwapBuffers(win->state->egl_display, win->egl_surface);
    eglMakeCurrent(win->state->egl_display, NULL, NULL, win->state->egl_context);
}
