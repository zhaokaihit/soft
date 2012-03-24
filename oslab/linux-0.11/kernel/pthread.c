#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#define __LIBRARY__

int pthread_attr_init(pthread_attr_t *attr)
{
	attr->tid = 0 ;
	attr->stsize = 0 ;
	return 0 ;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{  
	if ( !thread )
	{
		return -EINVAL ;
	}
	create_thread( thread ) ;
	return 0 ;
	
}

void pthread_exit(void *value_ptr)
{
	exit_thread(value_ptr) ;
	exit ( 0 ) ;
}

int pthread_join(pthread_t thread, void **value_ptr)
{
	jion_thread( thread , value_ptr ) ;
	return 0 ;
}



