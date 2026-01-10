CPPC=clang++
CPPFLAGS=-Wall -Wextra -pedantic -ggdb -std=c++20

http_server: http_server.cpp
	${CPPC} ${CPPFLAGS} $^ -o $@

.PHONY: clean

clean:
	rm -f http_server