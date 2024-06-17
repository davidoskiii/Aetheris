main: main.c
	$(CC) main.c utils/utils.c terminal/terminal.c input/input.c -o aetheris -Wall -Wextra -pedantic -std=c99
