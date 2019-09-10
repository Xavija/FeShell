/*Esegue il comando unix clear()*/
void clear(void);
/*Cambia directory di lavoro*/
void changeWorkDirectory(char* dir);
/*Stampa messaggio per la shell (left prompt*/
void message();
/*Esecuzione di una lista di comandi*/
int  run(Node* commands);
/*Duplica un file descriptor e chiude quello originale*/
int dupRedirect(int source, int dest);
/*Prepara l'esecuzione per un comando in pipe*/
int pipeCommand(_cmd* command, int in, int from_file);
/*Sceglie il metodo più adatto per l'esecuzione di un comando "esterno"*/
int chooseProvider(_cmd* cmd, int from_pipe);