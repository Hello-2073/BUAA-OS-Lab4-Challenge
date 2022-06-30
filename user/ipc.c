// User-level IPC library routines

#include "lib.h"
#include <mmu.h>
#include <env.h>


// Send val to whom.  This function keeps trying until
// it succeeds.  It should panic() on any error other than
// -E_IPC_NOT_RECV.
//
// Hint: use syscall_yield() to be CPU-friendly.
void
ipc_send(u_int whom, u_int val, u_int srcva, u_int perm)
{
	int r;
	struct Env *env = &envs[ENVX(syscall_getenvid())];

	//writef("%d send %d, %d\n", env->env_id, whom, val);
	while ((r = syscall_ipc_can_send(whom, val, srcva, perm)) == -E_IPC_NOT_RECV) {
		syscall_yield();
	}

	if (r == 0) {
		return;
	}

	user_panic("error in ipc_send: %d", r);
}

// Receive a value.  Return the value and store the caller's envid
// in *whom.
//
// Hint: use env to discover the value and who sent it.
u_int
ipc_recv(u_int *whom, u_int dstva, u_int *perm)
{
	syscall_ipc_recv(dstva);
	// writef("%d-%d\n", env->env_id, env->env_ipc_value);
	struct Env *env = &envs[ENVX(syscall_getenvid())];
	
	if (whom) {
		*whom = env->env_ipc_from;
	}

	if (perm) {
		*perm = env->env_ipc_perm;
	}

	//writef("%d recv %d from %d\n", env->env_id, env->env_ipc_value, env->env_ipc_from);
	return env->env_ipc_value;
}

