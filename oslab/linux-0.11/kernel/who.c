#include <unistd.h>
#include <errno.h>
#include <asm/segment.h>

extern char pool[23] = { 0 } ;

extern int i = 0 , j = 0 ;

int sys_iam( const char* name )
{
	
	i = 0 , j = 0;
    while( get_fs_byte( name + i ) != '\0' )
	 {
	   i++ ;
	 }
    if( i > 23 )   
	{
	   return -EINVAL ;//if above 23,wrong!
	}
	
    while( ( pool[j] = get_fs_byte( name + j ) ) != '\0' )
	 {
	   j++ ;  
	 }
    return j ;
}

int sys_whoami( char* name , unsigned int size )
{
	
    for( i = 0 ; i < 23 ; i++ )
	{
	   if( pool [i] == '\0' )
	       break ;
	}
	
    if( i > size )
	{
	   return -EINVAL ;
	}
	
    for( j = 0 ; j < i ; j++ )
	{
	   put_fs_byte( pool[j] , ( name + j ) );
    	}
    return i ;
}
