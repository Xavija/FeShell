#include <stdio.h>
#include <stdlib.h>
#include <string.h>						// op. su stringhe
#include <unistd.h>						// esecuzione dei comandi
#include <sys/types.h>					// tipi di dato primitivi da sistema
#include <sys/stat.h>
#include <fcntl.h>						// manipolazione descrittore file
#include <sys/wait.h>

#define STRTOK_RESTRICT_DELIM " \t\n"	// delimitatori per tokenizer
#define CMD_LENGTH 256					// lunghezza massima degli operandi

#define MODE_NONE 0
#define MODE_PIPE 1
#define MODE_OUT 2
#define MODE_APPEND 3
#define MODE_IN 4

typedef struct {
	char 	*args[CMD_LENGTH];			// CMD_LENGTH-1 argomenti max (il primo è l'operando), ciascuno lungo max CMD_LENGTH
	int 	arg_count;
	char 	file_in[FILENAME_MAX],
			file_out[FILENAME_MAX];
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
/*Aggiunge un argomento alla struttura dati di un comando*/
int  appendArg(_cmd* cmd, char* arg);
/*Imposta il flag di redirect*/
int  setRedirectMode(_cmd* cmd, int mode);
/*Registra un file indicato per il redirect*/
void addFile(_cmd* cmd, char* file, int redirect_mode);
Node* reachListHead(Node* list);
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
	
	for(int i = 0; i < 4; i++)
		node->command.redirect_mode[i] = 0;
	
	/* node->command.in 			= 0;
	node->command.append 		= 0;
	node->command.out 			= 0;
	node->command.piped 		= 0;
 */
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

int setRedirectMode(_cmd* cmd, int mode) { 
	// imposta a 1 l'elemento di posizione MODE_* corrispondente ad una specifica modalità di redirect
	if(mode == MODE_PIPE||mode == MODE_OUT||mode == MODE_APPEND||mode == MODE_IN)
		cmd->redirect_mode[mode-1] = 1;
	else {
		return 1;
		// stderr
	}
}

void addFile(_cmd* cmd, char* file, int redirect_mode) { 
	if(redirect_mode == MODE_IN)
		strcpy(cmd->file_in, file);
	else if(redirect_mode == MODE_OUT || redirect_mode == MODE_APPEND)
		strcpy(cmd->file_out, file); 
	else
		exit(1);
}

void clear() { 
	system("clear");
	//puts("\033[2J");	/* ncurses man page */ 
}

int getArgCount(_cmd* cmd) 		{ return cmd->arg_count;   	 }
int hasOperand(_cmd* cmd) 		{ if(getArgCount(cmd) >= 1) return 1; else return 0; 	 }
int getRedirectMode(_cmd* cmd) 	{ return cmd->redirect_mode; }

int execute(Node* commands);

void message() {
	char cwd[FILENAME_MAX];
	if(getcwd(cwd, FILENAME_MAX) != NULL) 
		printf("\n%s $ ", cwd);
	else
		printf("\n>> ");
}

int main() {
	char* 	buffer = (char*) malloc(sizeof(char)*BUFSIZ);	// input buffer
	if(buffer == NULL) {
		fprintf(stderr, "allocazione del buffer non riuscita\n");
		exit(2);
	}
	size_t 	dim = (size_t) sizeof(buffer); // dimensione dell'input buffer
	
	Node* commands;
	int result;

	//clear();
	while (1) {
		message();
		if (getline(&buffer, &dim, stdin) == -1 || feof(stdout)) {	// input
			free(buffer);
			exit(0);
		}

		if(strstr(buffer, "exit")) {
			free(buffer);
			exit(0);
		} else if(strstr(buffer, "clear")) {
			clear();
		} else if((commands = parse(buffer)) != NULL) {
			result = run(commands);
			deleteList(commands);

			if(result == -1)
				fprintf(stderr, "Esecuzione non riuscita\n");
		}
		
		setbuf(stdin, NULL); // non sembra, ma *funziona*
	}
	return 0;
}

void deleteList(Node* list) {
	list = reachListHead(list);
	while(list != NULL) {
		free(list);
		list = list->next;
	}
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
			else


			/* if(!strcmp(commands->command.args[0], "ls") 			// controlli per non inserire il path di ls nel posto sbagliato
				&& tok[0] != '-' 
					&& getArgCount(&commands->command) >= 1 
						&& redirect_mode == MODE_NONE) {

				if(hasPath(&commands->command)) {
					fprintf(stderr, "troppi argomenti per ls\n");
					return NULL;
				} else {
					addPath(&commands->command, tok);
					preserve_ls_path = 1;
				}
			} 
 */
			/* if(preserve_ls_path) {
				preserve_ls_path = 0;
			} else  */
			if(redirect_mode == MODE_NONE || redirect_mode == MODE_PIPE) {
				if(appendArg(&commands->command, tok)) {
					fprintf(stderr, "sono stati indicati troppi argomenti\n");
					return NULL;
				}
			} 

			if(redirect_mode != MODE_NONE)	// reset per prossimo token
				redirect_mode = MODE_NONE;

		} else {
			fprintf(stderr, "%s: carattere inatteso\n", tok);
			return NULL; // stderr
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

void execCmd(char* *args) {
	pid_t pid;

	pid = fork();
	if(pid == -1) {
		fprintf(stderr, "operazione di fork non riuscita\n");
		return -1;
	} else if(pid == 0) { 	// figlio
		execvp(args[0], args);
		fprintf(stderr, "%s: comando non riconosciuto\n", args[0]);
		exit(1);
	} else if(pid > 0) { 				// padre
		int status;
		waitpid(pid, &status, 0);
		if(WIFEXITED(status)) {
			printf("[%d] TERMINATED (Status: %d)\n",
			pid, WEXITSTATUS(status));
		}
	}
}

void changeWorkingDirectory(char* dir) {
	if (dir == NULL) 
		chdir(getenv("HOME")); 
	else { 
		if (chdir(dir) == -1) {
			fprintf(stderr, "%s: percorso inesistente\n", dir);
		}
	}
}

void chooseProvider(_cmd* cmd) {
	if(!strcmp(cmd->args[0], "cd")) {
		changeWorkingDirectory(cmd->args[1]);
	} else {
		execCmd(cmd->args);
	}
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
			execvp(command->args[0], command->args);	// esecuzione effettiva del comando indicato
			
			fprintf(stderr, "%s: comando non riconosciuto", command->args[0]); // se exec fallisce
			exit(1);
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
		from_pipe = 0;											// flag per informare che l'ultimo comando eseguito viene da una pipe

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
		}

		if(commands->command.redirect_mode[MODE_PIPE-1]) {
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
			chooseProvider(&commands->command);
			from_pipe = 0;
		}
	}

	dupRedirect(original_stdin, STDIN_FILENO);			// ripristino di stdin e stdout originali
	dupRedirect(original_stdout, STDOUT_FILENO);		// in questo modo si evita un "output sospeso"
														// o l'output di un comando come input
	return 0;
}
