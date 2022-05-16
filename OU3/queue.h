#ifndef __QUEUE_H
#define __QUEUE_H

#include <semaphore.h>
#include <stdbool.h>

#include "util.h"

// ==========PUBLIC DATA TYPES============

// Queue type.
typedef struct queue queue;

// ==========DATA STRUCTURE INTERFACE==========

/**
 * @brief Function that checks if the queue is empty or not
 * 
 * @param free_func NULL for defeault
 * @return queue* that was created
 */
queue *queue_empty(free_function free_func);

/**
 * @brief Function that checks if the queue is done or not. 
 * 
 * @param q queue to check 
 * @param semaphore semaphore to check
 * @param number_of_threads to check
 * @return true 
 * @return false 
 */
bool queue_is_done(const queue *q, sem_t *semaphore, int number_of_threads);


/**
 * @brief Function that adds *v to queue
 * 
 * @param q queue to manipulate
 * @param v variable to add
 */
void queue_enqueue(queue *q, void *v);

/**
 * @brief Function that removes the first element from the queue.
 * 
 * @param q queue to manipuliate
 * @return char* 
 */
char *queue_dequeue(queue *q);

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
void queue_kill(queue *q);

#endif
