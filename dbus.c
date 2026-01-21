#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdint.h>

#include "array.h"
#include "dbus.h"

#define ALIGN(array, alignment_counter, align_to) do {  \
    while ((alignment_counter) % (align_to)) {          \
        array_add((array), 0);                            \
        (alignment_counter)++;                          \
    }                                                   \
} while (0)

#define ALIGNI(index, value) do { \
    index += index % value ? value - (index % value) : 0; \
} while(0)

typedef struct _dbus_connection {
    int fd;
    char* unique_name;
} dbus_connection;


typedef enum {
    DBUS_MESSAGE_TYPE_INVALID = 0,
    DBUS_MESSAGE_TYPE_METHOD_CALL,
    DBUS_MESSAGE_TYPE_METHOD_RETURN,
    DBUS_MESSAGE_TYPE_ERROR,
    DBUS_MESSAGE_TYPE_SIGNAL,
} dbus_message_type_t;

typedef enum {
    DBUS_HEADER_FIELD_INVALID = 0,
    DBUS_HEADER_FIELD_PATH,
    DBUS_HEADER_FIELD_INTERFACE,
    DBUS_HEADER_FIELD_MEMBER,
    DBUS_HEADER_FIELD_ERROR_NAME,
    DBUS_HEADER_FIELD_REPLY_SERIAL,
    DBUS_HEADER_FIELD_DESTINATION,
    DBUS_HEADER_FIELD_SENDER,
    DBUS_HEADER_FIELD_SIGNATURE,
    DBUS_HEADER_FIELD_UNIX_FDS,
} dbus_header_field_type;

typedef enum {
    DBUS_TYPE_INVALID = 0,
    DBUS_TYPE_BYTE = 'y',
    DBUS_TYPE_BOOLEAN = 'b',
    DBUS_TYPE_INT16 = 'n',
    DBUS_TYPE_UINT16 = 'q',
    DBUS_TYPE_INT32 = 'i',
    DBUS_TYPE_UINT32 = 'u',
    DBUS_TYPE_INT64 = 'x',
    DBUS_TYPE_UINT64 = 't',
    DBUS_TYPE_DOUBLE = 'd',
    DBUS_TYPE_STRING = 's',
    DBUS_TYPE_OBJECT_PATH = 'o',
    DBUS_TYPE_SIGNATURE = 'g',
    DBUS_TYPE_ARRAY = 'a',
    DBUS_TYPE_STRUCT = 'r',
    DBUS_TYPE_VARIANT = 'v',
} dbus_type;

static const char* header_field_signatures[] = {
    [DBUS_HEADER_FIELD_INVALID] = NULL,
    [DBUS_HEADER_FIELD_PATH] = "o",
    [DBUS_HEADER_FIELD_INTERFACE] = "s",
    [DBUS_HEADER_FIELD_MEMBER] = "s",
    [DBUS_HEADER_FIELD_ERROR_NAME] = "s",
    [DBUS_HEADER_FIELD_REPLY_SERIAL] = "u",
    [DBUS_HEADER_FIELD_DESTINATION] = "s",
    [DBUS_HEADER_FIELD_SENDER] = "s",
    [DBUS_HEADER_FIELD_SIGNATURE] = "g",
    [DBUS_HEADER_FIELD_UNIX_FDS] = "u",
};

typedef struct {
    dbus_header_field_type type;
    union {
        char* path;
        char* interface;
        char* member;
        char* error_name;
        uint32_t reply_serial;
        char* destination;
        char* sender;
        char* signature;
        uint32_t unix_fds;
    };
} dbus_header_field;

typedef struct {
    char endianness;
    uint8_t message_type;
    uint8_t flags;
    uint8_t major_protocol_version;
    uint32_t message_body_length;
    uint32_t serial;
    dbus_header_field* header_fields;
} dbus_header;

typedef struct _dbus_message {
    char* header;
    char* body;
} dbus_message;

dbus_header* dbus_parse_input_buffer(char** buf);

