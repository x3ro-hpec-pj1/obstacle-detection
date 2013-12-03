CC=clang
CFLAGS=-Wall
DEPS = scanner_reader.h
OBJ = scanner_reader.o main.o
NAME=cimpl

all: run



%.o: %.c $(DEPS)
	$(CC) -c -o $@ $(CFLAGS)

run: $(NAME)
	./$^ ../res/scanner.out

$(NAME): $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm $(NAME) $(OBJ)
