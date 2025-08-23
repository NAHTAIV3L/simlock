#include "state.h"
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include "./util.h"
#include <string.h>
#include <wayland-client-protocol.h>

void wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard,
                        uint32_t format, int32_t fd, uint32_t size) {
    client_state* state = data;

    assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

    char* map_shm = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(map_shm != MAP_FAILED && "map_shm failed");

    state->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(state->xkb_context && "Failed to create xkb context");

    state->xkb_keymap = xkb_keymap_new_from_string(state->xkb_context, map_shm,
                                                  XKB_KEYMAP_FORMAT_TEXT_V1,
                                                  XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_shm, size);
    close(fd);
    state->xkb_state = xkb_state_new(state->xkb_keymap);
    assert(state->xkb_state && "Failed to create state");
}

void wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard,
                       uint32_t serial, struct wl_surface *surface,
                       struct wl_array *keys) {
}

void wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard,
                       uint32_t serial, struct wl_surface *surface) {
}

void wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t key_state) {
    client_state* state = data;
    xkb_keysym_t keysym = xkb_state_key_get_one_sym(state->xkb_state, key + 8);
    handle_keypress(state, keysym, key_state);
}

void wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard,
                           uint32_t serial, uint32_t mods_depressed,
                           uint32_t mods_latched, uint32_t mods_locked,
                           uint32_t group) {
    client_state* state = data;
    xkb_state_update_mask(state->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}

void wl_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
                             int32_t rate, int32_t delay) {
    client_state* state = data;
    state->key_repeat_rate = rate;
    state->key_repeat_delay = delay;
}

struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap = wl_keyboard_keymap,
    .enter = wl_keyboard_enter,
    .leave = wl_keyboard_leave,
    .key = wl_keyboard_key,
    .modifiers = wl_keyboard_modifiers,
    .repeat_info = wl_keyboard_repeat_info,
};
