#ifndef _USER_SEM_H_
#define _USER_SEM_H_

#include <ss.h>

int sem_init(sem_t *sem, int pshared, u_int value);
int sem_destroy(sem_t *sem);
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);

#endif
