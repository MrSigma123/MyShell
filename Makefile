CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = myshell

all: $(TARGET)

$(TARGET): myshell.c
	$(CC) $(CFLAGS) myshell.c -o $(TARGET)

test: $(TARGET)
	chmod +x test.sh
	./test.sh

clean:
	rm -f $(TARGET)
	rm -f test_output.txt append_test.txt out.txt commands.txt commands_test.txt input_test.txt script_test
	rm -f test_output.log
	rm -rf test_home
