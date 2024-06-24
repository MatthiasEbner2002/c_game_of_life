CC = gcc
CFLAGS = -std=gnu11 -Wall -Werror -Wextra -O3 -fopenmp ./logger.c
CFLAGS += -g  # For valgrind
LDLIBS = -lncursesw

.PHONY: all
all: main

.PHONY: clean
clean:
	$(RM) main

main: main.c

