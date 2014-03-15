SHELL=csh

JANSSON_DIR=./lib/jansson
JANSSON_LIB=$(JANSSON_DIR)/src/.libs

CC=clang
CFLAGS=-Werror -Wall -Wextra -I$(JANSSON_DIR)/src -g
LDFLAGS=-L$(JANSSON_LIB) -ljansson
DEPS = rpc.h scanner_reader.h obstacle_detection.h
OBJ = rpc.o scanner_reader.o obstacle_detection.o main.o
NAME=cimpl

all: $(NAME)

%.o: %.c $(DEPS)
	$(CC)  $(CFLAGS) -c -o $@ $<

run: $(NAME)
	setenv DYLD_LIBRARY_PATH $(JANSSON_LIB) ; ./$^ ../res/scanner.out

debug: $(NAME)
	setenv DYLD_LIBRARY_PATH $(JANSSON_LIB) ; lldb ./$^ ../res/scanner.out

$(NAME): jansson $(OBJ)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJ)

clean:
	rm -f $(NAME) $(OBJ)


# ======================
# Jansson (JSON library)
# ======================

jansson: jansson_configure
	cd $(JANSSON_DIR) && make

jansson_configure:
	cd $(JANSSON_DIR); [ ! -e "configure" ] && autoreconf -fi; \
		[ ! -e "Makefile" ] && ./configure; \
		[ -e "Makefile" ]

jansson_clean:
	cd $(JANSSON_DIR) && git clean -fdx
