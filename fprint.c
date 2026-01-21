#include <string.h>
#include "fprint.h"
#include "array.h"

#define FPRINT "net.reactivated.Fprint"
#define FPRINT_DEVICE FPRINT".Device"
#define FPRINT_MANAGER FPRINT".Manager"
#define FPRINT_MANAGER_PATH "/net/reactivated/Fprint/Manager"
#define FPRINT_ERROR FPRINT".Error"
#define FPRINT_ERROR_PERMISSION_DENIED FPRINT_ERROR".PermissionDenied"
#define FPRINT_ERROR_ALREADY_IN_USE FPRINT_ERROR".AlreadyInUse"
#define FPRINT_ERROR_INTERNAL FPRINT_ERROR".Internal"

#define DBUS_INTERFACE "org.freedesktop.DBus"

bool dbus_fprint_init(client_state* state);

void print_and_clear_error(DBusError* err) {
    fprintf(stderr, "%s: %s\n", err->name, err->message);
    dbus_error_free(err);
}

DBusHandlerResult filter(DBusConnection *conn, DBusMessage *msg, void *data) {
    client_state* state = data;
    fprintf(stderr, "message\n");
    if (dbus_message_is_signal(msg, FPRINT_DEVICE, "VerifyStatus")) {

        DBusMessageIter iter;
        dbus_message_iter_init(msg, &iter);

        char* status = NULL;
        dbus_message_iter_get_basic(&iter, &status);
        fprintf(stderr, "fprintd: %s\n", status);
        if (!strcmp(status, "verify-no-match")) {
            pthread_mutex_unlock(&state->dbus_mutex);
        }
        else if (!strcmp(status, "verify-unknown-error")) {
            pthread_mutex_unlock(&state->dbus_mutex);
        }
        else if (!strcmp(status, "verify-match")) {
            pthread_mutex_unlock(&state->dbus_mutex);
            state->verify_status = VERIFY_MATCH;
        }
        else {
            fprintf(stderr, "fprintd: %s\n", status);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (dbus_message_is_signal(msg, FPRINT_DEVICE, "VerifyFingerSelected")) {
        DBusMessageIter iter;
        dbus_message_iter_init(msg, &iter);
        char* finger = NULL;
        dbus_message_iter_get_basic(&iter, &finger);
        fprintf(stderr, "fprintd: verifying: %s\n", finger);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

bool dbus_fprint_claim_device(client_state* state) {
    DBusMessage* msg = dbus_message_new_method_call(FPRINT, state->fprint_device, FPRINT_DEVICE, "Claim");

    DBusMessageIter msg_iter;
    dbus_message_iter_init_append(msg, &msg_iter);
    char* username = "";
    dbus_message_iter_append_basic(&msg_iter, DBUS_TYPE_STRING, &username);

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(state->conn, msg, DBUS_TIMEOUT_INFINITE, &state->err);
    if (dbus_error_is_set(&state->err)) {
        print_and_clear_error(&state->err);
        dbus_message_unref(msg);
        return false;
    }
    dbus_message_unref(msg);
    dbus_message_unref(reply);
    state->fprint_claimed = true;
    return true;
}

bool dbus_fprint_get_default_device(client_state* state) {
    DBusMessage *msg = dbus_message_new_method_call(FPRINT, FPRINT_MANAGER_PATH, FPRINT_MANAGER, "GetDefaultDevice");
    if (!msg) {
        fprintf(stderr, "Failed to get default fingerprint reader\n");
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(state->conn, msg, DBUS_TIMEOUT_INFINITE, &state->err);
    if (dbus_error_is_set(&state->err)) {
        print_and_clear_error(&state->err);
        dbus_message_unref(msg);
        return false;
    }

    DBusMessageIter reply_iter;
    dbus_message_iter_init(reply, &reply_iter);

    char* tmp_device;
    dbus_message_iter_get_basic(&reply_iter, &tmp_device);
    state->fprint_device = strdup(tmp_device);

    dbus_message_unref(msg);
    dbus_message_unref(reply);
    return true;
}

bool dbus_fprint_check_device(client_state* state) {
    DBusMessage *msg = dbus_message_new_method_call(FPRINT, FPRINT_MANAGER_PATH, FPRINT_MANAGER, "GetDevices");
    if (!msg) {
        fprintf(stderr, "Failed to get fingerprint reader devices\n");
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(state->conn, msg, DBUS_TIMEOUT_INFINITE, &state->err);
    if (dbus_error_is_set(&state->err)) {
        print_and_clear_error(&state->err);
        fprintf(stderr, "Failed to get fingerprint reader devices\n");
        dbus_message_unref(msg);
        return false;
    }

    DBusMessageIter reply_iter;
    dbus_message_iter_init(reply, &reply_iter);
    DBusMessageIter reply_sub_iter;
    dbus_message_iter_recurse(&reply_iter, &reply_sub_iter);
    for (int i = 0; i < dbus_message_iter_get_element_count(&reply_iter); i++) {
        char* device_object = NULL;
        dbus_message_iter_get_basic(&reply_sub_iter, &device_object);
        if (!strcmp(device_object, state->fprint_device)) {
            dbus_message_unref(reply);
            dbus_message_unref(msg);
            return true;
        }
        dbus_message_iter_next(&reply_sub_iter);
    }
    dbus_message_unref(reply);
    dbus_message_unref(msg);

    return false;
}

bool dbus_fprint_get_first_finger(client_state* state) {
    DBusMessage* msg = dbus_message_new_method_call(FPRINT, state->fprint_device, FPRINT_DEVICE, "ListEnrolledFingers");
    if (!msg) {
        fprintf(stderr, "Failed to get default fingerprint reader\n");
        return false;
    }

    DBusMessageIter msg_iter;
    dbus_message_iter_init_append(msg, &msg_iter);
    char* username = "";
    dbus_message_iter_append_basic(&msg_iter, DBUS_TYPE_STRING, &username);

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(state->conn, msg, DBUS_TIMEOUT_INFINITE, &state->err);
    if (dbus_error_is_set(&state->err)) {
        print_and_clear_error(&state->err);
        dbus_message_unref(msg);
        return false;
    }

    DBusMessageIter reply_iter;
    dbus_message_iter_init(reply, &reply_iter);
    DBusMessageIter reply_sub_iter;
    dbus_message_iter_recurse(&reply_iter, &reply_sub_iter);
    char* tmp_finger = NULL;
    if (dbus_message_iter_get_element_count(&reply_iter)) {
        dbus_message_iter_get_basic(&reply_sub_iter, &tmp_finger);
    }
    state->fprint_finger = strdup(tmp_finger);

    dbus_message_unref(reply);
    dbus_message_unref(msg);
    return true;
}

bool dbus_fprint_verify_start(client_state* state) {
    if (!dbus_fprint_check_device(state)) {
        if (!dbus_fprint_init(state)) {
            return false;
        }
    }
    pthread_mutex_lock(&state->dbus_mutex);
    DBusMessage* msg = dbus_message_new_method_call(FPRINT, state->fprint_device, FPRINT_DEVICE, "VerifyStart");
    if (!msg) {
        fprintf(stderr, "Failed to start fingerprint veritfication\n");
        return false;
    }

    DBusMessageIter msg_iter;
    dbus_message_iter_init_append(msg, &msg_iter);
    dbus_message_iter_append_basic(&msg_iter, DBUS_TYPE_STRING, &(state->fprint_finger));

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(state->conn, msg, DBUS_TIMEOUT_INFINITE, &state->err);
    if (dbus_error_is_set(&state->err)) {
        print_and_clear_error(&state->err);
        dbus_message_unref(msg);
        return false;
    }
    dbus_message_unref(reply);
    dbus_message_unref(msg);
    state->fprint_verifying = true;
    return true;
}

bool dbus_fprint_verify_stop(client_state* state) {
    DBusMessage* msg = dbus_message_new_method_call(FPRINT, state->fprint_device, FPRINT_DEVICE, "VerifyStop");
    if (!msg) {
        fprintf(stderr, "Failed to stop verification\n");
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(state->conn, msg, DBUS_TIMEOUT_INFINITE, &state->err);
    if (dbus_error_is_set(&state->err)) {
        print_and_clear_error(&state->err);
        dbus_message_unref(msg);
        return false;
    }
    dbus_message_unref(reply);
    dbus_message_unref(msg);
    state->fprint_verifying = false;
    return true;
}

bool dbus_fprint_release_device(client_state* state) {
    DBusMessage* msg = dbus_message_new_method_call(FPRINT, state->fprint_device, FPRINT_DEVICE, "Release");
    if (!msg) {
        fprintf(stderr, "Failed to release device\n");
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(state->conn, msg, DBUS_TIMEOUT_INFINITE, &state->err);
    if (dbus_error_is_set(&state->err)) {
        print_and_clear_error(&state->err);
        dbus_message_unref(msg);
        return false;
    }
    dbus_message_unref(reply);
    dbus_message_unref(msg);
    state->fprint_claimed = false;
    return true;
}

bool dbus_check_for_fprintd(client_state* state) {
    DBusMessage* msg = dbus_message_new_method_call(DBUS_INTERFACE, "/org/freedesktop/Dbus", DBUS_INTERFACE, "ListNames");
    if (!msg) {
        fprintf(stderr, "Failed to list names on dbus\n");
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(state->conn, msg, DBUS_TIMEOUT_INFINITE, &state->err);
    if (dbus_error_is_set(&state->err)) {
        print_and_clear_error(&state->err);
        dbus_message_unref(msg);
        return false;
    }

    DBusMessageIter reply_iter;
    dbus_message_iter_init(reply, &reply_iter);
    DBusMessageIter reply_sub_iter;
    dbus_message_iter_recurse(&reply_iter, &reply_sub_iter);
    for (int i = 0; i < dbus_message_iter_get_element_count(&reply_iter); i++) {
        char* name = NULL;
        dbus_message_iter_get_basic(&reply_sub_iter, &name);
        if (!strcmp(name, FPRINT)) {
            dbus_message_unref(reply);
            dbus_message_unref(msg);
            return true;
        }
        dbus_message_iter_next(&reply_sub_iter);
    }
    dbus_message_unref(reply);
    dbus_message_unref(msg);
    return false;
}

bool dbus_fprint_init(client_state* state) {
    if (!dbus_check_for_fprintd(state)) {
        fprintf(stderr, "fprintd was not found on the system bus fingerprint auth is disabled\n");
        return false;
    }

    if (!dbus_fprint_get_default_device(state)) {
        fprintf(stderr, "No default device\n");
        return false;
    }

    if (!dbus_fprint_get_first_finger(state)) {
        fprintf(stderr, "no fingers enrolled\n");
        return false;
    }
    return true;
}

dbus_bool_t add_watch(DBusWatch* watch, void* data) {
    client_state* state = data;
    watch_t w;
    w.watch = watch;
    w.events = dbus_watch_get_flags(watch);
    w.fd = dbus_watch_get_unix_fd(watch);
    array_add(state->watches, w);
    printf("watch added: %p, %x, %d\n", w.watch, w.events, w.fd);
    return true;
}

void remove_watch(DBusWatch* watch, void* data) {
    client_state* state = data;
    for (int i = 0; i < array_size(state->watches); i++) {
        if (state->watches[i].watch == watch) {
            array_erase(state->watches, i);
        }
    }
}

void toggle_watch(DBusWatch* watch, void* data) {
    client_state* state = data;
    bool found = false;
    for (int i = 0; i < array_size(state->watches); i++) {
        if (state->watches[i].watch == watch) {
            array_erase(state->watches, i);
            found = true;
        }
    }
    if (!found) {
        add_watch(watch, data);
    }
}

dbus_bool_t add_timeout(DBusTimeout* timeout, void* data) {
    return true;
}

void remove_timeout(DBusTimeout* timeout, void* data) {
}

void toggle_timeout(DBusTimeout* timeout, void* data) {
}

bool dbus_init_1(client_state* state) {
    dbus_error_init(&state->err);
    state->conn = dbus_bus_get(DBUS_BUS_SYSTEM, &state->err);
    if (dbus_error_is_set(&state->err)) {
        print_and_clear_error(&state->err);
        return false;
    }

    if (!dbus_fprint_init(state)) {
        return false;
    }

    dbus_connection_add_filter(state->conn, filter, state, NULL);
    dbus_bus_add_match(state->conn, "type='signal'", &state->err);

    dbus_connection_set_watch_functions(state->conn, add_watch, remove_watch, toggle_watch, state, NULL);
    dbus_connection_set_timeout_functions(state->conn, add_timeout, remove_timeout, toggle_timeout, state, NULL);

    dbus_connection_flush(state->conn);
    state->dbus = true;
    return true;
}

void dbus_deinit(client_state* state) {
    dbus_connection_flush(state->conn);

    if (state->fprint_verifying) {
        dbus_fprint_verify_stop(state);
    }

    if (state->fprint_claimed) {
        dbus_fprint_release_device(state);
    }

    dbus_connection_unref(state->conn);
    dbus_error_free(&state->err);
}

bool verify_fingerprint(client_state* state) {
    if (!dbus_fprint_init(state)) {
        return false;
    }
    fprintf(stderr, "device: %s\nfinger: %s\n", state->fprint_device, state->fprint_finger);

    if (!state->fprint_claimed) {
        if (!dbus_fprint_claim_device(state)) {
            fprintf(stderr, "failed to claim device\n");
            return false;
        }
    }
    fprintf(stderr, "claimed device\n");

    if (!state->fprint_verifying) {
        if (!dbus_fprint_verify_start(state)) {
            fprintf(stderr, "failed to start verify\n");
            return false;
        }
        dbus_connection_flush(state->conn);
    }
    fprintf(stderr, "started verification\n");

    pthread_mutex_lock(&state->dbus_mutex);
    pthread_mutex_unlock(&state->dbus_mutex);

    if (state->fprint_verifying) {
        if (!dbus_fprint_verify_stop(state)) {
            fprintf(stderr, "failed to stop verify\n");
            return false;
        }
    }

    if (state->fprint_claimed) {
        if (!dbus_fprint_release_device(state)) {
            fprintf(stderr, "failed to release device\n");
            return false;
        }
    }
    fprintf(stderr, "released device\n");
    return true;

}

void* dbus_thread(void* data) {
    client_state *state = data;
    for (int i = 0; i < 3; i++) {
        verify_fingerprint(state);
        if (state->verify_status == VERIFY_MATCH) {
            state->run_unlock = true;
            break;
        }
        state->clear_color = CLEAR_RED;
    }
    return NULL;
}

void start_dbus_thread(client_state* state) {
    fprintf(stderr, "Running fprintd auth\n");
    pthread_create(&state->dbus_thread, NULL, dbus_thread, state);
    pthread_detach(state->dbus_thread);
}
