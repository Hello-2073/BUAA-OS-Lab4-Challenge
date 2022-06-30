#include "lib.h"
#include <ss.h>
#include <env.h>

#define SEMMAP 0x10000000

#define NSEM 32

void ss_init();
int ss_sem_init(sem_t semid);
void ss_sem_destroy(sem_t semid);
int ss_sem_wait(sem_t semid, u_int envid);
int ss_sem_trywait(sem_t semid, u_int envid);
void ss_sem_post(sem_t);
int ss_sem_getvalue(sem_t semid);
