#ifndef PTHREAD_H
#define PTHREAD_H

typedef struct pthread_attr_t 
{
	int tid;
	int stsize;
}pthread_attr_t;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
int pthread_attr_init(pthread_attr_t *attr);
void pthread_exit(void *value_ptr);
int pthread_join(pthread_t thread, void **value_ptr);

_syscall3(int,newthread, const pthread_attr_t *,attr, void *,start_routine, void *,arg);
_syscall1(int,exitthread,void *,value_ptr);
_syscall2(int,jointhread,pthread_t,thread,void **,value_ptr);



#endif
