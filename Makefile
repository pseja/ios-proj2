CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic

.PHONY: run clean zip

proj2: proj2.c
	$(CC) $(CFLAGS) $< -o $@

run: proj2
	./proj2

clean:
	rm -f proj2

zip:
	zip proj2.zip *.c
