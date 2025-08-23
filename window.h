#ifndef WINDOW_H_
#define WINDOW_H_

#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>
#include <stdbool.h>
#include <ext-session-lock-v1.h>

typedef struct _client_state client_state;

typedef struct {
    client_state* state;
    struct wl_output* output;
    struct wl_surface* surface;
    struct ext_session_lock_surface_v1* ext_session_lock_surface;
    struct wl_egl_window* egl_window;
    EGLSurface egl_surface;
    uint32_t width, height;
    uint32_t name;
} window_t;

void window_init(window_t *win);
void window_destroy(window_t *win);

void window_redraw(window_t* win);

#endif  // WINDOW_H_
