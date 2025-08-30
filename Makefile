BIN=simlock
CC=gcc
OBJDIR=objs
LIBS=egl wayland-client gl wayland-egl dbus-1 xkbcommon pam
CFLAGS=-Wall -g $(shell pkg-config --cflags $(LIBS)) -I$(OBJDIR)
LDFLAGS=$(shell pkg-config --libs $(LIBS))
VPATH=/usr/share/wayland-protocols/staging/ext-session-lock/:$(OBJDIR)
WLPROT=ext-session-lock-v1.xml
WLC=$(OBJDIR)/$(WLPROT:.xml=.c)
WLH=$(OBJDIR)/$(WLPROT:.xml=.h)
SRC=$(WLC) $(wildcard *.c) $(wildcard glad/*.c)
OBJ=$(patsubst %.c,$(OBJDIR)/%.o, $(notdir $(SRC)))

.SILENT: $(OBJ) $(WLC) $(WLH) $(BIN) $(OBJDIR) compile_flags

all: $(OBJDIR) $(BIN)

$(OBJDIR):
	[ -d $@ ] || mkdir -p $@

$(BIN): $(OBJ)
	printf "\tLD %s\n" $@
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: %.c %.h
	printf "\tCC %s\n" $@
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: */%.c */%.h
	printf "\tCC %s\n" $@
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c
	printf "\tCC %s\n" $@
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(OBJDIR)/%.c
$(OBJDIR)/%.c: %.xml $(OBJDIR)/%.h
	printf "\twayland-scanner %s\n" $@
	wayland-scanner private-code $<  $@

.PRECIOUS: $(OBJDIR)/%.h
$(OBJDIR)/%.h: %.xml
	printf "\twayland-scanner %s\n" $@
	wayland-scanner client-header $<  $@

compile_flags:
	echo $(CFLAGS) | tr ' ' '\n' > compile_flags.txt

clean:
	rm -rf $(BIN) $(OBJDIR)

install: all
	install -m 755 ./simlock $(PREFIX)/usr/bin
	install -m 644 -T ./simlock.pam $(PREFIX)/etc/pam.d/simlock

run: all
	./$(BIN)

.PHONY: all clean run compile_flags install
