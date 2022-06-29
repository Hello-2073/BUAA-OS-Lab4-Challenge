#include "lib.h"
#include <ss.h>
#include <env.h>

#define SEMMAP 0x10000000

#define SEMMAX 0x40000000

#define NSEM SEMMAX / sizeof(struct Sem)

void ss_init();
int ss_sem_init();
void ss_sem_destroy(Sem *psem);
int ss_sem_wait(sem_t semid, u_int envid);
int ss_sem_trywait(sem_t semid, u_int envid);
void ss_sem_post(sem_t);
int ss_sem_getvalue(sem_t semid);
