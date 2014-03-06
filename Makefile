SHELL=csh

JANSSON_DIR=./lib/jansson
JANSSON_LIB=$(JANSSON_DIR)/src/.libs

CC=clang
CFLAGS=-I$(JANSSON_DIR)/src -L$(JANSSON_LIB) -ljansson -Wall -g
DEPS = scanner_reader.h obstacle_detection.h
OBJ = scanner_reader.o obstacle_detection.o main.o
NAME=cimpl

all: $(NAME)



%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

run: $(NAME)
	setenv DYLD_LIBRARY_PATH $(JANSSON_LIB) ; ./$^ ../res/scanner.out

debug: $(NAME)
	setenv DYLD_LIBRARY_PATH $(JANSSON_LIB) ; lldb ./$^ ../res/scanner.out



$(NAME): $(OBJ) jansson
	$(CC) -o $@ $(CFLAGS) $(OBJ)

clean: jansson_clean
	rm -f $(NAME) $(OBJ)
	cd lib/jansson && make clean


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
