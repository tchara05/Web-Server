#include "threadpool.h"

int enqueue(threadpool *pool, task *t){
	int unlocked,error=0;
	queue *q=pool->activeClients;
	
	unlocked=pthread_mutex_trylock(&(pool->lock));
	node *newnode=malloc(sizeof(node));
	if(newnode==NULL) perror("Node alloc:");
	newnode->t=malloc(sizeof(task));
	if(newnode->t==NULL) perror("task:");
	newnode->t->function=t->function;
	newnode->t->arg=t->arg;
	newnode->next=NULL;
	if(unlocked) pthread_mutex_lock(&(pool->lock));
	if (pool->size==q->size){
		free(newnode);
		error=-1;
		goto done;
	}
	if(q->size==0){
		q->head=newnode;
		q->tail=newnode;
		q->size++;
		pthread_cond_signal(&(pool->empty));
	} else {
		q->tail->next=newnode;
		q->tail=newnode;
		q->size++;
	}
	done:
	pthread_mutex_unlock(&(pool->lock));
	return error;
}

void dequeue(threadpool *pool, task *t){
	queue *q=pool->activeClients;
	
	node *temp;
	if(q->size==0) return;
	temp=q->head;
	t->function=temp->t->function;
	t->arg=temp->t->arg;
	q->head=temp->next;
	q->size--;
	free(temp->t);
	free(temp);
}

void *worker(void *arg){
	threadpool *pool=(threadpool *) arg;
	task t;
	queue *q=pool->activeClients;
	
	while(1){
		pthread_mutex_lock(&(pool->lock));
		while (q->size==0 && !(pool->finish)) 
			pthread_cond_wait(&(pool->empty), &(pool->lock));
		if(pool->finish) break;
		dequeue(pool, &t);
		pthread_mutex_unlock(&(pool->lock));
		//int *f=(int*)t.arg;
		(*t.function)((int)t.arg);
	}
	pthread_mutex_unlock(&(pool->lock));
	return NULL;
}

void threadpool_fin(threadpool *pool){
	int i;
	node *temp;
	
	pool->finish=1;
	pthread_cond_broadcast(&(pool->empty));
	//pthread_mutex_unlock(&(pool->lock));
	for(i=0;i<pool->size;i++) pthread_join(pool->tid[i], NULL);
	
	while((temp=pool->activeClients->head)){
		pool->activeClients->head=temp->next;
		free(temp->t);
		free(temp);
	}
	free(pool->activeClients);
	free(pool->tid);
}

void threadpool_init(threadpool *pool, int n){
	int i;
	
	pool->activeClients=malloc(sizeof(queue));
	if(pool->activeClients==NULL) perror("activeClients creation:");
	pool->activeClients->head=NULL;
	pool->activeClients->tail=NULL;
	pool->activeClients->size=0;
	
	if(pthread_mutex_init(&(pool->lock), NULL) != 0) perror("mutex:");
	if(pthread_cond_init(&(pool->empty), NULL) != 0) perror("cond:");
	
	pool->size=n;
	pool->finish=0;
	
	pool->tid=malloc(n * sizeof(pthread_t));
	if(pool->tid==NULL) perror("Threads pointer:");
	for(i=0;i<n;i++){
		if(pthread_create(&(pool->tid[i]), NULL, &worker, (void *) pool)!=0) 
			perror("In threads creation");
	}
}

void f(void *a){
	printf("%d\n", *((int*)a));
}

int mains(){
task z;
int a=9;
//z.function=&f;
z.arg=a;
threadpool p;
threadpool_init(&p, 2);
enqueue(&p, &z);
enqueue(&p, &z);
enqueue(&p, &z);
enqueue(&p, &z);
enqueue(&p, &z);
//sleep(2);
threadpool_fin(&p);
return 0;
}
