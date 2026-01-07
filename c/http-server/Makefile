CC=clang
CFLAGS=-Wall -Wextra -pedantic -std=c99 -ggdb

http_server: http_server.c
	${CC} ${CFLAGS} $^ -o $@

.PHONY: clean

clean:
	rm -f http_server