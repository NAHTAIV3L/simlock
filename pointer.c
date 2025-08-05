#include "./state.h"

void wl_pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial,
                   struct wl_surface *surface, wl_fixed_t fixed_surface_x, wl_fixed_t fixed_surface_y) {
    wl_pointer_set_cursor(pointer, serial, NULL, 0, 0);
}

void wl_pointer_leave(void *data, struct wl_pointer *pointer,
                   uint32_t serial, struct wl_surface *surface) {
}

void wl_pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time,
                    wl_fixed_t fixed_surface_x, wl_fixed_t fixed_surface_y) {
}

void wl_pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial,
                    uint32_t time, uint32_t button, uint32_t button_state) {
}

void wl_pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time,
                  uint32_t axis, wl_fixed_t fixed_value) {
}

void wl_pointer_frame(void *data, struct wl_pointer *wl_pointer) {
}

void wl_pointer_axis_source(void *data, struct wl_pointer *wl_pointer, uint32_t axis_source) {
}

void wl_pointer_axis_stop(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis) {
}

void wl_pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete) {
}

void wl_pointer_axis_value120(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t value120) {
}

void wl_pointer_axis_relative_direction(void *data, struct wl_pointer *wl_pointer, uint32_t axis, uint32_t direction) {
}

struct wl_pointer_listener wl_pointer_listener = {
    .enter = wl_pointer_enter,
    .leave = wl_pointer_leave,
    .motion = wl_pointer_motion,
    .button = wl_pointer_button,
    .axis = wl_pointer_axis,
    .frame = wl_pointer_frame,
    .axis_source = wl_pointer_axis_source,
    .axis_stop = wl_pointer_axis_stop,
    .axis_discrete = wl_pointer_axis_discrete,
    .axis_value120 = wl_pointer_axis_value120,
    .axis_relative_direction = wl_pointer_axis_relative_direction,
};
