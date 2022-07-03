#include "lib.h"
#include <ss.h>
#include <env.h>

#define SEMMAP 0x10000000
#define NSEM 128
#define LOG2NSEM 7

#define SEMX(a) (a&(0x7f))

void ss_init();
int ss_sem_init(u_int value, int shared, sem_t *semid);
int ss_sem_destroy(sem_t semid);
int ss_sem_wait(sem_t semid, u_int envid, int *value);
int ss_sem_trywait(sem_t semid);
int ss_sem_post(sem_t);
int ss_sem_getvalue(sem_t semid, int *value);
