JANSSON_DIR=./lib/jansson
JANSSON_LIB=$(JANSSON_DIR)/src/.libs

#CC=
LDFLAGS=-static -L$(JANSSON_LIB) -ljansson -lm -lpthread
CFLAGS=-Werror -Wall -Wextra -I$(JANSSON_DIR)/src -O2 -g -std=gnu99
DEPS = rpc.h scanner_reader.h obstacle_detection.h visualization.h
OBJ = rpc.o scanner_reader.o obstacle_detection.o visualization.o main.o
NAME=cimpl

all: $(NAME)

%.o: %.c $(DEPS)
	$(CC)  $(CFLAGS) -c -o $@ $<

run: $(NAME)
	setenv DYLD_LIBRARY_PATH $(JANSSON_LIB) ; ./$^ ../res/scanner2.out

run_local: $(NAME)
	setenv DYLD_LIBRARY_PATH $(JANSSON_LIB) ; ./$^ ./scanner.out

debug: $(NAME)
	setenv DYLD_LIBRARY_PATH $(JANSSON_LIB) ; lldb ./$^ ../res/scanner.out

run_scanner: $(NAME)
	setenv DYLD_LIBRARY_PATH $(JANSSON_LIB) ; ./$^ /dev/cu.usbmodemfa131

$(NAME): jansson $(OBJ)
	$(CC) -o $@ $(CFLAGS) $(OBJ) $(LDFLAGS)

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
