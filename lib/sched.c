#include <env.h>
#include <pmap.h>
#include <printf.h>

/* Overview:
 *  Implement simple round-robin scheduling.
 *
 *
 * Hints:
 *  1. The variable which is for counting should be defined as 'static'.
 *  2. Use variable 'env_sched_list', which is a pointer array.
 *  3. CANNOT use `return` statement!
 */
/*** exercise 3.15 ***/
void sched_yield(void) {
	static int count = 0; // remaining time slices of current env
    static int point = 0; // current env_sched_list index
    static struct Env *e = NULL;
    /*  hint:
     *  1. if (count==0), insert `e` into `env_sched_list[1-point]`
     *     using LIST_REMOVE and LIST_INSERT_TAIL.
     *  2. if (env_sched_list[point] is empty), point = 1 - point;
     *     then search through `env_sched_list[point]` for a runnable env `e`, 
     *     and set count = e->env_pri
     *  3. count--
     *  4. env_run()
     *
     *  functions or macros below may be used (not all):
     *  LIST_INSERT_TAIL, LIST_REMOVE, LIST_FIRST, LIST_EMPTY
     */
	// printf("sched_yield() is called.\n");
	// if (e) {
	//	 printf("sched_yield(): env_id %d has %d to excute, pc 0x%x\n", e->env_id, count, e->env_tf.pc);
	// }
	// e = curenv;
	if (count <= 0 || e == NULL || e->env_status != ENV_RUNNABLE) {
		// if (e != NULL) {
		//	printf("sched_yield(): env_id %d, times %d, status %d", e->env_id, count, e->env_status);
		// } else {
		// 	printf("sched_yield(): first time\n");
		// }
		if (e != NULL) {
			LIST_REMOVE(e, env_sched_link);
			if (e->env_status != ENV_FREE) {
				// printf("sched_yield(): env %b waiting\n", e->env_stack_map);
				LIST_INSERT_TAIL(&env_sched_list[1 - point], e, env_sched_link);
			}
		}
		while (1) {
			// printf("sched_yield(): to find a new env\n");
			while (LIST_EMPTY(&env_sched_list[point])) {
				//printf("sched_yield(): sched_list %d is empty\n", point);
				point = 1 - point;
			}
			e = LIST_FIRST(&env_sched_list[point]);
			// printf("sched_yield(): id %d, status is %d\n", e->env_id, e->env_status);
			if (e->env_status == ENV_RUNNABLE) {	
				count = e->env_pri;
				break;
			} else if (e->env_status == ENV_FREE) {
				LIST_REMOVE(e, env_sched_link);
			} else {
				LIST_REMOVE(e, env_sched_link);
				LIST_INSERT_TAIL(&env_sched_list[1 - point], e, env_sched_link);
			}
		}
	}
	// if (e == NULL) {
	// 	panic("sched_yield(): NULL!\n");
	// }
	count--;
	e->env_runs++;
	//	 printf("sched_yield(): %d is sched\n", e->env_id);
	// printf("sched_yield() finished on success\n");
	env_run(e);
}
