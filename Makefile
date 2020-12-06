CC=cc
PREFIX=/usr/local

med: med.c
	${CC} -o $@ $< -std=c99 -Wall -Wextra -pedantic

install: med
	install -Dm 755 med ${PREFIX}/bin
