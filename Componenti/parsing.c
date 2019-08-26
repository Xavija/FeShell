#include <stdio.h>
#include <stdlib.h>
#include <string.h>						// op. su stringhe
#include <unistd.h>						// esecuzione dei comandi

#include "ls-lib.h"

#define STRTOK_RESTRICT_DELIM " \t\n"	// delimitatori per tokenizer
#define CMD_LENGTH 256					// lunghezza massima degli operandi

typedef struct {
	char 	operand[CMD_LENGTH];
	int		has_operand;
	char 	*args[CMD_LENGTH];			// CMD_LENGTH argomenti max, ciascuno lungo max CMD_LENGTH
	int 	arg_count;
	char	path[CMD_LENGTH];
	int 	has_path;
	int 	piped,
			in,
			out,
			append;
} _cmd;									// struct rappresentante un singolo comando
typedef struct _node { 
	_cmd command; 
	struct _node* next;
} Node;
typedef Node* cmd_list;					// lista collegata (next) di comandi

/*Esegue il comando unix clear()*/
void clear(void);
/*Inizializza una lista vuota di comandi*/
void newList(cmd_list* l);
/*Aggiunge un nuovo elemento vuoto alla lista di comandi*/
void addEmptyNode(cmd_list* l);
/*Registra un operando nella struttura dati di un comando*/
int  addOperand(_cmd* cmd, char* op);
/*Aggiunge un argomento alla struttura dati di un comando*/
int  appendArg(_cmd* cmd, char* arg);
/*Imposta il flag di pipe a 1*/
void setPiped(_cmd* cmd);
/*Imposta il flag di redirect (file input) a 1*/
void setIn(_cmd* cmd);
/*Imposta il flag di redirect (file output) a 1*/
void setOut(_cmd* cmd);
/*Imposta il flag di redirect (file append) a 1*/
void setAppend(_cmd* cmd);
/*Registra eventuale path indicato per ls()*/
void addPath(_cmd* cmd, char* path);
/*Dealloca tutti i puntatori della lista di comandi*/
void deleteList(cmd_list* commands);
/*Analizza l'input ottenuto dall'utente per identificare comandi*/
int  parse(char* string);
/*Esegue i comandi ottenuti dall'operazione di `parse()`*/
int  execute(cmd_list* commands);

void clear() { puts("\033[2J");	/* ncurses man page */ }

void newList(cmd_list *l) {	*l = NULL; }

void addEmptyNode(cmd_list *l) {
    Node* aux = (Node*) malloc(sizeof(Node));
    
    bzero(aux->command.args, CMD_LENGTH);		// riempie di zeri l'array degli argomenti
    bzero(aux->command.operand, CMD_LENGTH);	// riempie di zeri l'array dell'operando
	bzero(aux->command.path, CMD_LENGTH);
	// imposta a 0 gli opportuni flag
	aux->command.arg_count 		= 0;			
    aux->command.has_operand 	= 0;
    aux->command.piped 			= 0;
	aux->command.in 			= 0;
	aux->command.out 			= 0;
	aux->command.append 		= 0;
	aux->command.has_path 		= 0;
	// aggiorna il puntatore a next
    aux->next = *l;
    *l = aux;
}

int addOperand(_cmd* cmd, char* op) {
	if(cmd->has_operand)
		return 1;					// errore se la struttura del comando interessata ha già un operando
	strcpy(cmd->operand, op);
	cmd->has_operand = 1;
	return 0;
}

int appendArg(_cmd* cmd, char* arg) {
	// aggiunge un argomento all'array di argomenti nella struttura del comando;
	if(cmd->arg_count == CMD_LENGTH)
		return 1;	// limite del numero di argomenti raggiunto
	cmd->args[cmd->arg_count] = arg;
	cmd->arg_count++;
	return 0;
}

void setPiped(_cmd* cmd) 	{ cmd->piped  = 1; 	}
void setIn(_cmd* cmd) 		{ cmd->in 	  = 1; 	}
void setOut(_cmd* cmd) 		{ cmd->out 	  = 1; 	}
void setAppend(_cmd* cmd) 	{ cmd->append = 1; 	}

void addPath(_cmd* cmd, char* path) {
	if(cmd->has_path)
		return;
	strcpy(cmd->path, path);
	cmd->has_path = 1;
}