dbus_connection* dbus_connection_init(bool system) {
    char* path = "/var/run/dbus/system_bus_socket";
    if (!system) {
        return NULL;
    }
    dbus_connection* conn = malloc(sizeof(dbus_connection));
    if (!conn) {
        goto error;
    }
    conn->fd = -1;
    conn->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (conn->fd == -1) {
        goto error;
    }
    struct sockaddr_un sockaddr = {
        .sun_family = AF_UNIX,
    };
    strcpy(sockaddr.sun_path, path);
    if (connect(conn->fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        goto error;
    }
    if (send(conn->fd, "\0", 1, 0) < 0) {
    }

    char buf[1024];
    strcpy(buf, "AUTH\r\n");
    if (send(conn->fd, buf, strlen(buf), 0) < 0) {
        fprintf(stderr, "Failed to send AUTH\n");
        goto error;
    }

    ssize_t bytes_read = recv(conn->fd, buf, sizeof(buf), 0);
    if (bytes_read < 0) {
        goto error;
    }

    if (!strstr(buf, "EXTERNAL")) {
        fprintf(stderr, "AUTH unimplemented: %.*s", (int)bytes_read, buf);
        goto error;
    }

    uid_t uid = getuid();

    {
        char tmp_buf[1024];
        snprintf(tmp_buf, sizeof(tmp_buf), "%u", uid);
        strcpy(buf, "AUTH EXTERNAL ");
        int j = strlen(buf);
        for (int i = 0; i < strlen(tmp_buf); i++) {
            snprintf(&buf[j], sizeof(tmp_buf) - j, "%hhx", tmp_buf[i]);
            j += 2;
        }
        if (j < sizeof(buf) - 3) {
            buf[j] = '\r';
            buf[j + 1] = '\n';
            buf[j + 2] = 0;
        }
        else {
            fprintf(stderr, "Why is your uid so long?\n");
        }
    }

    if (send(conn->fd, buf, strlen(buf), 0) < 0) {
        fprintf(stderr, "Failed to send %.*s\n", (int)strlen(buf) - 2, buf);
        goto error;
    }

    bytes_read = recv(conn->fd, buf, sizeof(buf), 0);
    if (bytes_read < 0) {
        goto error;
    }
    if (strncmp(buf, "OK", 2)) {
        fprintf(stderr, "%s", buf);
        goto error;
    }

    strcpy(buf, "BEGIN\r\n");
    if (send(conn->fd, buf, strlen(buf), 0) < 0) {
        fprintf(stderr, "Failed to BEGIN\n");
        goto error;
    }

    dbus_message* msg = dbus_message_create_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "Hello");

    if (send(conn->fd, msg->header, array_size(msg->header), 0) < 0) {
        goto error;
    }

    bytes_read = recv(conn->fd, buf, sizeof(buf), 0);
    char* buff = buf;
    dbus_header* header = dbus_parse_input_buffer(&buff);
    printf("body len: %u\n", header->message_body_length);
    array_for_all(dbus_header_field, hf, header->header_fields) {
        printf("type: %d\n", hf->type);
        if (hf->type == DBUS_HEADER_FIELD_REPLY_SERIAL || hf->type == DBUS_HEADER_FIELD_UNIX_FDS) {
            printf("%u\n", hf->reply_serial);
        }
        else {
            printf("%s\n", hf->signature);
        }
    }
    // printf("buff: %p\n", buff);
    // printf("buf: %p\n", buf);
    // printf("buff - buf: %lu\n", buff - buf);
    for (int i = 0; i < bytes_read - (buff - buf); i++) {
        if (isprint(buff[i])) {
            fprintf(stderr, "%c", buff[i]);
        }
        else {
            fprintf(stderr, "\\x%x", buff[i]);
        }
    }
    fprintf(stderr, "\n");

    dbus_message_free(msg);

    // return conn;
error:
    if (conn && conn->fd != -1) {
        close(conn->fd);
    }
    if (conn) free(conn);
    return NULL;
}

