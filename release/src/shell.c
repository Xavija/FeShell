#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/structures.h"
#include "lib/utils/utils.h"
#include "lib/parsing-lib/parsing-lib.h"

int main(int argc, char* argv[]) {
	char* 	buffer = (char*) malloc(sizeof(char)*BUFSIZ);	// input buffer
	if(buffer == NULL) {
		fprintf(stderr, "allocazione del buffer non riuscita\n");
		exit(2);
	}
	size_t 	dim = (size_t) sizeof(buffer); // dimensione dell'input buffer
	
	Node* commands;
	int result;

	while (1) {
		message();
		if (getline(&buffer, &dim, stdin) == -1 || feof(stdout)) {	// input
			free(buffer);
			exit(0);
		}

		if(!strcmp(buffer, "\n")) 
			continue;

		if((commands = parse(buffer)) != NULL) {
			if(!strcmp(commands->command.args[0], "exit")) {
				deleteList(commands);
				free(buffer);
				exit(0);
			} else if(!strcmp(commands->command.args[0], "clear")) {
				clear();
			} else if(!strcmp(commands->command.args[0], "cd")) {
				changeWorkDirectory(commands->command.args[1]);
			} else {
				result = run(commands);
				deleteList(commands);

				if(result == -1)
					fprintf(stderr, "Esecuzione non riuscita\n");
			}
		}
		
		setbuf(stdin, NULL); // non sembra, ma *funziona*
	}

	return 0;
}
