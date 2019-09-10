all: feshell

ls-lib.o: lib/ls-lib/ls-lib.c
	gcc -Wall -c lib/ls-lib/ls-lib.c
	mv ls-lib.o lib/ls-lib

utils.o: lib/utils/utils.c
	gcc -Wall -c lib/utils/utils.c
	mv utils.o lib/utils

parsing-lib.o: lib/parsing-lib/parsing-lib.c
	gcc -Wall -c lib/parsing-lib/parsing-lib.c
	mv parsing-lib.o lib/parsing-lib

shell.o: shell.c
	gcc -Wall -c shell.c

feshell: shell.o lib/parsing-lib/parsing-lib.o lib/utils/utils.o lib/ls-lib/ls-lib.o
	gcc -o feshell shell.o lib/parsing-lib/parsing-lib.o lib/utils/utils.o lib/ls-lib/ls-lib.o -lm
	mv feshell ..
clean:
	rm shell.o
	rm lib/*/*.o
