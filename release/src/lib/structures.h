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
			// redirect_mode { MODE_PIPE, MODE_OUT, MODE_APPEND, MODE_IN }
} _cmd;									// struct rappresentante un singolo comando
typedef struct _node { 
	_cmd command; 
	struct _node* next;
	struct _node* prev;
} Node;									// lista collegata (next) di comandi