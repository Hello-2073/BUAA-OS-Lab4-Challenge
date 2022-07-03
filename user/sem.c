#include "lib.h"
#include "sem.h"
#include <env.h>

static int
semipc(u_int type, void *semreq, u_int dstva, u_int *perm)
{
    u_int whom;
    ipc_send(envs[0].env_id, type, (u_int)semreq, PTE_V | PTE_R);
    return ipc_recv(&whom, dstva, perm);;
}

int 
sem_init(sem_t *sem, int pshared, u_int value)
{
	struct Semreq_init *req = (struct Semreq_init *)syscall_get_thread_page();
	req->pshared = pshared;
	req->value = value;
	int r = semipc(SEMREQ_INIT, req, 0, 0);
	*sem = req->semid;
	return r;
}

int 
sem_destroy(sem_t *sem)
{
	struct Semreq_destroy *req = (struct Semreq_destroy *)syscall_get_thread_page();
	req->semid = *sem;
	return semipc(SEMREQ_DESTROY, req, 0, 0);
}

int 
sem_wait(sem_t *sem)
{
	struct Semreq_wait *req = (struct Semreq_wait *)syscall_get_thread_page();
	req->semid = *sem;
	return semipc(SEMREQ_WAIT, req, 0, 0);
}

int sem_trywait(sem_t *sem)
{
	struct Semreq_wait *req = (struct Semreq_wait *)syscall_get_thread_page();
    req->semid = *sem;
	return semipc(SEMREQ_TRYWAIT, req, 0, 0);
}

int sem_post(sem_t *sem)
{
	struct Semreq_post *req = (struct Semreq_post *)syscall_get_thread_page();
	req->semid = *sem;
    return semipc(SEMREQ_POST, req, 0, 0);
}

int sem_getvalue(sem_t *sem, int *sval)
{
	struct Semreq_getvalue *req = (struct Semreq_getvalue *)syscall_get_thread_page();
    req->semid = *sem;
    int r = semipc(SEMREQ_GETVALUE, req, 0, 0);
	*sval = req->value;
	return 0;
}
