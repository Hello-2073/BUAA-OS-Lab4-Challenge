#include "lib.h"
#include "pthread.h"
#include <env.h>

int times = 0;
int turn = 0;

void dog() {
	while(1) {
		if (!turn) {
			writef("Wang : %d\n", times++);
			if (times > 3) {
				turn = 1;
				break;
			} else {
				turn = 1;
			}
		} else {
			// writef("Dog yield, because turn == %d, times == %d\n", turn, times);
			syscall_yield();
		}
	}
}

void cat() {
	while(1) {
        if (turn) {
            writef("Miao : %d\n", times++);
            if (times > 3) {
				turn = 0;
				break;
			} else {
            	turn = 0;
			}
        } else {
            // writef("Cat yield, because turn == %d, times == %d\n", turn, times);
            syscall_yield();
        }
    }
}

void counter(void *arg) {
	int num = *(int *)arg, i, j;
	writef("Thread Counter() begins to count: ");
	for (i = 0; i < num; i++) {
		writef("%d, ", i);
		syscall_yield();
	}
	writef("... \n");
	writef("Thread Counter() trys to return %d\n", i);
	pthread_exit(&i);
}

void speaker() {
	while(1) {
		writef("Speaker() will never dead!!!\n");
		syscall_yield();
	}
}

void umain() {
	writef("0x%x\n", &times);
	writef("\n---------------- thread test begin ----------------\n");
	pthread_t thread1, thread2, thread3, thread4;
	int num = 10,  *retval;
	pthread_create(&thread1, 0, dog, NULL);
	pthread_create(&thread2, 0, cat, NULL);
	if (pthread_join(thread1, NULL) == 0) {
		writef("Umain recovers after thread Dog() exited.\n");
	} else {
		writef("Thread Dog() exited before umain() try to join.\n");
	}
	if (pthread_join(thread2, NULL)) {
		writef("Umain recovers after thread Cat() exited.\n");
	} else {
		writef("Thread Cat() exited before umain() try to join.\n"); 
	}
	pthread_create(&thread3, 0, counter, &num);
	if (pthread_join(thread3, &retval) == 0) {
		writef("Umain gets %d from thread counter, and %d is expected.\n", *retval, num);
	}
	pthread_create(&thread4, 0, speaker, NULL);
	syscall_yield();
	if (pthread_cancel(thread4) == 0) {
		writef("Umain() shut down thread Speaker()!!!");
	}
	writef("\n---------------- thread test end ------------------\n");
	writef("umain() reach end\n");
}
