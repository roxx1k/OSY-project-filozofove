#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include "msg.h"
#include "msg_io.h"

/*
int send_msg(int handle, msg *m)
{
    char buf[256];
    bzero(buf,sizeof(buf));
    sprintf(buf,"%1d%2d:%s\n",m->typ,m->kod,m->text);
    printf("zprava: %s",buf);
    int l = write(handle,buf,strlen(buf)+1);
    
    if(l < 0) return 1;
    return 0;
}
int read_msg(int handle, msg *m)
{
    char buf[256];
    bzero(buf,sizeof(buf));
    
    int l = read(handle,buf,sizeof(buf));
    if(l < 0) return 1;
    buf[l] = '\0';
    //-------------------printf("%1d%2d:%s",,&m->typ,&m->kod,m->text);
    sscanf(buf,"%1d%2d:%s",&m->typ,&m->kod,m->text);
    return 0;
}
int print_msg(msg *m)
{
    char text[512];
    char * t;
      
    switch (m->typ)
    {
        case MSG_C:   t = (char *)"Command";
            break;
        case MSG_A:   t = (char *)"Answer";
            break;   
        case MSG_W:   t = (char *)"Warning";
            break;
        case MSG_E:   t = (char *)"Error";
            break;
        case MSG_I:   t = (char *)"Information";
            break;
        default:   break;
    }
      
    sprintf(text, "%s ID %d %d: %s",t, m->typ ,m->kod,m->text);     // zpravy
    printf("\nZprava:\n%s\n",text);
}
*/

int send_msg(int handle, msg_io *msg)
{
    char buf[256];
    bzero(buf,sizeof(buf));
    buf[0]= msg->typ;
    if(msg->NN >=0)sprintf(buf+1,"%02d",msg->NN);
    sprintf(buf+strlen(buf),":%s",msg->text);

    //sprintf(buf,"%1c%2d:%s",msg->typ,msg->NN,msg->text);
    m_print(M_INFO,"zprava: %s\n",buf);
    int l = write(handle,buf,strlen(buf)+1);
    
    if(l < 0) return 1;
    return 0;
}
int read_msg(int handle, msg_io *msg)
{
    char buf[256];
    bzero(buf,sizeof(buf));
    
    int l = read(handle,buf,sizeof(buf)+1);
    if(l < 0) return -1;
    //buf[l] = '\0';
        
    sscanf(buf,"%c%02d:%[^\n]%*c\n",&msg->typ,&msg->NN,msg->text);
    m_print(M_INFO,"%c%2d:%s",msg->typ,msg->NN,msg->text);
    
    return msg->NN;
}



