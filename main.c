#include "state.h"
#include <EGL/eglext.h>
#include <string.h>
#include <unistd.h>

extern struct wl_registry_listener wl_registry_listener;
extern struct ext_session_lock_v1_listener ext_session_lock_listener;
extern struct ext_session_lock_surface_v1_listener ext_session_lock_surface_listener;

PFNEGLGETPLATFORMDISPLAYPROC eglGetPlatformDisplayEXT = NULL;
PFNEGLCREATEPLATFORMWINDOWSURFACEPROC eglCreatePlatformWindowSurfaceEXT = NULL;

bool initalize_egl(client_state *state) {

    EGLint attrs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_CONFORMANT, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };

    const char* exts = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (!exts) {
        if (eglGetError() == EGL_BAD_DISPLAY) {
            fprintf(stderr, "EGL_EXT_client_extensions not supported\n");
        }
        else {
            fprintf(stderr, "Failed to query EGL extensions\n");
        }
        return false;
    }

    if (!strstr(exts, "EGL_EXT_platform_base")) {
        fprintf(stderr, "EGL_EXT_platform_base not supported\n");
        return false;
    }
    if (!strstr(exts, "EGL_EXT_platform_wayland")) {
        fprintf(stderr, "EGL_EXT_platform_wayland not supported\n");
        return false;
    }

    eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (!eglGetPlatformDisplayEXT) {
        fprintf(stderr, "Failed to get eglGetPlatformDisplayEXT\n");
        return false;
    }
    eglCreatePlatformWindowSurfaceEXT = (PFNEGLCREATEPLATFORMWINDOWSURFACEPROC)eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT");
    if (!eglCreatePlatformWindowSurfaceEXT) {
        fprintf(stderr, "Failed to get eglCreatePlatformWindowSurfaceEXT\n");
        return false;
    }

    state->egl_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_WAYLAND_EXT, state->display, NULL);
    if (state->egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "Failed to get EGL display\n");
        return false;
    }

    if (!eglInitialize(state->egl_display, NULL, NULL)) {
        fprintf(stderr, "Failed to initalize EGL\n");
        return false;
    }
    eglBindAPI(EGL_OPENGL_API);

    EGLint num_configs;
    if (!eglChooseConfig(state->egl_display, attrs, &state->egl_config, 1, &num_configs)) {
        fprintf(stderr, "Failed to choose EGL config\n");
        return false;
    }
    if (num_configs == 0) {
        fprintf(stderr, "Failed to find any valid EGL configs\n");
        return false;
    }

    state->egl_context = eglCreateContext(state->egl_display, state->egl_config, EGL_NO_CONTEXT, NULL);
    if (state->egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "Failed to create EGL context\n");
        return false;
    }

    return true;
}

int main() {

    client_state state = { 0 };
    state.running = true;

    state.display = wl_display_connect(NULL);
    if (!state.display) {
        fprintf(stderr, "Failed to connect to wayland compositor. Exiting\n");
        return 1;
    }

    state.registry = wl_display_get_registry(state.display);
    wl_registry_add_listener(state.registry, &wl_registry_listener, &state);
    wl_display_roundtrip(state.display);

    if (!initalize_egl(&state)) {
        fprintf(stderr, "Failed to initalize EGL. Exiting\n");
        return 1;
    }

    if (!state.ext_session_lock_manager) {
        fprintf(stderr, "Failed to get ext_session_lock_manager. Exiting\n");
        return 1;
    }

    state.ext_session_lock = ext_session_lock_manager_v1_lock(state.ext_session_lock_manager);
    ext_session_lock_v1_add_listener(state.ext_session_lock, &ext_session_lock_listener, &state);
    wl_display_roundtrip(state.display);

    for (int i = 0; i < state.num_windows; i++) {
        window_t* win = &state.windows[i];
        win->surface = wl_compositor_create_surface(state.compositor);
        win->ext_session_lock_surface = ext_session_lock_v1_get_lock_surface(state.ext_session_lock, win->surface, win->output);
        ext_session_lock_surface_v1_add_listener(win->ext_session_lock_surface, &ext_session_lock_surface_listener, &state);
        wl_display_roundtrip(state.display);
    }

    while (state.running) {
        wl_display_dispatch_pending(state.display);
        wl_display_flush(state.display);
        for (int i = 0; i < state.num_windows; i++) {
            window_redraw(&state.windows[i]);
        }
    }

    ext_session_lock_v1_unlock_and_destroy(state.ext_session_lock);
    wl_display_roundtrip(state.display);
    printf("Unlocking session\n");

    return 0;
}