void deleteList(cmd_list* l) {
    cmd_list current = *l; 
    cmd_list next; 

    while(current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    *l = NULL;
}


int parse(char* string) {

	// PARSING
		/* TODO:
			- casistica "input vuoto" (viene erroneamente utilizzato l'input vecchio non vuoto)
			
		 */
	cmd_list commands;
	int   flag_out 		= 0,
		  flag_append 	= 0,
		  flag_in 		= 0,
		  flag_pipe 	= 0;
	char* tok;
    
	newList(&commands);	// lista vuota
    addEmptyNode(&commands);
    tok = strtok(string, STRTOK_RESTRICT_DELIM);
	
	while(tok != NULL) {
		if(!strcmp(tok, ">")) {
			flag_out = 1;
            printf(">\n");
		} else if(!strcmp(tok, ">>")) {
			flag_append = 1;
            printf(">>\n");
		} else if(!strcmp(tok, "<")) {
			flag_in = 1;
            printf("<\n");
		} else if(!strcmp(tok, "|")) {
            flag_pipe = 1;
            printf("pipe!\n");
        } else {
			if((flag_append||flag_in||flag_out||flag_pipe)) {
				if(&commands->command.has_operand)
                	addEmptyNode(&commands);
				if(flag_pipe) {
					setPiped(&commands->command);
					flag_pipe = 0;
				} else if(flag_out) {
					setOut(&commands->command);
					flag_out = 0;
				} else if(flag_in) {
					setIn(&commands->command);
					flag_in = 0;
				} else if(flag_append) {
					setAppend(&commands->command);
					flag_append = 0;
				}
			}

			if(!commands->command.has_operand) {
				if(addOperand(&commands->command, tok)) {
					printf("Errore durante il parsing (op)\n");
					return 1;
				}
			}
			if(!strcmp(commands->command.operand, "ls") && tok[0] != '-' && !commands->command.has_path) {
				addPath(&commands->command, tok);
			}
			if(appendArg(&commands->command, tok)) {
				printf("Errore durante il parsing (args)\n");
				return 2;
			}
			/* 
			// No sta tocare!
			if(!(flag_out || flag_append || flag_in)) {
                if(!commands->command.has_operand) {
                    if(addOperand(&commands->command, tok))
                        printf("Errore durante il parsing (op)\n");
                }
                if(appendArg(&commands->command, tok))
                    printf("Errore durante il parsing (args)\n");
            } */
            /*
            [...controlli su token speciali...]
            else 
                se non preceduto da token sp
                    se elemento non esiste
                        --> addNode()
                    appendArg()
                se pipe:
                    -->addNode(... piped=1)
                    -->flag_pipe = 0
            
            */
		}

        printf("tok:\t%s\n", tok);
        printf("Op:\t%s\n", commands->command.operand);
		
        printf("Args:\t");
        for(int abcd = 0; abcd < commands->command.arg_count; abcd++) {
			printf("%s\t", commands->command.args[abcd]);
		}
        printf("\n");

		printf("ArgCount:\t%d\n", 	commands->command.arg_count);
		printf("Piped:\t%s\n", 		commands->command.piped  ? "true" : "false");
		printf("In:\t%s\n", 		commands->command.in 	 ? "true" : "false");
		printf("Out:\t%s\n", 		commands->command.out 	 ? "true" : "false");
		printf("Append:\t%s\n",		commands->command.append ? "true" : "false");
		printf("\n");

		tok = strtok(NULL, STRTOK_RESTRICT_DELIM);
    }

	execute(&commands);

    deleteList(&commands);
	return 0;
}

int execute(cmd_list* commands) {
	cmd_list el = *commands;
	
	while(el) {
		if(el->command.has_operand) {
			if(!strcmp(el->command.operand, "ls")) {
				el->command.args[0] = '\0';			// il primo argomento di ogni comando è ls();
													// onde evitare falsi positivi per ls -l, viene azzerato il primo elemento
				ls(el->command.args, el->command.path);
				//ls(el->command.args); // chiamata a ls()
			} else
				printf("execvp\n"); // execvp
			//pipes
		}
		
		el = el->next;
	}
}

int main(int argc, char *argv[]) {
	//char *buffer = malloc(sizeof(char)*BUFSIZ);
	//char *buffer[BUFSIZ];
	char* 	buffer = (char*) malloc(sizeof(char)*BUFSIZ);	// input buffer
	if(buffer == NULL)
		exit(-1);
	size_t 	dim = (size_t) sizeof(buffer); // dimensione dell'input buffer
	int parse_val;

	clear();
	while (1) {
		
		printf("\n>> ");
		if (getline(&buffer, &dim, stdin) == -1)	// input
			return 1;
	
		if((parse_val = parse(buffer)) != 0) {
			printf("Errore durante il parsing\n");
			//STDERR
		}
				
		setbuf(stdin, NULL); // non sembra, ma *funziona*
	}
	return 0;
}
