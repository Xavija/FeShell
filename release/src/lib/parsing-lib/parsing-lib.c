#include <stdio.h>
#include <stdlib.h>
#include <string.h>						// op. su stringhe
#include <unistd.h>						// esecuzione dei comandi
#include <sys/types.h>					// tipi di dato primitivi da sistema
#include <sys/stat.h>
#include <fcntl.h>						// manipolazione descrittore file
#include <sys/wait.h>

#include "../structures.h"
#include "parsing-lib.h"

Node* appendNode(Node* *head) {
	Node* 	aux = malloc(sizeof(Node));
	Node* 	toTail = *head;
	
	initEmptyNode(aux);

	aux->next = NULL;
	if(*head == NULL) {
		aux->prev = NULL;
		*head = aux;
	} else {
		while(toTail->next != NULL)
			toTail = toTail->next;
			// coda raggiunta
		toTail->next = aux;
		aux->prev = toTail;

	}
	return aux;
}

void initEmptyNode(Node* node) {
	node->command.arg_count 	= 0;
	
	for(int i = 0; i < 4; i++)
		node->command.redirect_mode[i] = 0;
	
	bzero(node->command.args, CMD_LENGTH);
	bzero(node->command.file_in, CMD_LENGTH);
	bzero(node->command.file_out, CMD_LENGTH);
}

int appendArg(_cmd* cmd, char* arg) {
	// aggiunge un argomento all'array di argomenti nella struttura del comando;
	if(cmd->arg_count == CMD_LENGTH)
		return 1;	// limite del numero di argomenti raggiunto
	cmd->args[cmd->arg_count] = arg;
	cmd->arg_count++;
	return 0;
}

void addFile(_cmd* cmd, char* file, int redirect_mode) { 
	if(redirect_mode == MODE_IN)
		strcpy(cmd->file_in, file);
	else if(redirect_mode == MODE_OUT || redirect_mode == MODE_APPEND)
		strcpy(cmd->file_out, file); 
	else
		exit(1);
}

int getArgCount(_cmd* cmd) 		{ return cmd->arg_count;   	 }
int hasOperand(_cmd* cmd) 		{ if(getArgCount(cmd) >= 1) return 1; else return 0; 	 }

void deleteList(Node* list) {
	list = reachListHead(list);
	while(list != NULL) {
		free(list);
		list = list->next;
	}
}

int setRedirectMode(_cmd* cmd, int mode) { 
	// imposta a 1 l'elemento di posizione MODE_* corrispondente ad una specifica modalità di redirect
	if(mode == MODE_PIPE||mode == MODE_OUT||mode == MODE_APPEND||mode == MODE_IN)
		cmd->redirect_mode[mode-1] = 1;
	else {
		return 1;
		// stderr
	}
}

Node* parse(char* string) {
	Node* commands = NULL; 	// lista doppiamente collegata di comandi

	int   flag_out 		= 0,
		  flag_append 	= 0,
		  flag_in 		= 0,
		  flag_pipe 	= 0;
	char* tok;

	int preserve_ls_path = 0;		// flag per prevenire che eventuale path specificato per ls
									// venga inserito tra gli argomenti dello stesso
	int limit_file = 0;				// flag per prevenire l'inserimento di più di un file per redirect
	int redirect_mode = MODE_NONE;
	commands = appendNode(&commands);	// elemento iniziale (vuoto)

	tok = strtok(string, STRTOK_RESTRICT_DELIM);

	while(tok != NULL) {
		// per prima cosa viene controllata la presenza di caratteri speciali
		if(!strcmp(tok, ">")) {
			if(redirect_mode != MODE_NONE) {
				fprintf(stderr, "%s: carattere speciale inatteso\n", tok);
				return NULL;
			}
			redirect_mode = MODE_OUT;
		} else if(!strcmp(tok, ">>")) {
			if(redirect_mode != MODE_NONE) {
				fprintf(stderr, "%s: carattere speciale inatteso\n", tok);
				return NULL;
			}
			redirect_mode = MODE_APPEND;
		} else if(!strcmp(tok, "<")) {
			if(redirect_mode != MODE_NONE) {
				fprintf(stderr, "%s: carattere speciale inatteso\n", tok);
				return NULL;
			}
			redirect_mode = MODE_IN;
		} else if(!strcmp(tok, "|")) {
			if(redirect_mode != MODE_NONE) {
				fprintf(stderr, "%s: carattere speciale inatteso\n", tok);
				return NULL;
			}
			redirect_mode = MODE_PIPE;
		} else if(strpbrk(tok, ">|<") == NULL) {
			if(redirect_mode != MODE_NONE) {			// se nell'iterazione precedente c'è stato un carattere speciale, iniziano i controlli
				if(hasOperand(&commands->command)) {
					setRedirectMode(&commands->command, redirect_mode); 		// registro la modalità nel comando corrente
					switch(redirect_mode) {										// e la processo:
						case MODE_PIPE:											// per una pipe non mi aspetto di dover aggiungere un file in input
							redirect_mode = MODE_PIPE;
							commands = appendNode(&commands);
							break;		
						default:												// mentre per un redirect stdin/out sì
							addFile(&commands->command, tok, redirect_mode);
							break;
					}
				} else {
					fprintf(stderr, "carattere speciale inatteso\n");
					return NULL;
				}
			}

			if(!hasOperand(&commands->command))						// se il nodo cmd non ha un comando, lo inserisco
				appendArg(&commands->command, tok);
			else if(redirect_mode == MODE_NONE || redirect_mode == MODE_PIPE) { // altrimenti, se non devo inserire un file, inserisco gli argomenti
				if(appendArg(&commands->command, tok)) {
					fprintf(stderr, "sono stati indicati troppi argomenti\n");
					return NULL;
				}
			} 

			if(redirect_mode != MODE_NONE)	// reset per prossimo token
				redirect_mode = MODE_NONE;

		} else {
			fprintf(stderr, "%s: carattere inatteso\n", tok);
			return NULL;
		}

		tok = strtok(NULL, STRTOK_RESTRICT_DELIM);
	}
	
	commands = reachListHead(commands);			// raggiungo la testa della lista...
	return commands;							// e ne restituisco il puntatore
}

Node* reachListHead(Node* list) {
	while(list->prev != NULL)												// mi assicuro che la lista parta dall'elemento iniziale (prev=NULL)
		list = list->prev;
	
	return list;
}