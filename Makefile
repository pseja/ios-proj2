CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic
LDFLAGS=-pthread -lrt

.PHONY: run clean zip

proj2: proj2.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f proj2 proj2.out

zip:
	zip proj2.zip proj2.c Makefile
