#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "queue.h"
#include "list.h"

// ===========INTERNAL DATA TYPES============

/*
 * The queue is implemented using the list abstract datatype. Almost
 * everything is done by the list.
 */

struct queue {
	list *elements;
};

pthread_mutex_t mutex;
pthread_cond_t condition;


// ===========INTERNAL FUNCTION IMPLEMENTATIONS============

/**
 * @brief Function that checks if the queue is empty or not
 * 
 * @param free_func NULL for defeault
 * @return queue* that was created
 */
queue *queue_empty(free_function free_func)
{	
	//init of mutex 
	if (pthread_mutex_init(&mutex, NULL) != 0) 
	{
		perror("Mutex failed!");
		exit(EXIT_FAILURE);
	}

	//init global condtion
	if (pthread_cond_init(&condition, NULL) != 0) 
	{
		perror("Condition failed!");
		exit(EXIT_FAILURE);
	}

	// Allocate the queue head.
	queue *q=calloc(1, sizeof(*q));

	if (q == NULL) 
	{
		perror("Failed to allocate");
		exit(EXIT_FAILURE);
	}

	// Create an empty list.
	q->elements=list_empty(free_func);

	return q;
}

/**
 * @brief Function that checks if the queue is done or not. 
 * 
 * @param q queue to check 
 * @param semaphore semaphore to check
 * @param number_of_threads to check
 * @return true 
 * @return false 
 */
bool queue_is_done(const queue *q, sem_t *semaphore, int number_of_threads)
{	

	//lock mutex
	pthread_mutex_lock(&mutex);

	//get the value of semaphore.
	int semaphore_value;
	sem_getvalue(semaphore, &semaphore_value);

	//loop to check that the semaphore is set and the list is empty, then set condtion wait.
	while (semaphore_value != number_of_threads && list_is_empty(q->elements)) 
	{
		pthread_cond_wait(&condition, &mutex);
		sem_getvalue(semaphore, &semaphore_value);
	}

	//if all threads are waiting and the list is empty
	if (list_is_empty(q->elements) && semaphore_value == number_of_threads) 
	{	
		//set condition to broadcast.
		pthread_cond_broadcast(&condition);

		//unlock mutex
		pthread_mutex_unlock(&mutex);
		
		return true;
	}
	
	//unlock mutex
	pthread_mutex_unlock(&mutex);

	return false;
}


/**
 * @brief Function that adds *v to queue
 * 
 * @param q queue to manipulate
 * @param v variable to add
 */
void queue_enqueue(queue *q, void *v)
{	
	//lock mutex
	pthread_mutex_lock(&mutex);

	//insert to queue
	list_insert(q->elements, v, list_end(q->elements));

	//send signal 
	pthread_cond_signal(&condition);

	//unlock mutex
	pthread_mutex_unlock(&mutex);

}

/**
 * @brief Function that removes the first element from the queue.
 * 
 * @param q queue to manipuliate
 * @return char* 
 */
char *queue_dequeue(queue *q)
{	
	
	//lock mutex
	pthread_mutex_lock(&mutex);

	char *file = NULL;
	if(!list_is_empty(q->elements)) 
	{
		//get the element from the queue.
		file =list_inspect(q->elements, list_first(q->elements));

		//remove from queue.
		list_remove(q->elements, list_first(q->elements));
	}

	//unlock mutex
	pthread_mutex_unlock(&mutex);

	return file;
}

/**
 * queue_kill() - Destroy a given queue.
 * @q: Queue to destroy.
 *
 * Return all dynamic memory used by the queue and its elements. If a
 * free_func was registered at queue creation, also calls it for each
 * element to free any user-allocated memory occupied by the element values.
 *
 * Returns: Nothing.
 */
void queue_kill(queue *q)
{	
	//destroy the queue
	list_kill(q->elements);
	
	//destroy the condtion variable
	pthread_cond_destroy(&condition);

	//destroy the mutex 
	pthread_mutex_destroy(&mutex);

	//free allocated memory
	free(q);
}