dbus_header* dbus_parse_input_buffer(char** buff) {
    char* buf = *buff;
    dbus_header* header = calloc(1, sizeof(*header));
    int i = 0;
    header->endianness = buf[i++];
    header->message_type = buf[i++];
    header->flags = buf[i++];
    header->major_protocol_version = buf[i++];
    header->message_body_length = *(uint32_t*)&(buf[i]);
    i += 4;
    header->serial = *(uint32_t*)&(buf[8]);
    i += 4;
    if (header->endianness != 'l') {
        fprintf(stderr, "Big endian not supported yet\n");
        exit(1);
    }
    switch (header->message_type) {
    case DBUS_MESSAGE_TYPE_INVALID:
            fprintf(stderr, "Invalid message type\n");
            exit(1);
        default: break;
    }
    header->header_fields = array_new(dbus_header_field, 0);
    ALIGNI(i, 4);
    uint32_t hf_len = *(uint32_t*)&(buf[i]);
    i += 4;
    int start_of_headers = i;
    while (i < hf_len + start_of_headers) {
        ALIGNI(i, 8);
        dbus_header_field_type t = buf[i++];

        if (t == DBUS_HEADER_FIELD_INVALID) {
            fprintf(stderr, "Invalid message type\n");
            exit(1);
        }

        uint8_t siglen = buf[i++];

        char* signature = &(buf[i]);
        if (strncmp(signature, header_field_signatures[t], siglen)) {
            fprintf(stderr, "hf signature does not match expected\n");
            exit(1);
        }
        i += siglen + 1;
        ALIGNI(i, 4);

        switch (t) {
            case DBUS_HEADER_FIELD_INVALID: break;

            case DBUS_HEADER_FIELD_PATH:
            case DBUS_HEADER_FIELD_INTERFACE:
            case DBUS_HEADER_FIELD_MEMBER:
            case DBUS_HEADER_FIELD_ERROR_NAME:
            case DBUS_HEADER_FIELD_DESTINATION:
            case DBUS_HEADER_FIELD_SIGNATURE:
            case DBUS_HEADER_FIELD_SENDER: {
                uint32_t vallen = 0;
                if (t == DBUS_HEADER_FIELD_SIGNATURE) {
                    vallen = *(uint8_t*)&(buf[i]);
                    i += 1;
                }
                else {
                    vallen = *(uint32_t*)&(buf[i]);
                    i += 4;
                }
                char* val = &(buf[i]);
                dbus_header_field hf = {
                    .type = t,
                };
                hf.path = strdup(val);
                array_add(header->header_fields, hf);
                i += vallen + 1;
                break;
            }
            case DBUS_HEADER_FIELD_UNIX_FDS:
            case DBUS_HEADER_FIELD_REPLY_SERIAL: {
                uint32_t val = *(uint32_t*)&(buf[i]);
                dbus_header_field hf = {
                    .type = t,
                    .reply_serial = val,
                };
                array_add(header->header_fields, hf);
                i += 4;
            }
        }
    }
    ALIGNI(i, 8);
    // i += header->message_body_length;
    *buff = &(buf[i]);
    return header;
}

void dbus_message_free(dbus_message* msg) {
    if (!msg) return;
    if (msg->header) array_free(msg->header);
    free(msg);
}

void dbus_hf_add_value(char** str, uint32_t* alignment, uint8_t field, const char* value) {
    ALIGN(*str, *alignment, 8);
    array_add(*str, field);
    *alignment += 1;
    const char* sig = header_field_signatures[field];
    array_add(*str, (char)strlen(sig));
    array_add_str(*str, sig);
    array_add(*str, 0x0);
    *alignment += 2 + strlen(sig);
    ALIGN(*str, *alignment, 4);
    char len[4];
    *((uint32_t*)len) = strlen(value);
    array_add_many(*str, len, 4);
    *alignment += 4;
    array_add_str(*str, value);
    array_add(*str, 0x0);
    *alignment = array_size(*str) + 12;
}

dbus_message* dbus_message_create_method_call(const char* dest, const char* path, const char* iface, const char* method) {
    dbus_message* msg = calloc(0, sizeof(dbus_message));
    if (!msg) {
        goto error;
    }

    char* header = array_new(char, 12);
    dbus_header* hd = (dbus_header*)header;
    hd->endianness = 'l';
    hd->message_type = DBUS_MESSAGE_TYPE_METHOD_CALL;
    hd->flags = 0;
    hd->major_protocol_version = 1;
    hd->message_body_length = 0;
    hd->serial = 1;

    uint32_t alignment = 12;
    char* hf = array_new(char, 4);
    alignment += 4;
    dbus_hf_add_value(&hf, &alignment, DBUS_HEADER_FIELD_PATH, path);
    dbus_hf_add_value(&hf, &alignment, DBUS_HEADER_FIELD_INTERFACE, iface);
    dbus_hf_add_value(&hf, &alignment, DBUS_HEADER_FIELD_DESTINATION, dest);
    dbus_hf_add_value(&hf, &alignment, DBUS_HEADER_FIELD_MEMBER, method);
    *((uint32_t*)hf) = array_size(hf) - 4;

    array_add_many(header, hf, array_size(hf));
    array_free(hf);

    ALIGN(header, alignment, 8);

    msg->header = header;
    return msg;
error:
    if (msg) dbus_message_free(msg);
    return NULL;
}
