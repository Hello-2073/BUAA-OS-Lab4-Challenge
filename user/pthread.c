#include"lib.h"
#include"pthread.h"


int
pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
				void *(*start_routine)(void *), void *arg) {;
	u_int newenvid = syscall_env_alloc(0);
	if (newenvid == 0) {
		//writef("A new thread jump to 0x%x \n", start_routine);
		(*start_routine)(arg);
		exit(0);
		user_panic("error !\n\n");
	}
	syscall_set_env_status(newenvid, ENV_RUNNABLE);
	*thread = newenvid;
	// writef("create a new thread %d\n", *thread);	
	return 0;
}

void
pthread_exit(void *retval) {
	exit(retval);
}

int 
pthread_join(pthread_t thread, void **retval) {
	int r;
	r = syscall_env_join(thread, retval);	
	if (r) {
		// writef("failed, thread %d may be over\n\n", thread);
		return r;
	}
	syscall_yield();
// 	writef("sucessed\n\n");
	return 0;
}

int
pthread_cancel(pthread_t thread) {
	return syscall_env_destroy(thread, PTHREAD_CANCELED);
}
