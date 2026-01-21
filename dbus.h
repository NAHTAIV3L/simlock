#ifndef DBUS_H_
#define DBUS_H_
#include <stdbool.h>

typedef struct _dbus_connection dbus_connection;
typedef struct _dbus_message dbus_message;

dbus_connection* dbus_connection_init(bool system);

void dbus_message_free(dbus_message* msg);
dbus_message* dbus_message_create_method_call(const char* dest, const char* path, const char* iface, const char* method);
bool dbus_message_serialize(dbus_message* msg);

#endif // DBUS_H_
