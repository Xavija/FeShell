#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../structures.h"
#include "../ls-lib/ls-lib.h"
#include "utils.h"

void clear() { 	system("clear"); /*puts("\033[2J");  //ncurses man page */ }

void changeWorkDirectory(char* dir) {
	if (dir == NULL) 
		chdir(getenv("HOME")); 
	else { 
		if (chdir(dir) == -1) {
			fprintf(stderr, "%s: percorso inesistente\n", dir);
		}
	}
}

void message() {
	char cwd[FILENAME_MAX];
	if(getcwd(cwd, FILENAME_MAX) != NULL) 
		printf("\n%s $ ", cwd);
	else
		printf("\n>> ");
}

int chooseProvider(_cmd* cmd) {
	if(!strcmp(cmd->args[0], "ls")) { 									// recupero eventuali parametri per ls
		int j = 0;
		for(int i = 1; i < getArgCount(cmd); i++) {						// i = 1, args[0] = "ls"
			if(strncmp(cmd->args[i], "-", sizeof(char)) == NULL) {		// se in posizione 1 c'è un "-"
				j = i;													// allora salvo l'indice per poi passarlo come parametro a ls
				break;
			}
		}
		
		int result;
		if(!j)															// se j non è stato aggiornato, passo una stringa vuota come parametro
			result = ls("");
		else
			result = ls(cmd->args[j]);
		exit(result);
	} else {
	
			execvp(cmd->args[0], cmd->args);
			fprintf(stderr, "%s: comando non riconosciuto\n", cmd->args[0]);
			exit(1);
	}	
}

int dupRedirect(int source, int dest) {				// duplica il fd source e chiude l'originale
	if(source != dest) {			
		if(dup2(source, dest) != -1)	{
			close(source);
			return 0;
		} else {
			//fprintf(stderr, "duplicazione del file descriptor non riuscita\n");
			return -1;
		}
	}
	return 0;
}

int pipeCommand(_cmd* command, int in, int from_file) {
	int fd[2];		// pipe {READ_END, WRITE_END}
	pid_t pid;

	if(pipe(fd) != -1) {
		pid = fork();
		if(pid == 0) {						// figlio
			close(fd[0]);								// chiusura del lato READ inutilizzato del fd `fd[0]`
			
			if(dupRedirect(in, STDIN_FILENO) == -1) { 	// duplicazione del file descriptor in (READ) e successiva chiusura dell'originale
				fprintf(stderr, "duplicazione della pipe in non riuscita\n");
				exit(-1);
			}
			
			if(dupRedirect(fd[1], STDOUT_FILENO) == -1) { // duplicazione del file descriptor fd[1] (WRITE) e successiva chiusura dell'originale
				fprintf(stderr, "duplicazione della pipe out non riuscita\n");
				exit(-1);
			}
			// in questo modo si ha che il processo legge dal fd `in` e scrive su `fd[1]`
			
			if(chooseProvider(command) == -1) {
				fprintf(stderr, "%s: comando non riconosciuto", command->args[0]); // se exec fallisce
				exit(1);
			}
		} else if(pid > 0) { 				// padre
			int status;
			waitpid(pid, &status, 0);					// attendo il figlio

			close(fd[1]);								// chiusura lato WRITE inutilizzato
			close(in);									// chiusura lato READ inutilizzato della pipe precedente
			in = fd[0];									// copio il fd lato READ in `in` in modo tale da far leggere qui al prossimo comando
			
		} else if(pid == -1) {
			fprintf(stderr, "operazione di fork non riuscita\n");
			exit(-1);
		} else {
			fprintf(stderr, "errore nel processo padre\n");
			exit(-2);
		}
	} else {
		fprintf(stderr, "pipe non riuscita\n");
		exit(-3);
	}
	return in;											// return di in per aggiornamento del fd lato READ per il prossimo comando
}

