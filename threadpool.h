#ifndef threadpool_h
#define threadpool_h

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>


/**
 * Struct that keeps what task
 * have to be done
 *
 *
 */
typedef struct {
	void (*function)(int);
	int arg;
} task;
/**
 * Node of tasks waiting to be done
 *
 *
 */
typedef struct queue_node{
	task *t;
	struct queue_node *next;
} node;

/**
 * Queue that keeps tasks
 *
 *
 */
typedef struct{
	node *head;
	node *tail;
	int size;
} queue;
/**
 * Thread pool that is the share memory
 * keeps the tasks,threas and have semaphores
 * for locking and unlocking the share
 * memory
 */
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
