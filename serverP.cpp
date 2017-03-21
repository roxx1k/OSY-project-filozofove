//***************************************************************************
//
// Predmet: Operacni systemy
// Autor: Jan Hires
// Login: hir0004 
// Rok: 2013
//
// Implementace rozhrani sockets.
//
// Program pracuje jako server, ktery povoli pripojeni klienta.
// Povinny argument programu je cislo portu, na kterem bude server poslouchat.
//
//***************************************************************************

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

//#include "msg.h"
#include "semafor.h"
#include "msg_io.h"
#include "filozof.h"

#define PREMYSLI 0
#define HLADOVI 1
#define JI 2

//#define N 1     //pocet mist u stolu

using namespace std;

//const int N = 5; //pocet mist u stolu
//int debug =0;
//int s_id[S_COUNT]; 

struct sdilena
{
    int stav[N]; //premysli, ji, hladovi
    int zidle[N]; //volna x obsazena
    int f_PID[N]; //PID filozofa (procesu)
};
//id sdilene pameti
int shm_id = -1;


sdilena *sdilen;
//int mistUStolu = N;
int pocetVolnychZidli = N;
char tmpGlob[512];
int sock, newsock,portno;
struct sockaddr_in serv_addr, cli_addr; // adresa serveru a klienta

//***********************************************************
void help(char *name) {
    printf(
            "\n"
            "  Server - stul.\n"
            "\n"
            "  Pouziti: [-d -h] cislo-portu\n"
            "\n"
            "    -d  ladici mod\n"
            "    -h  tato napoveda\n"
            "     pro ukonceni servru zadejte 'quit' "
            "\n", name);

    exit(0);
}
//***********************************************************
// nastavi data ve sdilene pameti podle ID na defaultni hodnoty
void set_s_default(int num)
{
    sdilen->stav[num] = PREMYSLI;
    sdilen->zidle[num] = 0;      // zidle je volna    
    
    m_print(M_DEBUG,"Defaultne nastaven filozof ID: %d", num);
}

// vytvori a namapuje sdilenou pamet, pokud jiz neni vytvorena
int get_shared_mem(int key)
{
    sdilen = NULL;
    shm_id = -1;
    
    shm_id = shmget( key, sizeof( sdilena ), 0666 );

    if ( shm_id < 0 )
    {
    // sdilena pamet pravdepodobne neexistuje
    m_print(M_DEBUG, "Sdilenou pamet nelze ziskat, pokusim se o jeji vytvoreni" );

    // vytvoreni sdilene pameti
    shm_id = shmget( key, sizeof( sdilena ), 0666 | IPC_CREAT );
    if ( shm_id < 0 )
    {
        m_print(M_ERROR, "Sdilenou pamet nelze vytvorit!" );
        return 1;
    }
    else
        m_print(M_DEBUG, "Sdilena pamet vytvorena.");
    }
    
    // namapovani pameti na adresu
    
    sdilen = (sdilena *) shmat(shm_id, NULL, 0);
    
    m_print(M_DEBUG, "Sdilena pamet byla pripojena na adresu %p\n", sdilen);
    
    
    return 0;
}

//odstrani sdilenou pamet
int clean_shared_mem()
{
    m_print(M_DEBUG, "Odpojeni sdilene pameti..." );
    
    int ret = shmctl( shm_id, IPC_RMID, NULL );
    
    if ( ret < 0 )
        m_print(M_ERROR, "Zruseni sdilene pameti neprobehlo!" );
    else
        m_print(M_DEBUG, "Sdilena pamet byla zrusena." );
}

/*void zprava(int msg, const char *form, ...) {
    const char *out_fmt[] = {"CHYBA:  (%d-%s) %s\n",
        "INFO:   %s\n",
        "LADENI: %s\n"};

    char buf[ 1024 ], out[ 1024 ];
    va_list arg;

    if (msg == ZPR_LADENI && !debug) return;

    va_start(arg, form);
    vsprintf(buf, form, arg);
    va_end(arg);

    switch (msg) {
        case ZPR_INFO:
        case ZPR_LADENI:
            sprintf(out, out_fmt[ msg ], buf);
            fputs(out, stdout);
            break;

        case ZPR_CHYBA:
            sprintf(out, out_fmt[ msg ], errno, strerror(errno), buf);
            fputs(out, stderr);
            break;
    }
}*/

