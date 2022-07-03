#ifndef _SS_H_
#define _SS_H_

#include <env.h>

#define EINVAL 1
#define EAGAIN 2 

#define SEMREQ_INIT     1
#define SEMREQ_DESTROY  2
#define SEMREQ_WAIT     3
#define SEMREQ_TRYWAIT  4
#define SEMREQ_POST     5
#define SEMREQ_GETVALUE 6

typedef struct Sem {
    int value;
	int shared;
    struct Env_list queue;
	LIST_ENTRY(Sem) sem_link;
} Sem;

typedef u_int sem_t;

LIST_HEAD(Sem_list, Sem);

struct Semreq_init
{
    int pshared;
    u_int value;
	
	sem_t semid;
};

struct Semreq_destroy
{
    sem_t semid;
};

struct Semreq_wait
{
    sem_t semid;
	int value;
};

struct Semreq_trywait
{
    sem_t semid;
	int value;
};

struct Semreq_post
{
    sem_t semid;
};

struct Semreq_getvalue
{
    sem_t semid;
	
	u_int value;
};

#endif
