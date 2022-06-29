#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#define PTHREAD_CANCELED -1

typedef u_int pthread_t;
typedef u_int pthread_attr_t;

int pthread_create(pthread_t *thread, 
                   const pthread_attr_t *attr,
                   void *(*start_routine)(void *), 
                   void *arg);

void pthread_exit(void *retval);

int pthread_cancel(pthread_t thread);

int pthread_join(pthread_t thread, void **retval);

#endif
