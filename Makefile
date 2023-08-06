#i want to build main.c and main.h to cse561sim

CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -pedantic -std=c99
LDFLAGS = -g -Wall -Wextra -Werror -pedantic -std=c99
LDLIBS = -lm

all: cse561sim

cse561sim: main.o
	$(CC) $(LDFLAGS) -o cse561sim main.o $(LDLIBS)

main.o: main.c main.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f *.o cse561sim


