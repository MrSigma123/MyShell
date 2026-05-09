CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: myshell

myshell: myshell.c
	$(CC) $(CFLAGS) myshell.c -o myshell

clean:
	rm -f myshell
