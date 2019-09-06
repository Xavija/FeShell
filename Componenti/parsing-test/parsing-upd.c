#include <stdio.h>
#include <stdlib.h>
#include <string.h>						// op. su stringhe
#include <unistd.h>						// esecuzione dei comandi
#include <sys/types.h>					// tipi di dato primitivi da sistema
#include <sys/stat.h>
#include <fcntl.h>						// manipolazione descrittore file

#define STRTOK_RESTRICT_DELIM " \t\n"	// delimitatori per tokenizer
#define CMD_LENGTH 256					// lunghezza massima degli operandi

#define MODE_NONE 0
#define MODE_PIPE 1
#define MODE_OUT 2
#define MODE_APPEND 3
#define MODE_IN 4

typedef struct {
	char 	operand[CMD_LENGTH];
	int		has_operand;
	char 	*args[CMD_LENGTH];			// CMD_LENGTH-1 argomenti max (il primo è l'operando), ciascuno lungo max CMD_LENGTH
	int 	arg_count;
	char	path[CMD_LENGTH];
	int 	has_path;
	char 	file[CMD_LENGTH];			// file per redirect
	int 	redirect_mode[4];				/*	0 - niente
												1 - piped
												2 - out
												3 - append
												4 - in */
			// redirect_mode { MODE_NONE, MODE_PIPE, MODE_OUT, MODE_APPEND, MODE_IN }
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
/*Registra un operando nella struttura dati di un comando*/
void addOperand(_cmd* cmd, char* op);
/*Aggiunge un argomento alla struttura dati di un comando*/
int  appendArg(_cmd* cmd, char* arg);
/*Imposta il flag di redirect*/
int  setRedirectMode(_cmd* cmd, int mode);
/*Registra un file indicato per il redirect*/
void addFile(_cmd* cmd, char* file);
/*Registra eventuale path indicato per ls()*/
void addPath(_cmd* cmd, char* path);
Node* reachListHead(Node* commands);
Node* parse(char* string);
int  run(Node* commands);


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
	
	for(int i = 0; i < 4; i++)
		node->command.redirect_mode[i] = 0;
	
	/* node->command.in 			= 0;
	node->command.append 		= 0;
	node->command.out 			= 0;
	node->command.piped 		= 0;
 */
	bzero(node->command.args, CMD_LENGTH);
	bzero(node->command.operand, CMD_LENGTH);
	bzero(node->command.path, CMD_LENGTH);
	bzero(node->command.file, CMD_LENGTH);
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

int setRedirectMode(_cmd* cmd, int mode) { 
	// imposta a 1 l'elemento di posizione MODE_* corrispondente ad una specifica modalità di redirect
	if(mode == MODE_PIPE||mode == MODE_OUT||mode == MODE_APPEND||mode == MODE_IN)
		cmd->redirect_mode[mode-1] = mode;
	else {
		return 1;
		// stderr
	}
}

void addFile(_cmd* cmd, char* file) { strcpy(cmd->file, file); }

void clear() { 
	system("clear");
	//puts("\033[2J");	/* ncurses man page */ 
}

int getArgCount(_cmd* cmd) 		{ return cmd->arg_count;   	 }
int hasPath(_cmd* cmd) 			{ return cmd->has_path;  	 }
int hasOperand(_cmd* cmd) 		{ return cmd->has_operand; 	 }
int getRedirectMode(_cmd* cmd) 	{ return cmd->redirect_mode; }

int execute(Node* commands);

int main() {
	char* 	buffer = (char*) malloc(sizeof(char)*BUFSIZ);	// input buffer
	if(buffer == NULL)
		exit(-1);
	size_t 	dim = (size_t) sizeof(buffer); // dimensione dell'input buffer
	
	Node* commands;

	//clear();
	while (1) {
		
		printf("\n>> ");
		if (getline(&buffer, &dim, stdin) == -1)	// input
			return 1;

		if(strstr(buffer, "exit")) {
			// free
			exit(0);
		} else if(strstr(buffer, "clear")) {
			clear();
		} else if((commands = parse(buffer)) != NULL)
			run(commands);
		
		/* if(parse_val != 0) {
			printf("Errore durante il parsing\n");
			//STDERR
		} */
				
		setbuf(stdin, NULL); // non sembra, ma *funziona*
	}
	return 0;
}

Node* parse(char* string) {
	// TODO
	// ignorare altri file dopo il primo indicato per redirect
	// esempio: ... > out1 out2 | grep ... deve restituire errore
	Node* commands = NULL; 	// lista doppiamente collegata di comandi

	int   flag_out 		= 0,
		  flag_append 	= 0,
		  flag_in 		= 0,
		  flag_pipe 	= 0;
	char* tok;

	int preserve_ls_path = 0;		// flag per prevenire che eventuale path specificato per ls
									// venga inserito tra gli argomenti dello stesso
	int limit_file = 0;				// flag per prevenire l'inserimento di più di un file per redirect
	int redirect_mode = 0;
	commands = appendNode(&commands);	// elemento iniziale (vuoto)

	tok = strtok(string, STRTOK_RESTRICT_DELIM);

	while(tok != NULL) {
		// per prima cosa viene controllata la presenza di caratteri speciali
		if(!strcmp(tok, ">")) {
			if(redirect_mode != MODE_NONE) {
				printf("Errore durante il parsing: carattere speciale inatteso\n");
				return NULL;
			}
			redirect_mode = MODE_OUT;
		} else if(!strcmp(tok, ">>")) {
			if(redirect_mode != MODE_NONE) {
				printf("Errore durante il parsing: carattere speciale inatteso\n");
				return NULL;
			}
			redirect_mode = MODE_APPEND;
		} else if(!strcmp(tok, "<")) {
			if(redirect_mode != MODE_NONE) {
				printf("Errore durante il parsing: carattere speciale inatteso\n");
				return NULL;
			}
			redirect_mode = MODE_IN;
		} else if(!strcmp(tok, "|")) {
			if(redirect_mode != MODE_NONE) {
				printf("Errore durante il parsing: carattere speciale inatteso\n");
				return NULL;
			}
			redirect_mode = MODE_PIPE;
		} else if(strpbrk(tok, ">|<") == NULL) {
			if(redirect_mode != MODE_NONE) {			// se nell'iterazione precedente c'è stato un carattere speciale, iniziano i controlli
				if(hasOperand(&commands->command)) {
					setRedirectMode(&commands->command, redirect_mode);
					switch(redirect_mode) {
						case MODE_PIPE:											// per una pipe non mi aspetto un file in input
							redirect_mode = MODE_PIPE;
							commands = appendNode(&commands);
							break;		
						default:												// mentre per un redirect sì
							addFile(&commands->command, tok);
							break;
					}
				} else {
					//TODO: stderr
					printf("Errore durante il parsing: carattere speciale inatteso\n");
					return NULL;
				}
			}

			if(!hasOperand(&commands->command))						// se il nodo cmd non ha un comando, lo inserisco
				addOperand(&commands->command, tok);

			if(!strcmp(commands->command.operand, "ls") 			// controlli per non inserire il path di ls nel posto sbagliato
				&& tok[0] != '-' 
					&& getArgCount(&commands->command) > 0 
						&& redirect_mode == MODE_NONE) {

				if(hasPath(&commands->command)) {
					printf("Troppi argomenti per ls\n"); // TODO stderr
					return NULL;
				} else {
					addPath(&commands->command, tok);
					preserve_ls_path = 1;
				}
			} 

			if(preserve_ls_path) {
				preserve_ls_path = 0;
			} else if(redirect_mode == MODE_NONE || redirect_mode == MODE_PIPE) {
				if(appendArg(&commands->command, tok)) {
					printf("Errore durante il parsing: sono stati indicati troppi argomenti\n");
					return NULL;
				}
			} 
			if(redirect_mode != MODE_NONE)
				redirect_mode = MODE_NONE;

		} else {
			printf("%s: carattere inatteso\n", tok);
			return NULL; // stderr
		}

		tok = strtok(NULL, STRTOK_RESTRICT_DELIM);
	}
	
	commands = reachListHead(commands);
	return commands;
	//DELETE LIST
	return 0;
}

Node* reachListHead(Node* commands) {
	while(commands->prev != NULL)												// mi assicuro che la lista parta dall'elemento iniziale (prev=NULL)
		commands = commands->prev;
	
	return commands;
}

void dupRedirect(int source, int dest) {
	if(source != dest) {
		if(dup2(source, dest) != -1)
			close(source);
		else 
			printf("Error: bad dup2()");
			exit;
	}
}

int execCmd(char* *args) {
	pid_t pid;

	pid = fork();
	if(pid == -1) {
		fprintf(stderr, "operazione di fork non riuscita");
		return -1;
	} else if(pid == 0) { 	// figlio(
		execvp(args[0], args);
		fprintf(stderr, "%s: comando non riconosciuto", args[0]);
		exit(1);
	} else { 				// padre
		if(waitpid(pid, 0, 0) < 0) 
			fprintf(stderr, "waitpid");
	}
}

void chooseProvider(_cmd* cmd) {
	/* if(!strcmp(cmd->operand, "ls"))
		;//ls()
	else
		;//execvp(); */

	if(!strcmp(cmd->operand, "cd")) {
		//bzero(cmd->args, 1);
		//int dir = chdir(cmd->args);
		;
	} else if(!strcmp(cmd->operand, "ls")) {
		execCmd(cmd->args);
		; // ls()
	} else {
		execCmd(cmd->args);
		; // execvp()
	}
}

int pipeCommand(_cmd* command, int in) {
	int fd[2];		// pipe {READ_END, WRITE_END}
	pid_t pid;

	if(pipe(fd) != -1) {
		pid = fork();
		if(pid == 0) {						// figlio
			close(fd[0]);								// chiusura del lato READ inutilizzato del fd `fd[0]`
			dupRedirect(in, STDIN_FILENO);				// duplicazione del file descriptor in (READ) e successiva chiusura dell'originale
			dupRedirect(fd[1], STDOUT_FILENO);			// duplicazione del file descriptor fd[1] (WRITE) e successiva chiusura dell'originale
														// in questo modo si ha che il processo legge dal fd `in` e scrive su `fd[1]`
			execvp(command->operand, command->args);	// esecuzione del comando
			//chooseProvider(command);
			
		} else if(pid > 0) { 				// padre
			close(fd[1]);								// chiusura lato WRITE inutilizzato
			close(in);									// chiusura lato READ inutilizzato della pipe precedente
			in = fd[0];									// copio il fd lato READ in `in` in modo tale da far leggere qui al prossimo comando
		} else if(pid == -1) {
			printf("Bad fork()\n");
			exit;
		} else {
			printf("Bad parent\n");
			exit;
		}
	} else {
		printf("Bad pipe()\n");
		exit(1);
	}

	return in;											// return di in per aggiornamento del fd lato READ per il prossimo comando
}

int redirectCommand(char* file, int mode) {
	int old_fd, new_fd;
	switch(mode) {
		case MODE_OUT:
			old_fd = open(file, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IRGRP|S_IWGRP|S_IWUSR);
			new_fd = STDOUT_FILENO;
			break;
		case MODE_APPEND:
			old_fd = open(file, O_APPEND|O_WRONLY);
			new_fd = STDOUT_FILENO;
			break;
		case MODE_IN:
			old_fd = open(file, O_RDONLY);
			new_fd = STDIN_FILENO;
			break;
		default:
			return;
			// stderr
			break;
	}
	if(old_fd == -1) {
		return -2;
		// stderr
	} else if(dup2(old_fd, new_fd) == -1) {
		return -3;
		// stderr
	} else {
		close(old_fd);
		if(new_fd == STDIN_FILENO)
			return old_fd;
	}
}

int run(Node* commands) {
	// TODO:
	// redirect prima di una pipe
	// print stderr
	

	int in = STDIN_FILENO;														// fd READ originale
	
	int original_stdout = dup(1);
	int i;
	int out_set = 0,
		in_set  = 0;

	for(; commands != NULL; commands = commands->next) {
		if(commands->command.redirect_mode[MODE_IN-1] == MODE_IN) {
			in = redirectCommand(commands->command.file, MODE_IN);
			in_set = 1;
		}

		if(commands->command.redirect_mode[MODE_OUT-1] == MODE_OUT) {
			redirectCommand(commands->command.file, MODE_OUT);
			out_set = 1;
		} else if(commands->command.redirect_mode[MODE_APPEND-1] == MODE_APPEND) {
			redirectCommand(commands->command.file, MODE_APPEND);
			out_set = 1;
		}

		if(commands->command.redirect_mode[MODE_PIPE-1] == MODE_PIPE) {
			in = pipeCommand(&commands->command, in);
			in_set = 0;
			out_set = 0;
		} else {
			if(!in_set)
				dupRedirect(in, STDIN_FILENO);											
			if(!out_set) 
				dup2(original_stdout, STDOUT_FILENO);
			//dupRedirect(STDOUT_FILENO, STDOUT_FILENO);								// riporto il lato WRITE al fd originale per poter stampare a video
			//execvp(commands->command.operand, commands->command.args);
			chooseProvider(&commands->command);
		}
	}

	//fflush(stdout);
}