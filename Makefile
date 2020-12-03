CC=gcc

med: med.c
	${CC} -o $@ $< -std=c99 -Wall -Wextra -pedantic
