#include "lib.h"
#include "pthread.h"
#include "sem.h"

sem_t mutex;
char buf[100];

void writer(char name[]) 
{
	int num = 0, i, j;
	while(1) {
		sem_wait(&mutex);
		i = num;
		j = 0;
		do {
			buf[j++] = '0' + i % 10;
			i /= 10;
		} while(i);
		buf[j] = '\0';
		writef("Writer %s: %s\n", name, buf);
		sem_post(&mutex);
	}
}

void reader(char name[])
{
	while(1) {
		sem_wait(&mutex);
		writef("Reader %s: %s\n", name, buf);
		sem_post(&mutex);
	}
}


void umain()
{
	sem_init(&mutex, 0, 1);
	pthread_create(NULL, 0, writer, "writer1");
	pthread_create(NULL, 0, reader, "reader1");
	pthread_create(NULL, 0, reader, "reader2");
}
