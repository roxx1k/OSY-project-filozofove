
#include "semafor.h"
#include "msg_io.h"

int s_id[ S_COUNT ];

// id semaforu 
int sem_id;

int get_s( int key )
{
    sem_id = -1;
    
    // hledani semaforu
    sem_id = semget( key, S_COUNT, 0666 );
    //zprava(ZPR_LADENI,"ziskano sem %d", sem_id);
    m_print(M_DEBUG,"ziskano sem %d", sem_id);
    if ( sem_id < 0 )
    {
		m_print(M_DEBUG, "Semafor nebyl ziskan, pokusim se o jeho vytvoreni." );
		
		// vytvoreni semaforu
		sem_id = semget( key, S_COUNT, 0666 | IPC_CREAT );

		if ( sem_id < 0 )
		{
		    m_print(M_ERROR, "Semafor nelze vytvorit." );
		    return 1;
		}
		else
		{
		    m_print(M_DEBUG, "Semafor vytvoren" );
		    return 0;
		}      
    }
	
    return 0;
}

// nastaveni semaforu na vychozi hodnoty
int default_s()
{
    // nasteveni id semaforu filozofu
    for ( int a = 0; a < S_COUNT; a++ ) 
	s_id[a] = a+1; 
    
    // nastaveni jednotlivych semaforu na nulovou hodnotu podle s_id cisla filozofu
    for ( int i = 0; i < S_COUNT; i++ )
    {
    	if ( ( semctl( sem_id, s_id[ i ], SETVAL, 0 ) ) < 0 )
    	{	
    	    return 1; // nepodarilo se nastavit
    	}	
    }
  
    if ( ( semctl( sem_id, S_MUTEX, SETVAL, 1 ) ) < 0 )
    {
	m_print(M_ERROR, "Nelze nastavit hodnotu mutexu." );
	return 1;
    } 
    
   
}

// zvedani urciteho semaforu podle id
int up_s(int id)
{
    sembuf s_buffer = {id, S_UP, 0};    
    //s_buffer.sem_num = id;	// id semaforu v poli	
    //s_buffer.sem_op = S_UP;	// operace
    //s_buffer.sem_flg = 0;	// flag
    
    // predani operace z s_buffer
    int r = semop( sem_id, &s_buffer, 1 );
	
    if ( r < 0 )
    {
	// chyba
	m_print(M_ERROR, "Nelze zvednout hodnotu semaforu ID: %d", id);
    }
    
    return r;
}

// snizeni urciteho semaforu podle id
int down_s(int id)
{
    sembuf s_buffer;
    
    s_buffer.sem_num = id;	// id semaforu v poli	
    s_buffer.sem_op = S_DOWN;	// operace
    s_buffer.sem_flg = 0;	// flag
    
    // predani operace z s_buffer
    int r = semop( sem_id, &s_buffer, 1 );
	
    if ( r < 0 )
    {
	// chyba
	m_print(M_ERROR, "Nelze snizit hodnotu semaforu ID: %d", id);
    }
    
    return r;
}

// zjisteni hodnoty semaforu podle jeho ID
int get_s_val( int id )
{
	int r = semctl( sem_id, id, GETVAL );

	if ( r < 0 )
	{
	    m_print(M_ERROR, "Nelze zjistit hodnotu semaforu ID: %d", id );
	    r = -1;
	}

	return r;
}
