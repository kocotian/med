CC=gcc

cbms: cbms.c
	${CC} -o $@ $< -std=c99 -Wall -Wextra -pedantic
