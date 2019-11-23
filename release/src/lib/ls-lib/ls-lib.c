#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>

#include "../structures.h"
#include "ls-lib.h"

void print_help() {
	printf("Uso: ls [OPZIONE]\n");
	printf("Fornisce informazioni sulle entry presenti nella directory corrente.\n");
	printf("\n");
	printf("Opzioni disponibili:\n");
	printf("-a\tnon nasconde le voci che iniziano con .\n");
	printf("-h\tstampa le dimensioni delle entry in formato human-readable (con -l)\n");
	printf("-l\tstampa ulteriori informazioni sulle entry\n");
	printf("-t\tstampa le entry ordinate secondo data di modifica, dalla più recente alla meno recente\n");
}

void print_mod(__mode_t mode) {
	/*
		Stampa in maniera leggibile i permessi di un'entry convertendone prima i valori in base 8.
		Accetta:	permessi		`mode`
	*/

	int p = mode&0777;								// conversione dei permessi in ottale
	printf((S_ISDIR(mode)) ? "d" : "-");
	printf((p & S_IRUSR) ? "r" : "-");				// USER
	printf((p & S_IWUSR) ? "w" : "-");
	printf((p & S_IXUSR) ? "x" : "-");
	printf((p & S_IRGRP) ? "r" : "-");				// GROUP
	printf((p & S_IWGRP) ? "w" : "-");
	printf((p & S_IXGRP) ? "x" : "-");
	printf((p & S_IROTH) ? "r" : "-");				// OTHER
	printf((p & S_IWOTH) ? "w" : "-");
	printf((p & S_IXOTH) ? "x" : "-");
}

int file_count(char *d) {
	/*
		Resituisce il numero di entry contenute nella directory fornita come parametro.
		Accetta:	directory 	`char d[]`
		Restituisce: 	intero	`n`		
	*/
	
	DIR *directory;
	int n = 0;
	if((directory = opendir(d)) != NULL) {
		while(readdir(directory) != NULL) 
			n++;
	} else {
		printf("ls: %s (%s)\n", strerror(errno), d);
		return -1;
	}

	return n;
}

int visible_file_count(char *d) {
	/*
		Restituisce il numero di entry visibili contenute nella directory fornita come parametro.
		Accetta:	directory 	`char d[]`
		Restituisce: 	intero	`n`
	*/
	
	DIR *directory;
	struct dirent *entry;
	int n = 0;
	if((directory = opendir(d)) != NULL) {
		while((entry = readdir(directory)) != NULL) 
			if(entry->d_name[0] != '.')
				n++;
	} else {
		printf("ls: %s (%s)\n", strerror(errno), d);
		return -1;
	}

	return n;
}

void sort_by_mtime(file_fe *files, int dim){   
	/*
		Ordinamento dei file per data di ultima modifica più recente (insertion sort).
		Accetta:	vettore file		`files`
					dimensione vettore	`dim`
	*/

    int i, j; 
    file_fe aux;        
    for(i = 1; i < dim; i++) {
        aux = files[i];       
        j = i - 1;

        while(j >= 0 && difftime(files[j].mtime, aux.mtime) < 0) {     //aux è più recente di files[j]
            files[j+1] = files[j];
            j--;
        }

        files[j+1] = aux;
    }
}

void print_long(file_fe* f) {
	/*
		Stampa tutte le caratteristiche di ogni file passato come parametro.
		- permessi
		- nodi
		- (ID) utente
		- (ID) gruppo
		- dimensione
		- ultima modifica
		- nome
		Accetta: 	file_fe*		`f`
	*/

	struct 	passwd 	*usr;			// utente
	struct 	group 	*grp;			// gruppo
	struct 	tm 		*time_raw;		// data/ora "raw", da localizzare
	char	time_str[256];			// data/ora visualizzabile

	print_mod(f->mode);																// permessi
	printf("\t%4d\t", f->links);													// nodi

	if((usr = getpwuid(f->uid)) != NULL)												
		printf("%-8.8s\t", usr->pw_name);											// utente
	else
		printf("%lu\t", (unsigned long int)f->uid);									// (oppure ID utente)

	if ((grp = getgrgid(f->gid)) != NULL)													
		printf(" %-8.8s\t", grp->gr_name);											// gruppo
	else
		printf(" %-8d\t", f->gid);													// (oppure ID gruppo)
	
	if(f->ord == NULL)
		printf("%d\t", (long int)f->size);													// dimensione
	else
		printf("%d%s\t", (int)f->size, f->ord);
	
	time_raw = localtime(&f->mtime);													
	strftime(time_str, sizeof(time_str), /*nl_langinfo(D_T_FMT)*//*"%c %d %H:%M"*/"%c", time_raw);	
	printf("%s\t", time_str);														// data e ora ultima modifica
	printf("%s\n", f->name);
}

