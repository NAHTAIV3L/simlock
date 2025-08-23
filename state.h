#ifndef STATE_H_
#define STATE_H_
#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/timerfd.h>
#include "./window.h"
#include <ext-session-lock-v1.h>
#define CLEAR_BLACK 0
#define CLEAR_GREEN 1
#define CLEAR_RED 2

typedef struct _client_state {
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_seat* seat;
    struct wl_keyboard* keyboard;
    struct wl_pointer* pointer;
    struct wl_shm* shm;

    struct ext_session_lock_manager_v1* ext_session_lock_manager;
    struct ext_session_lock_v1* ext_session_lock;

    window_t* windows;
    uint32_t num_windows;

    struct wl_surface* cursor_surface;
    struct wl_cursor_image* cursor_image;
    struct wl_cursor_theme* cursor_theme;
    struct wl_buffer* cursor_buffer;
    struct wl_cursor* cursor;

    struct xkb_context* xkb_context;
    struct xkb_keymap* xkb_keymap;
    struct xkb_state* xkb_state;

    EGLDisplay egl_display;
    EGLConfig egl_config;
    EGLContext egl_context;

    bool running, opengl_initalized, locked, run_unlock;
    char* buffer;
    uint32_t buffer_len;

    pthread_mutex_t input_lock1, input_lock2;
    int key_repeat_timer_fd;
    xkb_keysym_t repeat_keysym;
    int key_repeat_rate, key_repeat_delay;

    int clear_color;

} client_state;

#endif // STATE_H_
