#include <pam_appl.h>
#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./state.h"

char* get_input(client_state *state) {
    pthread_mutex_lock(&state->input_lock2);
    pthread_mutex_lock(&state->input_lock1);
    pthread_mutex_unlock(&state->input_lock2);
    char* str = strdup(state->buffer);
    pthread_mutex_unlock(&state->input_lock1);
    return str;
}

int conv(int num_msg, const struct pam_message **msg, struct pam_response **resp,
         void *appdata_ptr) {
    client_state *state = appdata_ptr;
    struct pam_response* response = calloc(num_msg, sizeof(struct pam_response));

    for (int i = 0; i < num_msg; i++) {
        switch (msg[i]->msg_style) {
            case PAM_PROMPT_ECHO_OFF:
            case PAM_PROMPT_ECHO_ON: {
                if (!state->locked) {
                    return PAM_CONV_ERR;
                }
                char* input = get_input(state);
                response->resp = input;
            } break;
            case PAM_ERROR_MSG: {
                printf("%s\n", msg[i]->msg);
            }
            case PAM_TEXT_INFO: {
                printf("%s\n", msg[i]->msg);
            }
        }
    }

    *resp = response;
    return PAM_SUCCESS;
}

bool pam_auth(client_state* state) {
    pam_handle_t* pam_handle = NULL;
    struct passwd *passwd_info = getpwuid(getuid());
    const struct pam_conv conversation = { .conv = conv, .appdata_ptr = state };
    if (!passwd_info || !passwd_info->pw_name) {
        fprintf(stderr, "Failed to get username\n");
        return false;
    }

    int retval = 0;
    retval = pam_start("hyprland", passwd_info->pw_name, &conversation, &pam_handle);

    if (retval != PAM_SUCCESS)
        return false;

    retval = pam_authenticate(pam_handle, 0);
    pam_end(pam_handle, retval);
    pam_handle = NULL;
    return (retval == PAM_SUCCESS);
}

void *pam_thread(void *data) {
    client_state *state = data;
    while (1) {
        if (!state->locked) {
            return NULL;
        }

        bool auth = pam_auth(state);

        if (!state->locked) {
            return NULL;
        }
        if (!auth) {
            printf("Authentication Failed\n");
            state->clear_color = CLEAR_RED;
        }
        else {
            printf("Authenticated\n");
            state->run_unlock = true;
            return NULL;
        }
    }
    return NULL;
}

void start_pam(client_state* state) {
    pthread_t thread;
    pthread_create(&thread, NULL, pam_thread, state);
    pthread_detach(thread);
}

