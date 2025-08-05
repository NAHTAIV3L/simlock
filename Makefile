BIN=simlock
CC=gcc
LIBS=egl wayland-client wayland-cursor gl wayland-egl dbus-1 xkbcommon
CFLAGS=-Wall -g $(shell pkg-config --cflags $(LIBS))
LDFLAGS=$(shell pkg-config --libs $(LIBS))
VPATH=/usr/share/wayland-protocols/staging/ext-session-lock/
WLPROT=ext-session-lock-v1.xml
WLC=$(WLPROT:.xml=.c)
WLH=$(WLPROT:.xml=.h)
SRC=$(WLC) $(wildcard *.c) $(wildcard glad/*.c)
OBJ=$(SRC:.c=.o)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: %.c
%.c: %.xml %.h
	wayland-scanner private-code $<  $@

.PRECIOUS: %.h
%.h: %.xml
	wayland-scanner client-header $<  $@

clean:
	$(RM) $(BIN) *.o $(WLC) $(WLH)

run: $(BIN)
	./$(BIN)

.PHONY: all clean run