int run(Node* commands) {

	/*
	run esegue ogni comando presente nella lista di comandi passata come argomento;
	restituisce 0 se l'esecuzione è avvenuta senza problemi, -1 altrimenti.
	*/

	int in  = STDIN_FILENO;										// fd READ originale
	int out;									
	int original_stdin  = dup(in);								// backup di stdin e stdout originali
	int original_stdout = dup(out);
	
	int	in_set  = 0,											// flag per redirect in
		from_pipe = 0,											// flag per informare che l'ultimo comando eseguito viene da una pipe
		out_set = 0;

	for(; commands != NULL; commands = commands->next) {
		if(commands->command.redirect_mode[MODE_IN-1]) {	// redirect STDIN <
			in = open(commands->command.file_in, O_RDONLY);
			if(in < 0) {
				fprintf(stderr, "%s: impossibile accedere al file", commands->command.file_in);
				return -1;
			}

			in_set = 1;
		}

		if(commands->command.redirect_mode[MODE_OUT-1]) {	// redirect STDOUT >
			out = open(commands->command.file_out, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IRGRP|S_IWGRP|S_IWUSR);
			if(out < 0) {
				fprintf(stderr, "%s: impossibile accedere al file", commands->command.file_in);
				return -1;
			}
			if(dup2(out, STDOUT_FILENO) == -1) {
				dupRedirect(original_stdin, STDIN_FILENO);
				dupRedirect(original_stdout, STDOUT_FILENO);
				return -1;
			} else
				close(out);
			out_set = 1;
				
		} else if(commands->command.redirect_mode[MODE_APPEND-1]) { // redirect STDOUT >>
			out = open(commands->command.file_out, O_WRONLY|O_APPEND);
			if(out < 0) {
				fprintf(stderr, "%s: impossibile accedere al file", commands->command.file_in);
				return -1;
			}
			if(dup2(out, STDOUT_FILENO) == -1) {
				dupRedirect(original_stdin, STDIN_FILENO);
				dupRedirect(original_stdout, STDOUT_FILENO);
				return -1;
			} else
				close(out);
			out_set = 1;
		}

		if(commands->command.redirect_mode[MODE_PIPE-1]) {
			if(out_set) {
				fprintf(stderr, "errore di sintassi\n");

				dupRedirect(original_stdin, STDIN_FILENO);
				dupRedirect(original_stdout, STDOUT_FILENO);
 
				return -1;
			}
			in = pipeCommand(&commands->command, in, in_set);
			in_set = 0;
			from_pipe = 1;
		} else {
			if(from_pipe && in_set) {	// se il comando precedente era in pipe al comando corrente
										// e quest'ulimo ha un file in input
				fprintf(stderr, "errore di sintassi\n");

				dupRedirect(original_stdin, STDIN_FILENO);
				dupRedirect(original_stdout, STDOUT_FILENO);
 
				return -1;
			} else {
				if(dupRedirect(in, STDIN_FILENO) == -1) { // redirect in (da pipe O file in)
					dupRedirect(original_stdin, STDIN_FILENO);
					dupRedirect(original_stdout, STDOUT_FILENO);
					return -1;
				}
			}
 			
			 pid_t pid;
			pid = fork();
			if(pid == 0) {
				chooseProvider(&commands->command); 		// figlio esegue
			} else if(pid > 0) {
				int status;
				waitpid(pid, &status, 0);					// attendo il figlio
			} else if(pid == -1) {
				fprintf(stderr, "operazione di fork non riuscita\n");
				exit(-1);
			} else {
				fprintf(stderr, "errore nel processo padre\n");
				exit(-2);
			}

			from_pipe = 0;
		}
	}

	dupRedirect(original_stdin, STDIN_FILENO);			// ripristino di stdin e stdout originali
	dupRedirect(original_stdout, STDOUT_FILENO);		// in questo modo si evita un "output sospeso"
														// o l'output di un comando come input
	return 0;
}