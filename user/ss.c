#include "lib.h"
#include "ss.h"

Sem *sems = (Sem *)SEMMAP;
static struct Sem_list sem_free_list;

void 
ss_init()
{
	int i;
	LIST_INIT(&sem_free_list);
	for (i = NSEM - 1; i >= 0; i--) {
		LIST_INSERT_HEAD(&sem_free_list, &sems[i], sem_link);
	}
}

int
ss_sem_init()
{
	Sem *psem;
	if (LIST_EMPTY(&sem_free_list)) {	
		return 0;
	}
	psem = LIST_FIRST(&sem_free_list);
	LIST_REMOVE(psem, sem_link);
	LIST_INIT(&((psem)->queue));
	return 0;
}

void 
ss_sem_destroy(Sem *psem)
{
	LIST_INSERT_HEAD(&sem_free_list, psem, sem_link);
}

int
ss_sem_wait(sem_t semid, u_int envid)
{
	Sem *psem = sems + semid;
    if (psem->value > 0) {
        psem->value--;
        return 0;
    } else {
		LIST_INSERT_TAIL(&(psem->queue), &envs[ENVX(envid)], env_waiting_link);
        return -1;
    }	
}

int
ss_sem_trywait(sem_t semid, u_int envid)
{
	Sem *psem = sems + semid;
	if (psem->value > 0) {
		psem->value--;
		return 0;
	} else {
		return -1;
	}
}

void
ss_sem_post(sem_t semid)
{
	Sem *psem = sems + semid;
	struct Env *e;
	psem->value++;
	if (psem->value <= 0) {
		e = LIST_FIRST(&(psem->queue));
		LIST_REMOVE(e, env_waiting_link);
		e->env_status = ENV_RUNNABLE;
	}
}

int
ss_sem_getvalue(sem_t semid)
{
	return sems[semid].value;
}

