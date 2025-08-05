#include "state.h"

extern struct wl_pointer_listener wl_pointer_listener;
extern struct wl_keyboard_listener wl_keyboard_listener;

void wl_seat_capabilities(void *data, struct wl_seat *wl_seat,
                          uint32_t capabilities) {
    client_state* state = data;
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        state->keyboard = wl_seat_get_keyboard(wl_seat);
        wl_keyboard_add_listener(state->keyboard, &wl_keyboard_listener, state);
    }
    if (capabilities & WL_SEAT_CAPABILITY_POINTER && !state->pointer) {
        state->pointer = wl_seat_get_pointer(wl_seat);
        wl_pointer_add_listener(state->pointer, &wl_pointer_listener, state);
        // state->cursor_theme = wl_cursor_theme_load(NULL, 24, state->shm);
        // state->cursor = wl_cursor_theme_get_cursor(state->cursor_theme, "left_ptr");
        // state->cursor_image = state->cursor->images[0];
        // state->cursor_buffer = wl_cursor_image_get_buffer(state->cursor_image);
        // state->cursor_surface = wl_compositor_create_surface(state->compositor);
        // wl_surface_attach(state->cursor_surface, state->cursor_buffer, 0, 0);
        // wl_surface_commit(state->cursor_surface);
    }
}

void wl_seat_name(void *data, struct wl_seat *wl_seat, const char *name) {
}

struct wl_seat_listener wl_seat_listener = {
    .capabilities = wl_seat_capabilities,
    .name = wl_seat_name,
};
