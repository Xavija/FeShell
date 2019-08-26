/*
	LEGGI QUI
	FAQ:
	- Cosa devo fare?
		parse(): c'è da sistemare (riga 73) l'init della lista dei comandi e l'aggiunta di un elemento
	- Per il resto?
		Leggi il TODO in basso.
		ls funziona;
		ricordati i free()!
		ricordati il makefile!
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>		// op. su stringhe
#include <dirent.h> 	// op. su directories
#include <sys/stat.h>	// descrizione di file
#include <sys/types.h>	// gestione dei tipi
#include <pwd.h>		// info sull'utente
#include <grp.h>		// info sul gruppo dell'utente
#include <time.h>		
#include <errno.h> 		// error handling
#include <unistd.h>		// esecuzione dei comandi
#include <math.h>

//includere ls

#define STRTOK_RESTRICT_DELIM " \t\n"	// delimitatori per tokenizer
#define DIM 256			// lunghezza massima degli operandi

#define MAX_NAME 256 	// lunghezza max dei nomi dei file
#define ORD_SIZE 5 		// dimensione array ordini di grandezza (per ls -(l)h: fino all'ord. dei TB)


/* TODO:
	- parser;
	- execvp;
	- dup2;
	- file redirect;
	
	- free();
	- header file(s)
	- makefile
*/




typedef struct {
	char 	operand[DIM];
	int		has_operand;
	char 	*args[DIM];		// DIM argomenti max, ciascuno lungo max DIM
	int 	arg_count;
	int 	piped,
			in,
			out,
			append;
} _cmd;
typedef struct _node { 
	_cmd command; 
	struct _node* next;
} Node;
typedef Node* cmd_list;

void clear(void);
void newList(cmd_list*);
void addNode(cmd_list*, char*, char*[], int, int, int);
void initCmd(_cmd*);
int  addOperand(_cmd*, char*);
int  appendArg(_cmd*, char*);
void setPiped(_cmd*);
void setIn(_cmd*);
void setOut(_cmd*);
void setAppend(_cmd*);
void deleteList(cmd_list*);
int  parse(char*);
int  execute(cmd_list*);

void clear() 				{ puts("\033[2J");	/* ncurses man page */ }

void newList(cmd_list *l) {	l = NULL; }

void addEmptyNode(cmd_list *l) {
    Node* aux = (Node*) malloc(sizeof(Node));
    
    bzero(aux->command.args, DIM);
    bzero(aux->command.operand, DIM);
	
	aux->command.arg_count 		= 0;
    aux->command.has_operand 	= 0;
    aux->command.piped 			= 0;
	aux->command.in 			= 0;
	aux->command.out 			= 0;
	aux->command.append 		= 0;

    aux->next = *l;
    *l = aux;
}

int addOperand(_cmd* cmd, char* op) {
	// aggiunge operando alla struct del comando e aggiorna il relativo flag
	if(cmd->has_operand)
		return 1;
	strcpy(cmd->operand, op);
	cmd->has_operand = 1;
	return 0;
}

int appendArg(_cmd* cmd, char* arg) {
	// aggiunge un argomento all'array di argomenti nella struttura del comando;
	if(cmd->arg_count == DIM)
		return 1;	// limite del numero di argomenti raggiunto
	cmd->args[cmd->arg_count] = arg;
	cmd->arg_count++;
	return 0;
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

void setPiped(_cmd* cmd) 	{ cmd->piped  = 1; 	}
void setIn(_cmd* cmd) 		{ cmd->in 	  = 1; 	}
void setOut(_cmd* cmd) 		{ cmd->out 	  = 1; 	}
void setAppend(_cmd* cmd) 	{ cmd->append = 1; 	}

int parse(char* string) {

	// PARSING
		/* TODO:
			- casistica "input vuoto" (viene erroneamente utilizzato l'input vecchio non vuoto)
			- has_operand per nuovi comandi
				--> linked list?
				- RIDEFINIZIONE METODI per LINKED LIST cmd_list
			- '|' (e ';'?) handling
				--> viene aspettato un nuovo *comando*
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

    deleteList(&commands);
	return 0;
}

int execute(cmd_list* list) {
}

int main(int argc, char *argv[]) {
	//char *buffer = malloc(sizeof(char)*BUFSIZ);
	//char *buffer[BUFSIZ];
	char* 	buffer = /* (char*) */ malloc(sizeof(char)*BUFSIZ);	// input buffer
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
