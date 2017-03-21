#include <sys/sem.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
//#include "msg.h"

#define N 5

#define S_COUNT 	N//(N+1)	// pocet semaforu
#define S_MUTEX		0		// mutex kriticke sekce
 		 

#define S_UP	1
#define S_DOWN	-1

//id semaforu
extern int s_id[S_COUNT];
extern int sem_id;
// funkce vytvarejici semaforu v pripade ze jiz neni vytvoren
int get_s( int key );

// nastavi semafory na defaultni hodnoty
// mutex, counter a jednotlive semafory filozofu
int default_s();

// zvedani urciteho semaforu podle id
int up_s(int id);

// snizeni urciteho semaforu podle id
int down_s(int id);

// zjisteni hodnoty semaforu podle jeho ID
int get_s_val( int id );
