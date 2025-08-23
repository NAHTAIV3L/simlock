#include "./state.h"
#include "./window.h"
#include "./glad/glad.h"
#include <stdlib.h>

extern PFNEGLCREATEPLATFORMWINDOWSURFACEPROC eglCreatePlatformWindowSurfaceEXT;

void ext_session_lock_surface_configure(void *data, struct ext_session_lock_surface_v1 *ext_session_lock_surface_v1, uint32_t serial, uint32_t width, uint32_t height) {
    window_t *win = data;
    ext_session_lock_surface_v1_ack_configure(ext_session_lock_surface_v1, serial);

    win->width = width;
    win->height = height;
    if (!win->egl_window) {
        window_init(win);
        if (!win->state->opengl_initalized) {
            eglMakeCurrent(win->state->egl_display, win->egl_surface,
                           win->egl_surface, win->state->egl_context);
            if (!gladLoadGL()) {
                fprintf(stderr, "Failed to initalize opengl\n");
                exit(1);
            }
            eglMakeCurrent(win->state->egl_display, NULL,
                           NULL, win->state->egl_context);
        }
    }
    else {
        wl_egl_window_resize(win->egl_window, win->width, win->height, 0, 0);
    }
    window_redraw(win);

}

struct ext_session_lock_surface_v1_listener ext_session_lock_surface_listener = {
        .configure = ext_session_lock_surface_configure,
};
