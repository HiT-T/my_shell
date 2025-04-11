CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99

all: mysh

mysh.o: mysh.c
	$(CC) $(CFLAGS) -c $< -o $@

mysh: mysh.o
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f mysh mysh.o