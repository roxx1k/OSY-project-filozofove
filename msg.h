#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/*//definice typu zprav
#define MSG_C 5 //command
#define MSG_A 1 //answer
#define MSG_W 2 //warning
#define MSG_E 3 //error
#define MSG_I 4 //info

#define ZPR_CHYBA	0   // zpravy chybove
#define ZPR_INFO    1   // zpravy informativni a oznamovaci
#define ZPR_LADENI  2 	// zpravy ladici a pomocne
*/
//#define N 2



//void zprava(int msg, const char *form, ... );
int send_msg(int handle, msg_io *m);
int read_msg(int handle, msg_io *m);
//int print_msg(msg *m);
