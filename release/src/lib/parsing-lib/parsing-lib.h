#define STRTOK_RESTRICT_DELIM " \t\n"	// delimitatori per tokenizer

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
void addFile(_cmd* cmd, char* file, int redirect_mode);
/*Raggiunge la testa di una lista e ne restituisce il puntatore*/
Node* reachListHead(Node* list);
/*Analizza una stringa riconoscendone i comandi e restituendo un elenco di comandi*/
Node* parse(char* string);
/*Elimina una lista*/
void deleteList(Node* list);
/*Restituisce il numero di argomenti di un comando*/
int getArgCount(_cmd* cmd);
/*Restituisce 1 se il comando ha 1 argomento (operando), 0 altrimenti*/
int hasOperand(_cmd* cmd);