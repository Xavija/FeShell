all: feshell

ls-lib.o: src/lib/ls-lib/ls-lib.c
	gcc -Wall -c src/lib/ls-lib/ls-lib.c
	mv ls-lib.o src/lib/ls-lib

utils.o: src/lib/utils/utils.c
	gcc -Wall -c src/lib/utils/utils.c
	mv utils.o src/lib/utils

parsing-lib.o: src/lib/parsing-lib/parsing-lib.c
	gcc -Wall -c src/lib/parsing-lib/parsing-lib.c
	mv parsing-lib.o src/lib/parsing-lib

shell.o: src/shell.c
	gcc -Wall -c src/shell.c

feshell: src/shell.o src/lib/parsing-lib/parsing-lib.o src/lib/utils/utils.o src/lib/ls-lib/ls-lib.o
	gcc -o feshell src/shell.o src/lib/parsing-lib/parsing-lib.o src/lib/utils/utils.o src/lib/ls-lib/ls-lib.o -lm

clean:
	rm src/shell.o
	rm src/lib/*/*.o
