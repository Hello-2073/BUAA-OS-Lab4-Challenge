#include "lib.h"
#include "ss.h"

Sem *sems = (Sem *)SEMMAP;
static struct Sem_list sem_free_list;

void 
ss_init()
{
	int i;
	syscall_mem_alloc(0, sems, PTE_R | PTE_V);
	LIST_INIT(&sem_free_list);
	for (i = NSEM - 1; i >= 0; i--) {
		LIST_INSERT_HEAD(&sem_free_list, &sems[i], sem_link);
	}
}

static int
semid2sem(sem_t semid, Sem **sem)
{
	if ((semid >> LOG2NSEM) == 0) {
		return -EINVAL;
	}
	*sem = sems + SEMX(semid);
	return 0;
}

int
ss_sem_init(u_int value, int shared, sem_t *semid)
{
	Sem *sem;
	if (LIST_EMPTY(&sem_free_list)) {	
		return -EINVAL;
	}
	sem = LIST_FIRST(&sem_free_list);
	LIST_REMOVE(sem, sem_link);
	sem->value = (int)value;
	LIST_INIT(&(sem->queue));
	// writef("a new semm id %d, value %d\n", (psem - sems) | (1 << 7), psem->value);
	*semid = (sem - sems) | (1 << LOG2NSEM);
	return 0;
}

int
ss_sem_destroy(sem_t semid)
{
	Sem *sem;
	int r;
	r = semid2sem(semid, &sem);
	if (r < 0) {
		return r;
	}
	LIST_INSERT_HEAD(&sem_free_list, sem, sem_link);
	return 0;
}

int
ss_sem_wait(sem_t semid, u_int envid, int *value)
{
	Sem *sem;
	int r;
	r = semid2sem(semid, &sem);
	if (r < 0) {
		return r;
	}
	*value = --sem->value;
    if (sem->value < 0) {
		LIST_INSERT_TAIL(&(sem->queue), &envs[ENVX(envid)], env_link);
    }
	return 0;
}

int
ss_sem_trywait(sem_t semid)
{
	Sem *sem;
	int r;
	r = semid2sem(semid, &sem);
	if (r < 0) {
		return r;
	}
	if (sem->value > 0) {
		sem->value--;
		return 0;
	} else {
		return -EAGAIN;
	}
}

int
ss_sem_post(sem_t semid)
{
	Sem *sem;
	int r;
	r = semid2sem(semid, &sem);
	if (r < 0) {
		return r;
	}
	struct Env *e;
	sem->value++;
	if (sem->value == 0) {
		e = LIST_FIRST(&(sem->queue));
		LIST_REMOVE(e, env_link);
		// writef("%d out, %d : %d\n\n", e->env_id, semid, psem->value);
		ipc_send(e->env_id, 0, 0, 0);
	}
	return 0;
}

int
ss_sem_getvalue(sem_t semid, int *value)
{
	Sem *sem;
	int r;
	r = semid2sem(semid, &sem);
	if (r < 0) {
		return r;
	}
	*value = sem->value;
	return 0;
}
