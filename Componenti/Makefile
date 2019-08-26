CC = gcc
CFLAGS=-lm -g
DEPS = ls-lib.h
	
%.o: %.c ($DEPS)
	$(CC) -c -o $@ $<

parsing: parsing.o ls-lib.o
	$(CC) -o parsing parsing.o ls-lib.o $(CFLAGS)