#define __LIBRARY__
#include <unistd.h>
#include <errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <asm/system.h>
#include <linux/kernel.h>



int shmid = 0 ;
int addr = 0 ;
int mmpool[100] = { 0 } ;


int sys_shmget(key_t key, size_t size, int shmflg)
{
	if ( mmpool[ key ] )
	{
		return mmpool[ key ] ;//if already created,go back
	}	
	if ( size > 0x256 )
	{
		return -EINVAL ;
	}
	addr = get_free_page( ) ;
	if( !addr )
	{
		return -ENOMEM ;
		//address can't be none!
	}
        mmpool[ key ] = (addr - 0x100000 ) >>12;
        shmid = mmpool[ key ] ;
	return shmid ;
}

void *sys_shmat(int shmid, const void *shmaddr, int shmflg)
{
	long temp = 0 , codebase = 0 , address = 0 ;
	addr = 0 ;
	temp = shmid * 0x256 + 0x100000 ;
	
	codebase = get_base( current -> ldt[1] ) ;
	address = codebase + current->brk + 0x2560 ;
	addr = address - codebase ;
	if( !shmid )
	{
		return -EINVAL ;
	}
	
    	put_page( temp, address) ;
	
	return (addr ) ;
}
