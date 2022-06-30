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

int
ss_sem_init(u_int value)
{
	Sem *psem;
	if (LIST_EMPTY(&sem_free_list)) {	
		user_panic("no free sem\n\n");
		return 0;
	}
	psem = LIST_FIRST(&sem_free_list);
	LIST_REMOVE(psem, sem_link);
	psem->value = value;
	LIST_INIT(&((psem)->queue));
	writef("a new semm id %d, value %d\n", (psem - sems) | (1 << 7), psem->value);
	return (psem - sems) | (1 << 7);
}

void 
ss_sem_destroy(sem_t semid)
{
	Sem *psem = sems + (semid & 0x7f);
	LIST_INSERT_HEAD(&sem_free_list, psem, sem_link);
}

int
ss_sem_wait(sem_t semid, u_int envid)
{
	Sem *psem = sems + (semid & 0x7f);
	psem->value--;
    if (psem->value >= 0) {
		// writef("%d not in, %d: %d\n\n", envid, semid, psem->value);
        return 0;
    } else {
		// writef("%D in, %d: %d\n\n", envid, semid, psem->value);
		LIST_INSERT_TAIL(&(psem->queue), &envs[ENVX(envid)], env_link);
        return -1;
    }	
}

int
ss_sem_trywait(sem_t semid, u_int envid)
{
	Sem *psem = sems + (semid & 0x7f);
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
	//writef("post\n\n\n");
	Sem *psem = sems + (semid & 0x7f);
	struct Env *e;
	psem->value++;
	if (psem->value <= 0) {
		e = LIST_FIRST(&(psem->queue));
		LIST_REMOVE(e, env_link);
		// writef("%d out, %d : %d\n\n", e->env_id, semid, psem->value);
		e->env_status = ENV_RUNNABLE;
	}
}

int
ss_sem_getvalue(sem_t semid)
{
	return sems[semid & 0x7f].value;
}

