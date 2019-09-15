#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>

#define MAX_NAME 256 	// lunghezza max dei nomi dei file
#define ORD_SIZE 5 		// dimensione array ordini di grandezza (per ls -(l)h: fino all'ord. dei TB)
#define LS_OPTIONS_NUM 4
#define LS_ALL 0
#define LS_HUMANIZE 1
#define LS_LONG	2
#define LS_TIME	3


typedef struct _file_fe {
	//char name[MAX_NAME];
	char name[MAX_NAME];	// nome
	time_t mtime;			// data/ora ultima modifica
	mode_t mode;			// permessi
	nlink_t links;			// # nodi
	uid_t uid;				// user id
	gid_t gid;				// group id
	size_t size;			// dimensione
	char ord[3];			// ordine di grandezza per dimensione (-h)
} file_fe;

/*Stampa in maniera leggibile i permessi di un'entry in una directory aperta.*/
void print_mod(__mode_t mode);
void print_long(file_fe* f);
void humanize(file_fe* files, int arr_size);
/*Conta i file presenti nella directory specificata (compresi `.` e `..`).*/
int file_count(char *d);
/*Conta i file presenti nella directory specificata - escludendo `.`, `..` e i file nascosti*/
int visible_file_count(char *d);
/*Ordinamento dei file per data di modifica più recente (insertion sort).*/
void sort_by_mtime(file_fe* files, int dim);
/*Reimplementazione del comando `ls`*/
int ls(char** param, int param_count);