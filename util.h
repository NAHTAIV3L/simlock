#ifndef UTIL_H_
#define UTIL_H_
#include <stdint.h>
#include "state.h"

char* append_to_string(char* str, char* append);

void redraw(client_state *state);
void unlock(client_state *state);
void handle_keypress(client_state* state, xkb_keysym_t keysym, uint32_t key_state);
void poll_events(client_state *state);

#endif // UTIL_H_
