CC = gcc
CFLAGS = ./logger.c -std=gnu11 -Wall -Werror -Wextra -O3 -fopenmp -g
LOADLIBES = -lncursesw
.PHONY: all
all: main

.PHONY: clean
clean:
	$(RM) main

main: main.c

