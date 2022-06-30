#include "lib.h"
#include "pthread.h"
#include "sem.h"
#include <env.h>

sem_t mutex_rn, mutex_rw, mutex_io;
int rn = 0;
char buf[100] = {'\0'};

void writer(char name[]) 
{
	int num = 0, i, j;
	while(1) {
		sem_wait(&mutex_rw);
		i = num++;
		j = 0;
		do {
			buf[j++] = '0' + i % 10;
			i /= 10;
		} while(i);
		buf[j] = '\0';
		sem_wait(&mutex_io);
		writef("[%s] writes [%s] to buf.\n", name, buf);
		sem_post(&mutex_io);
		sem_post(&mutex_rw);
	}
}

void reader(char name[])
{
	while(1) {
		sem_wait(&mutex_rn);
		if (rn == 0)
			sem_wait(&mutex_rw);
		rn++;
		sem_post(&mutex_rn);
		sem_wait(&mutex_io);
		writef("[%s] reads [%s] from buf.\n", name, buf);
		sem_post(&mutex_io);
		sem_wait(&mutex_rn);
		rn--;
		if (rn == 0)
			sem_post(&mutex_rw);
		sem_post(&mutex_rn);
	}
}


void umain()
{
	pthread_t thread1, thread2, thread3;
	sem_init(&mutex_rw, 0, 1);
	sem_init(&mutex_rn, 0, 1);
	sem_init(&mutex_io, 0, 1);

	pthread_create(&thread1, 0, writer, "writer1");
	pthread_create(&thread2, 0, reader, "reader2");
	pthread_create(&thread3, 0, reader, "reader3");
	pthread_create(&thread3, 0, reader, "reader4");
	pthread_create(&thread3, 0, reader, "reader5");
	pthread_create(&thread3, 0, reader, "reader6");
	pthread_create(&thread3, 0, reader, "reader7");

}
