#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "queue.h"
#include "list.h"

/*
 * Implementation of a generic queue for the "Datastructures and
 * algorithms" courses at the Department of Computing Science, Umea
 * University.
 *
 * Authors: Niclas Borlin (niclas@cs.umu.se)
 *	    Adam Dahlgren Lindstrom (dali@cs.umu.se)
 *
 * Based on earlier code by: Johan Eliasson (johane@cs.umu.se).
 *
 * Version information:
 *   2018-01-28: v1.0, first public version.
 */

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
 * queue_empty() - Create an empty queue.
 * @free_func: A pointer to a function (or NULL) to be called to
 *	       de-allocate memory on remove/kill.
 *
 * Returns: A pointer to the new queue.
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
 * queue_is_empty() - Check if a queue is empty.
 * @q: Queue to check.
 *
 * Returns: True if queue is empty, otherwise false.
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
 * queue_enqueue() - Put a value at the end of the queue.
 * @q: Queue to manipulate.
 * @v: Value (pointer) to be put in the queue.
 *
 * Returns: The modified queue.
 */
queue *queue_enqueue(queue *q, void *v)
{	
	//lock mutex
	pthread_mutex_lock(&mutex);

	//insert to queue
	list_insert(q->elements, v, list_end(q->elements));

	//unlock mutex
	pthread_mutex_unlock(&mutex);

	//send signal 
	pthread_cond_signal(&condition);
	
	return q;
}

/**
 * queue_dequeue() - Remove the element at the front of a queue.
 * @q: Queue to manipulate.
 *
 * NOTE: Undefined for an empty queue.
 *
 * Returns: The modified queue.
 */
char *queue_dequeue(queue *q)
{	
	
	//lock mutex
	pthread_mutex_lock(&mutex);
	char *file = NULL;
	if(!list_is_empty(q->elements)) {
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
