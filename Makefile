CC=clang
CFLAGS=-Wall
DEPS = scanner_reader.h obstacle_detection.h
OBJ = scanner_reader.o obstacle_detection.o main.o
NAME=cimpl

all: $(NAME)



%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

run: $(NAME)
	./$^ ../res/scanner.out

$(NAME): $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm -f $(NAME) $(OBJ)
