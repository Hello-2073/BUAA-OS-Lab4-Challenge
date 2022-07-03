#include "ss.h"
#include "lib.h"
#include <mmu.h>

#define REQVA 0x0ffff000

void 
serve_sem_init(u_int envid, struct Semreq_init *rq) 
{
	int r = ss_sem_init(rq->value, rq->pshared, &rq->semid);
	ipc_send(envid, r, 0, 0);
}

void
serve_sem_destroy(u_int envid, struct Semreq_destroy *rq)
{
	int r = ss_sem_destroy(rq->semid);
	ipc_send(envid, r, 0, 0);
}

void
serve_sem_wait(u_int envid, struct Semreq_wait *rq)
{
	int r = ss_sem_wait(rq->semid, envid, &rq->value);
	if (r < 0 || rq->value >= 0) ipc_send(envid, r, 0, 0);
}

void
serve_sem_trywait(u_int envid, struct Semreq_trywait *rq)
{
	int r = ss_sem_trywait(rq->semid);
	ipc_send(envid, r, 0, 0);
}

void
serve_sem_post(u_int envid, struct Semreq_post *rq)
{
	int r = ss_sem_post(rq->semid);
	ipc_send(envid, r, 0, 0);
}

void
serve_sem_getvalue(u_int envid, struct Semreq_getvalue *rq)
{
	int r = ss_sem_getvalue(rq->semid, &rq->value);
	ipc_send(envid, r, 0, 0);
}

void
serve()
{
	u_int req, whom, perm;

	for (;;) {
		perm = 0;
		req = ipc_recv(&whom, REQVA, &perm);
		
		// writef("receiving semreq %d from %d\n", req, whom);
		if (!(perm & PTE_V))
		{
			writef("Invalid request from %08x: no argument page\n", whom);
			continue;
		}

		switch(req)
		{
			case SEMREQ_INIT:
				serve_sem_init(whom, (struct Semreq_init *)REQVA);
				break;
			case SEMREQ_DESTROY:
				serve_sem_destroy(whom, (struct Semreq_destroy *)REQVA);
				break;
			case SEMREQ_WAIT:
				serve_sem_wait(whom, (struct Semreq_wait *)REQVA);
				break;
			case SEMREQ_TRYWAIT:
				serve_sem_trywait(whom, (struct Semreq_trywait *)REQVA);
				break;
			case SEMREQ_POST:
				serve_sem_post(whom, (struct Semreq_post *)REQVA);
				break;
			case SEMREQ_GETVALUE:
				serve_sem_getvalue(whom, (struct Semreq_getvalue *)REQVA);
				break;
			default:
				break;
		}
		syscall_mem_unmap(0, REQVA);
	}	
}

void umain()
{
	writef("sem_serve begin\n");
	ss_init();
	writef("sem_serve init success\n");
	serve();
}
