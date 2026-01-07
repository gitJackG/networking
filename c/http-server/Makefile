CC=clang
CFLAGS=-Wall -Wextra -pedantic -std=c99 -ggdb

http-server: http-server.c
	${CC} ${CFLAGS} $^ -o $@

.PHONY: clean

clean:
	rm -f http-server