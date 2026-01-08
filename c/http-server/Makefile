CC=clang
CFLAGS=-Wall -Wextra -pedantic -std=gnu99 -ggdb

http_server: http_server.c string_ops.h fs.h
	${CC} ${CFLAGS} $< -o $@

.PHONY: clean

clean:
	rm -f http_server