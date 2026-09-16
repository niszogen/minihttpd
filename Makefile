CC = gcc
CFLAGS = -Wall -Wextra --pedantic
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
BINDIR ?= $(PREFIX)/bin
INSTALL ?= install
BUILD_DIR ?= build

NAME = minihttpd
SONAME = lib$(NAME).so.1
LIB_A = $(BUILD_DIR)/lib$(NAME).a
LIB_SO = $(BUILD_DIR)/$(SONAME)
BIN = $(BUILD_DIR)/$(NAME)
OBJ = $(BUILD_DIR)/$(NAME).o

all: $(LIB_A) $(LIB_SO) $(BIN)

$(BUILD_DIR):
	@mkdir -p $@

$(OBJ): lib/$(NAME).c lib/$(NAME).h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(LIB_A): $(OBJ)
	ar rcs $@ $^

$(LIB_SO): $(OBJ)
	$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^
	ln -sf $(SONAME) $(BUILD_DIR)/lib$(NAME).so

$(BIN): main.c $(LIB_SO)
	$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -l$(NAME) -lpthread -Wl,-rpath,'$$ORIGIN':$(LIBDIR) -o $@

run: $(BIN)
	./$(BIN)

install: $(LIB_SO) $(BIN)
	$(INSTALL) -d $(DESTDIR)$(LIBDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 755 $(LIB_SO) $(DESTDIR)$(LIBDIR)/$(SONAME)
	ln -sf $(SONAME) $(DESTDIR)$(LIBDIR)/lib$(NAME).so
	$(INSTALL) -m 755 $(BIN) $(DESTDIR)$(BINDIR)/$(NAME)

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(SONAME) $(DESTDIR)$(LIBDIR)/lib$(NAME).so
	rm -f $(DESTDIR)$(BINDIR)/$(NAME)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean install uninstall

