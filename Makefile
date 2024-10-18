CC = gcc
CFLAGS = -Wall -Wextra -O2

all: file_sender xor_files

file_sender: ./src/file_sender.c
	$(CC) $(CFLAGS) -o file_sender ./src/file_sender.c

xor_files: ./src/xor_files.c
	$(CC) $(CFLAGS) -o xor_files ./src/xor_files.c

clean:
	rm -f file_sender xor_files
