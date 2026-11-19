#ifndef FPRINT_H_
#define FPRINT_H_
#include "state.h"

bool dbus_init(client_state* state);
void dbus_deinit(client_state* state);

bool fprint_start_verify(client_state* state);
bool fprint_stop_verify(client_state* state);
#endif // FPRINT_H_
