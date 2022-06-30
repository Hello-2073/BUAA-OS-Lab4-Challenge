#include "lib.h"
#include "pthread.h"
#include "sem.h"
#include <env.h>

sem_t mutex;
char buf[100] = {'\0'};

void writer(char name[]) 
{
	int num = 0, i, j;
	while(1) {
		sem_wait(&mutex);
		writef("[%s] enters critical area.\n", name);
		i = num++;
		j = 0;
		do {
			buf[j++] = '0' + i % 10;
			i /= 10;
		} while(i);
		buf[j] = '\0';
		writef("[%s] writes [%s] to buf.\n\n", name, buf);
		sem_post(&mutex);
	}
}

void reader(char name[])
{
	while(1) {
		sem_wait(&mutex);
		writef("[%s] enters critical area.\n", name);
		writef("[%s] reads [%s] from buf.\n\n", name, buf);
		sem_post(&mutex);
	}
}


void umain()
{
	pthread_t thread1, thread2, thread3;
	sem_init(&mutex, 0, 1);
	writef("init a semphore, id %d\n", mutex);

	pthread_create(&thread1, 0, writer, "writer1");
	pthread_create(&thread2, 0, reader, "reader1");
	pthread_create(&thread3, 0, reader, "reader2");
}