//zpracovani znaku predavane zpravy
void zpracuj(string zprava)
{
    char typZpravy;
    int cisloZpr;
    char text [251];    

    typZpravy = zprava[0];
    if(zprava[1] != ':') 
    {
        //cislo zpravy je zadano
        char tmp[2];
        tmp[0] = zprava[1];
        tmp[1] = zprava[2];
        cisloZpr = atoi(tmp);
        
        if(zprava[4] != '\n')
        {
            int i=0;
            
            while(zprava[4 + i] != '\n'){
                text[i] = zprava[4+i];
                i++;
            }
            text[i] = '\0';
        }
        else text[0] ='\0';
        printf("typ zpravy: %c\ncislo zpravy: %d\ntext: %s\n",typZpravy,cisloZpr,text);
    }
    //cislo zpravy nebylo zadano
    else
    {
        if(zprava[2] != '\n')
        {
            int i=0;
            
            while(zprava[2+i] != '\n'){
                text[i] = zprava[2+i];
                i++;
            }
            text[i] = '\0';
        }
        printf("typ zpravy: %c\ntext: %s\n",typZpravy,text);
    }   
}
// inicializace serveru
int init()
{
    // vytvoreni socketu pro komunikaci    
    sock = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sock == -1 )
    {
    m_print(M_ERROR,"CHYBA, nelze vytvorit socket");
    exit( 1 );
    }
    else
    m_print(M_DEBUG,"Socket uspesne vytvoren.");
    
    
    bzero((char *) &serv_addr, sizeof(serv_addr));  // vynuluje serv_addr
    
    
    // nastaveni adresy-------------------------------------------------
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    //------------------------------------------------------------------
    
    int opt = 1;
    if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ) ) < 0 )
    m_print(M_ERROR, "Nelze nastavit vlastnosti socketu." );
    else
    m_print(M_DEBUG, "Vlastnosti socketu nastaveny.");
    
    
    // pripojeni adresy k otevrenemu socketu
    if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    m_print(M_ERROR,"CHYBA, nepodarilo se adresovat socket");
    else
    m_print(M_INFO,"Server ceka na klienta, prikazem quit jej ukoncite\n");
    
    // povoleni odposlechu na urcenem portu
    if ( listen(sock,1) < 0 )
    {
    m_print(M_ERROR, "Nelze naslouchat na zadanem portu." );
    close( sock );
    return 1;
    }
    else
    m_print(M_INFO, "Server nasloucha na portu %d", portno);
    
    
   if (get_s(234) == 0)
   {
        if (default_s() != 0)
        {
         return 1;
        }
        else
        m_print(M_DEBUG, "Defaultni hodnoty semaforu uspesne nastaveny.");
   }
   else
   {
        return 1;
   }
    
   return 0;
}
//**************************************************************************
int muzu_jist(int id)
{
    //zjisteni cisla sousedu
    int l,r;
    if (id == 0)
        l = (N-1);
    else
        l = id-1;
    
    if (id == (N-1))
        r = 0;
    else
        r = id + 1;

    if ( sdilen->stav[id] == HLADOVI &&
      sdilen->stav[l] != JI &&
      sdilen->stav[r] != JI )
    {
    // nastaven stav na jezeni
    sdilen->stav[id ] = JI;
    // zvednuti prislusneho semaforu (0, 1 - Mutex, Counter)
    up_s( s_id[ id + 2 ] );
    }  

}
void vem_vidlicky( int id )
{
    
    down_s( S_MUTEX );
    
    m_print(M_DEBUG, "Mutext snizen.");
    
    // kriticka sekce
    sdilen->stav[ id ] = HLADOVI;      // filozof zjistil, ze je hladovy
    muzu_jist( id );              // pokus o ziskani dvou vidlicek
    m_print(M_DEBUG, "Muzu jist");
    // konec kriticke sekce
    up_s( S_MUTEX );
    m_print(M_DEBUG, "Mutext zvysen.");
    down_s( s_id[ id + 2 ] );           
    m_print(M_DEBUG, "Semafor ID: %d ma hodnotu %d\n", id + 2, s_id[ id + 2 ]);
}

