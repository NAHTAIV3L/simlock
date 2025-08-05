#include "./state.h"
#include "./window.h"
#include "./glad/glad.h"
#include <assert.h>
#include <stdlib.h>

extern PFNEGLCREATEPLATFORMWINDOWSURFACEPROC eglCreatePlatformWindowSurfaceEXT;

void ext_session_lock_surface_configure(void *data, struct ext_session_lock_surface_v1 *ext_session_lock_surface_v1, uint32_t serial, uint32_t width, uint32_t height) {
    client_state *state = data;
    window_t *win = NULL;
    for (int i = 0; i < state->num_windows; i++) {
        if (state->windows[i].ext_session_lock_surface == ext_session_lock_surface_v1) {
            win = &state->windows[i];
            break;
        }
    }
    ext_session_lock_surface_v1_ack_configure(ext_session_lock_surface_v1, serial);

    win->width = width;
    win->height = height;

    if(!win->egl_window) {
        win->egl_window = wl_egl_window_create(win->surface, win->width, win->height);
        assert(win->egl_window && "Failed to create EGL window");
    }
    else {
        wl_egl_window_resize(win->egl_window, win->width, win->height, 0, 0);
    }

    if (!win->egl_surface) {
        win->egl_surface = eglCreatePlatformWindowSurfaceEXT(win->state->egl_display,
                                                             win->state->egl_config,
                                                             win->egl_window, NULL);
        assert(win->egl_surface && "Failed to create EGL surface");
    }
    if (!state->opengl_initalized) {
        eglMakeCurrent(state->egl_display, win->egl_surface,
                       win->egl_surface, state->egl_context);
        if (!gladLoadGL()) {
            fprintf(stderr, "Failed to initalize opengl\n");
            exit(1);
        }
        eglMakeCurrent(state->egl_display, NULL,
                       NULL, state->egl_context);
    }
}

struct ext_session_lock_surface_v1_listener ext_session_lock_surface_listener = {
        .configure = ext_session_lock_surface_configure,
};
