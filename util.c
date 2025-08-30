#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include "array.h"
#include "util.h"

void redraw(client_state *state) {
    if (!state->locked) return;
    for (int i = 0; i < array_size(state->windows); i++) {
        window_redraw(&state->windows[i]);
    }
}

void unlock(client_state *state) {
    state->locked = false;

    ext_session_lock_v1_unlock_and_destroy(state->ext_session_lock);
    wl_display_roundtrip(state->display);

    for (int i = 0; i < array_size(state->windows); i++) {
        window_destroy(&state->windows[i]);
    }
    array_free(state->windows);

    state->running = false;
}

void start_key_repeat_timer(client_state* state, xkb_keysym_t keysym) {
    struct itimerspec timer = {0};
    state->repeat_keysym = keysym;
    if (state->key_repeat_rate > 1) {
        timer.it_interval.tv_nsec = 1000000000 / state->key_repeat_rate;
    }
    else {
        timer.it_interval.tv_sec = 1;
    }
    timer.it_value.tv_sec = state->key_repeat_delay / 1000;
    timer.it_value.tv_nsec = (state->key_repeat_delay % 1000) * 1000000;
    if (timerfd_settime(state->key_repeat_timer_fd, 0, &timer, NULL) == -1) {
        printf("error setting key repeat timer\n");
    }
}

void handle_keypress(client_state* state, xkb_keysym_t keysym, uint32_t key_state) {
    if (key_state == WL_KEYBOARD_KEY_STATE_RELEASED) {
        if (keysym == state->repeat_keysym) {
            struct itimerspec timer = {0};
            timerfd_settime(state->key_repeat_timer_fd, 0, &timer, NULL);
        }
        if (state->xkb_compose_state && xkb_compose_state_get_status(state->xkb_compose_state) == XKB_COMPOSE_COMPOSED) {
            xkb_compose_state_reset(state->xkb_compose_state);
        }
        return;
    }
    bool ctrl = xkb_state_mod_name_is_active(state->xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE);

    enum xkb_compose_status compose_status = XKB_COMPOSE_NOTHING;
    if (state->xkb_compose_state) {
        xkb_compose_state_feed(state->xkb_compose_state, keysym);
        compose_status = xkb_compose_state_get_status(state->xkb_compose_state);
    }

    if (keysym == XKB_KEY_BackSpace && ctrl) {
        array_free(state->buffer);
        state->clear_color = CLEAR_BLACK;
    }
    else if (keysym == XKB_KEY_BackSpace) {
        if (state->buffer) {
            array_pop(state->buffer);
        }
        if (!array_size(state->buffer)) {
            state->clear_color = CLEAR_BLACK;
        }
        if (key_state == WL_KEYBOARD_KEY_STATE_PRESSED && state->repeat_keysym != keysym) {
            start_key_repeat_timer(state, keysym);
        }
    }
    else if (keysym == XKB_KEY_Return || keysym == XKB_KEY_KP_Enter) {
        if (state->buffer) {
            printf("Authenticating\n");
            pthread_mutex_unlock(&state->input_lock1);
            pthread_mutex_lock(&state->input_lock2);
            pthread_mutex_lock(&state->input_lock1);
            pthread_mutex_unlock(&state->input_lock2);
            array_clear(state->buffer);
        }
    }
    else {
        char buf[16] = { 0 };
        int len = (compose_status == XKB_COMPOSE_COMPOSED) ?
            xkb_compose_state_get_utf8(state->xkb_compose_state, buf, sizeof(buf))
            : xkb_keysym_to_utf8(keysym, buf, sizeof(buf));
        if (len > 0) {
            for (int i = 0; i < strlen(buf); i++) {
                array_add(state->buffer, buf[i]);
            }
            state->clear_color = CLEAR_GREEN;
        }
    }
}

void poll_events(client_state* state) {
    enum { DISPLAY_FD, KEYREPEAT_FD };
    struct pollfd fds[] = {
        [DISPLAY_FD] = { wl_display_get_fd(state->display), POLLIN },
        [KEYREPEAT_FD] = { state->key_repeat_timer_fd, POLLIN },
    };

    bool event = false;
    while (!event) {
        while (wl_display_prepare_read(state->display) != 0) {
            if (wl_display_dispatch_pending(state->display) > 0) {
                return;
            }
        }
        if (poll(fds, sizeof(fds) / sizeof(fds[0]), -1) == -1) {
            wl_display_cancel_read(state->display);
            return;
        }
        if (fds[DISPLAY_FD].revents & POLLIN) {
            wl_display_read_events(state->display);
            if (wl_display_dispatch_pending(state->display) > 0)
                event = true;
        }
        else {
            wl_display_cancel_read(state->display);
        }
        if (fds[KEYREPEAT_FD].revents & POLLIN) {
            uint64_t repeats;
            if (read(state->key_repeat_timer_fd, &repeats, sizeof(repeats)) == 8) {
                for (uint64_t i = 0; i < repeats; i++) {
                    handle_keypress(state, state->repeat_keysym, WL_KEYBOARD_KEY_STATE_PRESSED);
                }
                event = true;
            }
        }
    }
}