void poloz_vidlicky( int id )
{
    int l,r;
  
    if (id == 0)
    l = (N-1);
    else
    l = id-1;
    
    if (id == (N-1))
    r = 0;
    else
    r = id + 1;
  
    down_s( S_MUTEX );
    sdilen->stav[ id ] = PREMYSLI;    // filozof premysli
    // aktualni filozof odlozil vidlicky cimz muze odblokovat jednoho ze svych sousedu
    muzu_jist( l );           // muze jist levy soused ? 
    muzu_jist( r );           // muze jist pravy soused ?
    up_s( S_MUTEX );
}
//*************************************************************************
int VolnaZidle(int pole []) {
    int l,r;
    for (int i = 0; i < N; i++) {
        if(i == 0)
            l = (N-1);
        else{
            l = (i-1);
        }

        if(i == (N-1))
            r = 0;
        else{
            r = (i+1);
        }

        if((pole[l]==0) && (pole[r] == 0)){

            if (pole[i] == 0) 
            {
              //sdilen->zidle[i]=1;
              return i;
            }
        }
        else{
            continue;            
        }
        
    }
    for (int i = 0; i < N; i++) {
        if (pole[i] == 0) {
            return i;
        }
    }
    return -1;
}
int recv_proto_msg( int handle, char *type, int *NN, char *text ){}
int send_proto_msg( int handle, char type, int NN, const char *text ){}
int send_msg(int handle, msg_io *msg)
{
    char buf[256];
    bzero(buf,sizeof(buf));
    buf[0]= msg->typ;
    if(msg->NN >=0)sprintf(buf+1,"%02d",msg->NN);
    //m_print(M_INFO,"%s -- %d -- %d",msg->text,strlen(msg->text),sizeof(msg->text));
    char textik[20]; //msg->text;
    sprintf(textik,"%s",msg->text);
    //m_print(M_INFO,"%d",strlen(buf));
    strcat(buf,":");
    //m_print(M_INFO,"zprava: %s\n",buf); 
    for (int i = 0; i < strlen(msg->text); i++)
    {       
       buf[strlen(buf)] = msg->text[i];
       //printf("_%c",msg->text[i]); 
       //sprintf(buf+strlen(buf),"%c",textik[i]);
    }
    strcat(buf,"\n");
    //sprintf(buf,"%1c%2d:%s",msg->typ,msg->NN,msg->text);
    //m_print(M_INFO,"zprava: %s\n",buf);
    int l = write(handle,buf,strlen(buf));
    m_print(M_INFO,"posilam: %s\n",buf);
    if(l < 0) return 1;
    return 0;
}
int read_msg(int handle, msg_io *msg)
{
    char buf[256],tmpLocal[256];
    bzero(buf,sizeof(buf));
    bzero(tmpLocal,sizeof(tmpLocal));
    //memset(tmpGlob,'\0',sizeof(tmpGlob));
    
    char * pch;
    pch = strchr(tmpGlob,'\n');

    while(pch == NULL){
        int l = read(handle,buf,sizeof(buf)+1);
        if(l < 0) return -1;
        
        if((buf[0]== 'I')){
            continue;            
        }
        strcat(tmpGlob,buf);
        pch = strchr(tmpGlob,'\n'); 
    } 

    bzero(buf,sizeof(buf));
    char znak = 'a';
    int counter = 0;
    
    while(znak != '\n')
    {
        znak = tmpGlob[counter];
        buf[counter] = znak;
        counter++;
    }
    strcat(tmpLocal,pch+1);
    bzero(tmpGlob,sizeof(tmpGlob));
    strcat(tmpGlob,tmpLocal);

    sscanf(buf,"%c%02d:%[^\n]%*c",&msg->typ,&msg->NN,msg->text);

    m_print(M_INFO,"precteno: %c%2d:%s",msg->typ,msg->NN,msg->text);
    
    return msg->NN; 
}

