#ifndef threadpool_h
#define threadpool_h

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

typedef struct {
	void (*function)(int);
	int arg;
} task;

typedef struct queue_node{
	task *t;
	struct queue_node *next;
} node;


typedef struct{
	node *head;
	node *tail;
	int size;
} queue;

typedef struct{
	pthread_t *tid;
	queue *activeClients;
	pthread_mutex_t lock;
	pthread_cond_t empty;
	int size;
	int finish;
} threadpool;



int enqueue(threadpool *pool, task *t);
void threadpool_init(threadpool *pool, int n);
void threadpool_fin(threadpool *pool);

#endif
