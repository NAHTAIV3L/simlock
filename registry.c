#include <string.h>
#include "state.h"
#include "array.h"

extern struct wl_seat_listener wl_seat_listener;
extern struct ext_session_lock_surface_v1_listener ext_session_lock_surface_listener;

void wl_registry_global(void *data, struct wl_registry *wl_registry,
                        uint32_t name, const char *interface,
                        uint32_t version) {
    client_state *state = data;

    printf("iface: %s v%d\n", interface, version);

    if (!strcmp(interface, wl_compositor_interface.name)) {
        state->compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
        printf(" > bound to %s v%d\n", interface, version);
    }
    else if (!strcmp(interface, wl_shm_interface.name)) {
        state->shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, version);
    }
    else if (!strcmp(interface, wl_seat_interface.name)) {
        state->seat = wl_registry_bind(wl_registry, name, &wl_seat_interface, version);
        wl_seat_add_listener(state->seat, &wl_seat_listener, state);
        printf(" > bound to %s v%d\n", interface, version);
    }
    else if (!strcmp(interface, wl_output_interface.name)) {
        array_add(state->windows, (window_t){ 0 });
        window_t* win = &state->windows[array_size(state->windows) - 1];
        win->state = state;
        win->output = wl_registry_bind(wl_registry, name, &wl_output_interface, version);
        if (state->locked) {
            win->surface = wl_compositor_create_surface(state->compositor);
            win->ext_session_lock_surface = ext_session_lock_v1_get_lock_surface(state->ext_session_lock, win->surface, win->output);
            ext_session_lock_surface_v1_add_listener(win->ext_session_lock_surface, &ext_session_lock_surface_listener, &state);
            wl_display_roundtrip(state->display);
        }
        win->name = name;
        printf(" > bound to %s v%d\n", interface, version);
    }
    else if (!strcmp(interface, ext_session_lock_manager_v1_interface.name)) {
        state->ext_session_lock_manager = wl_registry_bind(wl_registry, name, &ext_session_lock_manager_v1_interface, version);
        printf(" > bound to %s v%d\n", interface, version);
    }
}

void wl_registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {
    client_state *state = data;
    for (int i = 0; i < array_size(state->windows); i++) {
        if (state->windows[i].name == name) {
            window_destroy(&state->windows[i]);
            state->windows[i] = state->windows[array_size(state->windows) - 1];
            array_pop(state->windows);
        }
    }
}

struct wl_registry_listener wl_registry_listener = {
    .global = wl_registry_global,
    .global_remove = wl_registry_global_remove,
};