msg_io formatuj_zpravu(msg_io *retMsg, char typ,int NN,const char *text){
    
    retMsg->typ = typ;
    m_print(M_DEBUG,"%c",retMsg->typ);

    retMsg->NN = NN;
    m_print(M_DEBUG,"%2d",retMsg->NN);

    /*for(int i=0;i<strlen(text);i++)
    {
        retMsg->text[i] = text[i];
    }*/
    sprintf(retMsg->text,"%s",text);
    m_print(M_DEBUG,"naformatovan text: %s",retMsg->text);

    return *retMsg;

}

//***************************************************************************
int main(int argn, char **arg) {
    printf("---- Filozofove ----\nServer pomoci procesu\n");
    int sock_client = 0;    
    char buf[PROTO_LEN];
    int port = 0;

    if (argn <= 1) help(*arg);

    // zpracovani prikazoveho radku
    for (int i = 1; i < argn; i++) {
        if (!strcmp(arg[ i ], "-d"))
            debug = 1;

        if (!strcmp(arg[ i ], "-h"))
            help(*arg);

        if (*arg[ i ] != '-' && !port) {
            port = atoi(arg[ i ]);
            break;
        }
    }

    if (!port) {
        m_print(M_INFO, "Chybejici nebo spatne cislo portu!");
        help(*arg);
    }

    m_print(M_INFO, "Server bude poslouchat na portu: %d.", port);

    // vytvoreni socketu
    int sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_listen == -1) {
        m_print(M_ERROR, "Nelze vytvorit socket.");
        exit(1);
    }
    else m_print(M_INFO,"socket vytvoren\n");

    in_addr addr_any = {INADDR_ANY};
    sockaddr_in srv_addr;

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    srv_addr.sin_addr = addr_any;

    // socket smi znovu okamzite pouzit jiz drive pouzite cislo portu
    int opt = 1;
    if (setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof ( opt)) < 0)
        m_print(M_ERROR, "Nelze nastavit vlastnosti socketu.");

    // prirazeni adresy a portu socketu
    if (bind(sock_listen, (const sockaddr *) &srv_addr, sizeof ( srv_addr)) < 0) {
        m_print(M_ERROR, "Prirazeni adresy selhalo.");
        close(sock_listen);
        exit(1);
    }else m_print(M_DEBUG,"bind ok");

    // aplikace bude naslouchat na zadanem portu
    if (listen(sock_listen, 1) < 0) {
        m_print(M_ERROR, "Nelze naslouchat na pozadovanem portu.");
        close(sock_listen);
        exit(1);
    }
    else m_print(M_DEBUG,"listen ok");

    //ziskani sdilene pameti
    if (get_shared_mem(0x245) == 0)
    {
        // nastaveni vsech hodnot na defaultni hodnoty
        for (int i = 0; i < N; i++)
            set_s_default(i);    
        
    }
    m_print(M_DEBUG,"ziskavani semaforu: ");
    if (get_s(234) == 0)
    {
        m_print(M_DEBUG,"semafor ziskan");
        if (default_s() != 0)
        {
         return 1;
        }
        else
        m_print(M_DEBUG, "Defaultni hodnoty semaforu uspesne nastaveny.");
    }
    else
    {
        m_print(M_ERROR,"semafor neziskan");
        return 1;
    }
    //nastaveni defaultniho stavu mist u stolu
    for (int i = 0; i < N; i++)
        set_s_default(i);
    // jedeme!
    while (1) {
        char buf[ 100 ];
	bzero(tmpGlob,sizeof(tmpGlob));
        // mnozina hadlu
        fd_set read_wait_set;
        // vynulovani mnoziny
        FD_ZERO(&read_wait_set);

        // pridani handlu stdin
        FD_SET(STDIN_FILENO, &read_wait_set);

        // vyber handlu - listen nebo spojeni se serverem?
        // cekame na spojeni, nebo na data od serveru?

        if (sock_client)
            FD_SET(sock_client, &read_wait_set);
        else
            FD_SET(sock_listen, &read_wait_set);

        // cekame na data u nektereho handlu
        if (select(MAX(sock_client, sock_listen) + 1,
                &read_wait_set, 0, 0, 0) < 0) break;

        // data na stdin?
        if (FD_ISSET(STDIN_FILENO, &read_wait_set)) {
            // cteme data ze stdin
            int l = read(STDIN_FILENO, buf, sizeof ( buf));
            if (l < 0)
                m_print(M_ERROR, "Nelze cist data ze stdin.");
            else
                m_print(M_DEBUG, "Nacteno %d bytu ze stdin.", l);

            // posleme data klientovi
            l = write(sock_client, buf, l);

            if (l < 0)
                m_print(M_ERROR, "Nelze zaslat data klientovi.");
            else {
                m_print(M_DEBUG, "Odeslano %d bytu klientovi.", l);
            }
        }// nove spojeni od klienta?
        else if (FD_ISSET(sock_listen, &read_wait_set)) {
            sockaddr_in rsa;
            int rsa_size = sizeof ( rsa);

            // prijmeme nove spojeni
            sock_client = accept(sock_listen, (sockaddr *) & rsa, (socklen_t *) & rsa_size);
            //newsock = accept(sock_listen, (sockaddr *) & rsa, (socklen_t *) & rsa_size);
            
            if (sock_client < 0) {
                m_print(M_ERROR, "Spojeni se nezdarilo.");
                close(sock_listen);
                exit(1);
            }
            int child = fork();

            if(child < 0){
                m_print(M_ERROR,"Chyba forku!!");
                exit(0);
            }
            //kod pro potomka
            else if(child == 0){
                close(sock_listen);
                /*down_s(S_MUTEX);
                int volnaZidle = VolnaZidle(sdilen->zidle);
                up_s(S_MUTEX);
                */
            
            msg_io newMsg,filMsg;// = new msg_io; 
        
            int id,rcvNN,l;
            
            m_print(M_INFO,"xxxxxxxxxxxxxxxxxxxx");

            //newMsg = formatuj_zpravu(&newMsg,'C',CI_Prichazim,CS_Prichazim);
            //if(cekej_zpravu(sock_client,&newMsg)<=0)printf("%c %02d %s",newMsg.typ,newMsg.NN,newMsg.text);
            
            rcvNN = read_msg(sock_client,&filMsg);
            
            /*int pole[2];
            int number = filMsg.NN;
            for (int i = 0; i < 2 ; i++)
            {
                int pozice = 2-i-1;
                pole[pozice]= number % 10;
                number = number/10;
                //m_print(M_INFO,"%d",pole[pozice]);
            }
            for (int i = 0; i < 2; i++)
            {
                m_print(M_INFO,"%d",pole[i]);
            }
        */
            //m_print(M_INFO,"%d %d %d",filMsg.NN,pole[0],pole[1]);
            if(rcvNN == CI_Prichazim) //61
            {
                        
                down_s(S_MUTEX);    //mutex pro kontrolu podminky
                if(pocetVolnychZidli > 0)
                {    
                    int volnaZidle = VolnaZidle(sdilen->zidle);
                    pocetVolnychZidli--;
                    id = volnaZidle;
                    m_print(M_INFO,"sedis na zidli %i",id);
                    up_s(S_MUTEX);
                           

                    down_s(S_MUTEX);
                    sdilen->zidle[id] =1;
                    //m_print(M_INFO,"obsazuji zidli %d, volnych mist u stolu %d",id,pocetVolnychZidli);
                    up_s(S_MUTEX);
               

                    sprintf(buf,AS_ZidleX,id);               
                    filMsg = formatuj_zpravu(&filMsg,'A',AI_ZidleX,buf);
                    l = send_msg(sock_client,&filMsg);  //62
                
                    //int l =read_msg(sock_client,&filMsg); 
                    //m_print(M_INFO,"%02d  --- %02d",filMsg.NN,l);
                    do {  //prubeh vecere
                        //char buf[ 256 ];
                        //l = read(sock_client, buf, sizeof ( buf));
                        //msg_io readMsg;
                        l = read_msg(sock_client,&filMsg);
                        //m_print(M_INFO,"---- %c %d  %s ----",filMsg.typ,filMsg.NN,filMsg.text);
                        //m_print(M_INFO,"---- %c %d  %s ----",readMsg.typ,readMsg.NN,readMsg.text);
                        
                        //zjisteni zda dostal filozof hlad                
                        if(l == CI_Hladovim) //63
                        {
                            m_print(M_INFO,"filozof na zidli %i dostal hlad",id);
                            vem_vidlicky(id);
                            m_print(M_INFO,"vidlicky zajisteny");

                            //msg_io jezMsg;
                            formatuj_zpravu(&filMsg,'A', AI_Jez, AS_Jez );
                            l = send_msg(sock_client,&filMsg);  //64
                            if(l < 0)printf("zprava JEZ neodeslana\n");

                            //msg_io hotovoMsg;
                            bool znovu = true;
                            while(znovu)
                            {
                                l = read_msg(sock_client,&filMsg);                
                                if(filMsg.NN == CI_Premyslim) //65
                                {
                                    m_print(M_INFO,"filozof %i dojedl, pokladam vidlicky",id);
                                    poloz_vidlicky(id);
                                    //m_print(M_INFO,"vidlicky polozeny");

                                    formatuj_zpravu(&filMsg,'A', AI_Dojezeno, AS_Dojezeno );
                                    l = send_msg(sock_client,&filMsg);  //66
                                    if(l < 0)printf("zprava ODCHAZIM neodeslana\n");
                                    znovu = false;
                                }
                                /*else
                                {
                                  m_print(M_INFO,"vidlicky nelze polozit!!");
                                  
                                  //exit(1); 
                                } */
                            }
                            
                        }
                        else if(filMsg.NN == CI_Odchazim){  
                            formatuj_zpravu(&filMsg,'A', AI_Nashledanou, AS_Nashledanou );
                            l = send_msg(sock_client,&filMsg);  //68
                            if(l < 0)printf("zprava NASHLEDANOU neodeslana\n");                    


                            break;
                        }
                        else {
                            //m_print(M_INFO,"obdrzen spam s cislem: %d",l);
                            continue;
                            //exit(1);
                            //break;
                        }
                        
                        
                    } while (sock_client != 0);
                    
                    down_s(S_MUTEX);
                    sdilen->zidle[id]= 0;
                    pocetVolnychZidli++;
                    up_s(S_MUTEX);
            
                }
                else{
                    up_s(S_MUTEX);
                    m_print(M_INFO,"neni misto u stolu");
                    formatuj_zpravu(&filMsg,'A',EI_Obsazeno,ES_Obsazeno);            
                    l = send_msg(sock_client,&filMsg);  

                    l = read_msg(sock_client,&filMsg);             
                }
                
            }
            else{
                m_print(M_INFO,"nebyl prijat pozadavek na usednuti ke stolu");
                formatuj_zpravu(&filMsg,'A', AI_Nashledanou, AS_Nashledanou );
                l = send_msg(sock_client,&filMsg);  //68
            }
            m_print(M_INFO,"ooooooooooooooooooooooo");

                
            /*
                msg *filMsg = new msg;
                int l = read_msg(newsock,filMsg);
                int id;

                if ( (!strncmp( filMsg->text, "CHCI_SEDNOUT", 12 )) && (filMsg->typ == MSG_C) )
                {
                    zprava(ZPR_INFO, "filozof si chce sednout");
                    if(pocetVolnychZidli > 0)
                    {        
                        int volnaZidle;
                        down_s(S_MUTEX);
                        volnaZidle = VolnaZidle(sdilen->zidle);
                        sdilen->zidle[volnaZidle] = 1;                                               
                        pocetVolnychZidli--;
                        up_s(S_MUTEX);
                        //zprava(ZPR_INFO,"zidle cislo %d byla prave obsazena",volnaZidle); 

                        id = volnaZidle;
                        zprava(ZPR_INFO,"sedis ted na zidli %i",id);        
                  
                        filMsg->typ = MSG_A;
                        filMsg->kod = 20;
                        sprintf(filMsg->text, "SEDEJ%d",id );
                
                        //sprintf(filMsg->text, "SEDEJ" );
                        int l = send_msg(newsock,filMsg);

                        do { 
                            char buf[ 256 ];
                            //l = read(sock_client, buf, sizeof ( buf));
                            
                            l = read_msg(newsock,filMsg);
                            //zjisteni zda dostal filozof hlad
                            if ( (!strncmp( filMsg->text, "MAM_HLAD", 8 )) && (filMsg->typ == MSG_C) )
                                zprava(ZPR_INFO,"filozof na zidli %i dostal hlad",id);
                            else if ( (!strncmp( filMsg->text, "ODCHAZIM", 8 )) && (filMsg->typ == MSG_C) )
                                break;
                            else {
                                break;
                                zprava(ZPR_INFO,"neosetrena zprava");
                                
                            }
                            vem_vidlicky(id);
                            zprava(ZPR_INFO,"vidlicky zajisteny");

                            msg *jezMsg = new msg;
                            jezMsg->typ = MSG_A;
                            jezMsg->kod = 66;
                            sprintf(jezMsg->text, "JEZ" );
                            l = send_msg(newsock,jezMsg);
                            if(l < 0)printf("zprava JEZ neodeslana\n");

                            msg *hotovoMsg = new msg;
                            l = read_msg(newsock,hotovoMsg);
                            if ( (!strncmp( hotovoMsg->text, "HOTOVO", 6 )) && (hotovoMsg->typ = MSG_C) ) 
                            {
                                zprava(ZPR_INFO,"filozof %i dojedl, pokladam vidlicky",id);                    
                            }
                            poloz_vidlicky(id);
                            zprava(ZPR_INFO,"vidlicky polozeny");
                            
                        } while (newsock != 0);
                        down_s(S_MUTEX);
                        sdilen->zidle[id]= 0;
                        pocetVolnychZidli++;
                        up_s(S_MUTEX);
                    }
                    else{
                        zprava(ZPR_INFO,"neni misto");
                        filMsg->typ = MSG_A;
                        filMsg->kod = 29;
                        sprintf(filMsg->text, "PLNO" );
                        int l = send_msg(newsock,filMsg);               
                    }
                }
                //l = read_msg(sock_client,filMsg);
                if ( (!strncmp( filMsg->text, "ODCHAZIM", 8 )) && (filMsg->typ == MSG_C) )
                {
                    filMsg->typ = MSG_A;
                    filMsg->kod = 89;
                    sprintf(filMsg->text, "SBOHEM" );
                    int l = send_msg(newsock,filMsg);
                    newsock = 0;
                }*/
                sock_client=0;
            }

            //kod pro rodice
            else if(child > 0){
                close(sock_client);
                sock_listen =0;

            }

            

                //filozof(fil_params);

                //pthread_create(&vlakna[volnaZidle], NULL, filozof, &fil_params);
                
                /*
                if (pthread_create(&vlakna[volnaZidle], NULL, filozof, &fil_params) == 0)
                {
                    printf("filozof prisel ke stolu\n");                    
                } 
                else
                {
                    //close(newsock);
                    newsock = 0;
                }*/
                //close(newsock);
                //newsock = 0;
 
            
            //close(sock_listen);
            /*uint lsa = sizeof ( srv_addr);

            // ziskani vlastni IP
            getsockname(sock_client, (sockaddr *) & srv_addr, &lsa);

            zprava(ZPR_INFO, "Moje IP: '%s'  port: %d",
                    inet_ntoa(srv_addr.sin_addr), ntohs(srv_addr.sin_port));

            // ziskani IP klienta
            getpeername(sock_client, (sockaddr *) & srv_addr, &lsa);

            zprava(ZPR_INFO, "Klient IP: '%s'  port: %d",
                    inet_ntoa(srv_addr.sin_addr), ntohs(srv_addr.sin_port));
            */
            //zprava(ZPR_INFO, "Zadejte 'quit' pro ukonceni procesu serveru.");

        }
        // data od klienta?
        //else if (FD_ISSET(sock_client, &read_wait_set)) {  }

        // byl pozadavek na ukonceni prace serveru?
        if (!strncasecmp(buf, "quit", 4)) {
            close(sock_client);
            m_print(M_INFO, "Byl zadan pozadavek 'quit'. Server konci svou cinnost.\n");
            break;
        }

    }

    close(sock_listen);
    clean_shared_mem();

    return 0;
}
