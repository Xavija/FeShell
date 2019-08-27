#include <stdio.h>
#include <stdlib.h>
#include <string.h>						// op. su stringhe
#include <unistd.h>						// esecuzione dei comandi

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
	struct _node* prev;
} Node;									// lista collegata (next) di comandi

/*Esegue il comando unix clear()*/
void clear(void);
/*Inserisce un elemento vuoto in coda alla lista*/
Node* appendNode(Node* *head);
/*Inizializza un nodo come vuoto*/
void initEmptyNode(Node* node);
/*Stampa una lista*/
void printList(Node* node);
/*Registra un operando nella struttura dati di un comando*/
void addOperand(_cmd* cmd, char* op);
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

int parse(char* string);

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
	node->command.has_operand 	= 0;
	node->command.has_path 		= 0;
	node->command.in 			= 0;
	node->command.append 		= 0;
	node->command.out 			= 0;
	node->command.piped 		= 0;

	bzero(node->command.args, CMD_LENGTH);
	bzero(node->command.operand, CMD_LENGTH);
	bzero(node->command.path, CMD_LENGTH);
}

void printList(Node* node) {
	while(node->prev != NULL) {
		node = node->prev;
	}

	while(node != NULL) {
		printf("%d\t", node->command.arg_count);
		node = node->next;
	}
}

void addOperand(_cmd* cmd, char* op) {
	strcpy(cmd->operand, op);
	cmd->has_operand = 1;
}

int appendArg(_cmd* cmd, char* arg) {
	// aggiunge un argomento all'array di argomenti nella struttura del comando;
	if(cmd->arg_count == CMD_LENGTH)
		return 1;	// limite del numero di argomenti raggiunto
	cmd->args[cmd->arg_count] = arg;
	cmd->arg_count++;
	return 0;
}

void addPath(_cmd* cmd, char* path) {
	if(cmd->has_path)
		return;
	strcpy(cmd->path, path);
	cmd->has_path = 1;
}

void setPiped(_cmd* cmd) 	{ cmd->piped  = 1; 	}
void setIn(_cmd* cmd) 		{ cmd->in 	  = 1; 	}
void setOut(_cmd* cmd) 		{ cmd->out 	  = 1; 	}
void setAppend(_cmd* cmd) 	{ cmd->append = 1; 	}

void clear() { puts("\033[2J");	/* ncurses man page */ }

int getArgCount(_cmd* cmd) 	{ return cmd->arg_count;   	}
int hasPath(_cmd* cmd) 		{ return cmd->has_path;  	}
int hasOperand(_cmd* cmd) 	{ return cmd->has_operand; 	}
int isPiped(_cmd* cmd)		{ return cmd->piped;		}
int isOut(_cmd* cmd) 		{ return cmd->out;			}
int isAppend(_cmd* cmd)		{ return cmd->append;		}
int isIn(_cmd* cmd)			{ return cmd->in;			}

int main() {
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

int parse(char* string) {
	
	Node* commands = NULL; 	// lista doppiamente collegata di comandi

	int   flag_out 		= 0,
		  flag_append 	= 0,
		  flag_in 		= 0,
		  flag_pipe 	= 0;
	char* tok;

	int preserve_ls_path = 0;		// flag per prevenire che eventuale path specificato per ls
									// venga inserito tra gli argomenti dello stesso
	
	commands = appendNode(&commands);	// elemento iniziale (vuoto)

	tok = strtok(string, STRTOK_RESTRICT_DELIM);

	while(tok != NULL) {
		// per prima cosa viene controllata la presenza di caratteri speciali
		if(!strcmp(tok, ">")) {
			flag_out 	= 1;
		} else if(!strcmp(tok, ">>")) {
			flag_append = 1;
		} else if(!strcmp(tok, "<")) {
			flag_in		= 1;
		} else if(!strcmp(tok, "|")) {
			flag_pipe	= 1;
		} else {
			// trattamento dei caratteri speciali identificati nell'iterazione precedente
			if((flag_append||flag_in||flag_out||flag_pipe)) {
				if(hasOperand(&commands->command))
					commands = appendNode(&commands);
				if(flag_pipe) {
					setPiped(&commands->command);
					flag_pipe 	= 0;
				} else if(flag_out) {
					setOut(&commands->command);
					flag_out 	= 0;
				} else if(flag_append) {
					setAppend(&commands->command);
					flag_append = 0;
				} else if(flag_in) {
					setIn(&commands->command);
					flag_in		= 0;
				}
			}

			/* // registrazione del comando
			if(!hasOperand(&commands->command))
				addOperand(&commands->command, tok);

			
			if(appendArg(&commands->command, tok)) {
				printf("Errore durante il parsing: sono stati indicati troppi argomenti\n");
				return 2;
			}

			if(!strcmp(commands->command.operand, "ls") && tok[0] != '-' && getArgCount(&commands->command) > 1) {
				if(hasPath(&commands->command)) {
					printf("Troppi argomenti per ls\n");
					return 1;
				} else
					addPath(&commands->command, tok);
			}  */

			if(!hasOperand(&commands->command))
				addOperand(&commands->command, tok);

			if(!strcmp(commands->command.operand, "ls") && tok[0] != '-' && getArgCount(&commands->command) > 0) {
				if(hasPath(&commands->command)) {
					printf("Troppi argomenti per ls\n");
					return 1;
				} else {
					addPath(&commands->command, tok);
					preserve_ls_path = 1;
				}
			} 

			if(preserve_ls_path) {
				preserve_ls_path = 0;
			} else {
				if(appendArg(&commands->command, tok)) {
					printf("Errore durante il parsing: sono stati indicati troppi argomenti\n");
					return 2;
				}
			}
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
		printf("Path (ls):\t%s\n", 	commands->command.path);
		printf("\n");


		tok = strtok(NULL, STRTOK_RESTRICT_DELIM);
	}
	
	printList(commands);
	//EXECUTE
	//DELETE LIST
	return 0;
}

int execute(Node* commands) {
	while(commands->prev != NULL)
		commands = commands->prev;
	
	/*
	exec:
		while commands:
			if command is ls:
				ls(args, path)
			else execvp;
		// supporto pipe e redirect?
	*/
}