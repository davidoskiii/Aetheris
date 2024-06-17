main: main.c
	$(CC) main.c utils/utils.c -o aetheris -Wall -Wextra -pedantic -std=c99
