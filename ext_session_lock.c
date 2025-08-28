#include <stdlib.h>
#include "state.h"

void ext_session_lock_locked(void *data, struct ext_session_lock_v1 *ext_session_lock_v1) {
    client_state *state = data;
    state->locked = true;
    printf("Locking session\n");
}

void ext_session_lock_finished(void *data, struct ext_session_lock_v1 *ext_session_lock_v1) {
    fprintf(stderr, "Session is already locked?\n");
    exit(1);
}

struct ext_session_lock_v1_listener ext_session_lock_listener = {
	.locked = ext_session_lock_locked,
	.finished = ext_session_lock_finished,
};