void humanize(file_fe* files, int arr_size) {

	/*
		Elabora la dimensione di ogni file per renderla "human readable", con ordini di grandezza B ~ TB.
		Accetta:	array dei file 		`files`
					dimensione array	`arr_size`
	*/

	char *ord[ORD_SIZE] = { "B", "KB", "MB", "GB", "TB" }; 	// array degli ordini di grandezza
	int j;													// contatore per l'array di ordini di grandezza
	float dim;												// var. ausiliaria per calcoli sulla dimensione
	
	for(int i = 0; i < arr_size; i++) {
		dim = (float) files[i].size;						
		j = 0;
		while(dim >= (float) 1024 && j < ORD_SIZE-1) {
			dim = ceil(dim/1024);							// divisioni successive per determinare l'ordine di grandezza con arrotondamento (ceil)
			j++;
		}
		files[i].size = (long int) dim;						// una volta finito il ciclo, sovrascrivo la dimensione in files con quella risultante...
		strcpy(files[i].ord, ord[j]);						// ...e copio l'ordine di grandezza indicato da j
	}

	// VERSIONE LONG INT
	/* char *ord[ORD_SIZE] = { "B", "KB", "MB", "GB", "TB" }; 	// array degli ordini di grandezza
	int j;													// contatore per l'array di ordini di grandezza
	long int dim;											// var. ausiliaria

	for(int i = 0; i < arr_size; i++) {
		dim = (long int) files[i].size;
		j = 0;
		while(dim >= (long int)1024 && j < ORD_SIZE-1) {
			dim /= 1024;									// divisioni successive per determinare l'ordine di grandezza
			//dim = ceil(dim/1024);
			j++;
		}
		files[i].size = (long int) dim;						
		strcpy(files[i].ord, ord[j]);						
	} 
 	*/
}

int scan_dir(char* path, file_fe* files, int all) {
	/*
		Legge i contenuti della directory `path` e li inserisce nell'array `files`.
		Accetta: 	directory		`path`
					array 			`files`
					flag visibilità `all`
		Restituisce:	`0` in caso di riuscita
						`1` in caso di fallimento nella lettura della directory
	*/
	DIR *directory;					// flusso della directory aperta
	struct 	dirent 	*entry;			// rappresentazione di ogni elemento in una directory
	struct 	stat 	st;				// buffer contenente le caratteristiche di un file
	struct 	passwd 	*usr;			// utente
	struct 	group 	*grp;			// gruppo
	struct 	tm 		*time_raw;		// data/ora "raw", da localizzare
	char	time_str[256];			// data/ora visualizzabile
	int 	i = 0;					// contatore
	
	file_fe temp;					// struct temporanea per inserimento nell'array `files`

	if((directory = opendir(path)) != NULL) {					// apertura directory
		while((entry = readdir(directory)) != NULL) {			// lettura dir.
			if(stat(entry->d_name, &st) == 0) {					// lettura del contenuto della dir.
				if(!all) {										// flag -a di ls:
					if(entry->d_name[0] != '.') {				// se non è presente allora vengono interessati i soli file visibili
						strcpy(temp.name, entry->d_name);		
						temp.mtime 	= st.st_mtime;
						temp.mode 	= st.st_mode;
						temp.links 	= st.st_nlink;
						temp.uid	= st.st_uid;
						temp.gid	= st.st_gid;
						temp.size 	= st.st_size;
						temp.ord[0]	= NULL;
						files[i] = temp;
						i++;
					}
				} else {
					strcpy(temp.name, entry->d_name);
					temp.mtime 	= st.st_mtime;
					temp.mode 	= st.st_mode;
					temp.links 	= st.st_nlink;
					temp.uid	= st.st_uid;
					temp.gid	= st.st_gid;
					temp.size 	= st.st_size;
					temp.ord[0]	= NULL;
					files[i] = temp;
					i++;
				}
			}
		}
	} else {
		printf("ls: %s (%s)\n", strerror(errno), path);
		return -1;
	}

	return 0;
}

int ls(char** param, int param_count) {
	file_fe *files;
	int 	flag_all,
			n,
			i;

	char *path = ".";
/* 	int has_path = 0;
 */	int presence[LS_OPTIONS_NUM] = {0, 0, 0, 0};

	for(i = 1; i < param_count; i++) {
		if(!strcmp(param[i], "--help")) {
			print_help();
			return 0;
		} else if(strncmp(param[i], "-", sizeof(char)) == NULL) {
			if(strpbrk(param[i], "a") != NULL)
				presence[LS_ALL] = 1;
			if(strstr(param[i], "h") != NULL)
				presence[LS_HUMANIZE] = 1;
			if(strstr(param[i], "l") != NULL)
				presence[LS_LONG] = 1;
			if(strstr(param[i], "t") != NULL)
				presence[LS_TIME] = 1;
		} /* else {
			if(!has_path) {
				path = param[i];
				has_path = 1;
			} else {
				fprintf(stderr, "ls: troppi file indicati\n");
				return -1;
			}
		} */
	}

	if(presence[LS_ALL]) {	// controllo sul flag -a
		flag_all = 1;
		n = file_count(path);			// conta tutti i file
	} else {
		flag_all = 0;
		n = visible_file_count(path);	// conta i file visibili
	}

	if(n == -1) {
		return -1;
	}

	files = malloc(sizeof(file_fe)*n);	// array che conterrà nome e data ultima modifica delle entry della directory

	if(scan_dir(path, files, flag_all) == -1)
		return -1;							// errore (in scan_dir nel dettaglio)

	if(presence[LS_TIME])		// -t
		sort_by_mtime(files, n);

	if(presence[LS_HUMANIZE])	// -h
		humanize(files, n);

	if(presence[LS_LONG]) { 	// -l
		for(i = 0; i < n; i++)
			print_long(&files[i]);
	} else {
		for(i = 0; i < n; i++)
			printf("%s\n", files[i].name);
	}
	
	free(files);
	return 0;

}
