#include <linux/limits.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <pthread.h>

#include "list.h"
#include "queue.h"

//data structure decliration
typedef struct data
{   
    int number_of_threads;
    queue *queue;
    int exit_code;
    pthread_mutex_t mutex;
    sem_t semaphore;

}data;

//decliration of functions.
int check_target_size(const char *target_file);
void dir_check(const char *target_dir, data *d);
mode_t check_target_mode(const char *target);
void *check_target(void *ptr);
int thread_maker(data *d);
void mutex_init(data *d);
void add_target(data *d, int argc, char *argv[]);

/**
 * @brief Main function that runs the program.
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[])
{
    //if no arguments(files or directories) are sent.
    if (argc < 2)
    {
        fprintf(stderr, "No files or directories!\n");
        return EXIT_FAILURE;
    }

    char flag; 
    data *d = malloc(sizeof(*d));
    mutex_init(d);

    //checks if the previous malloc was successfull or not
    if (d == NULL) 
    {
        perror("Allocation failed!");
        return EXIT_FAILURE;
    }

    d->number_of_threads = 1;
    d->exit_code = EXIT_SUCCESS;

    // loop to catch j flag.
    while ((flag = getopt(argc, argv, "j:")) != -1)
    {   
        // j flag caught
        if (flag == 'j')
        {   
            char* rest;

            errno = 0; 
            //check how many threads to make
            d->number_of_threads = strtol(optarg, &rest, 10);

            //if strtol fails.
            if (errno != 0)
            {
                perror("Strltol failed!");
                return EXIT_FAILURE;
            }
            
            //if no, or invalid amout of threads is read
            if (rest[0] != '\0' || d->number_of_threads <= 0)
            {
                fprintf(stderr, "Invalid amount of threads!\n");
                return EXIT_FAILURE;
            }
        }
        // if a invalid flag is read, print error and close exit program.
        else
        {
            fprintf(stderr, "No valid flag!\n");
            return EXIT_FAILURE;
        }
    }

    //add targets to queue.
    add_target(d, argc ,argv);

    //return exit_code and free data structure.
    int exit_code = d->exit_code;
    free(d);   
    return exit_code;
}

/**
 * @brief Function that checks the target file. 
 * 
 * @param target_file file to check size of.
 * @return the size as a int.
 */
int check_target_size(const char *target_file)
{   
    struct stat file_information;
    
    //check if target stats was returned correctly.
    if (lstat(target_file, &file_information) < 0)
    {
        perror(target_file);
        exit(EXIT_FAILURE);
    }
    
    return file_information.st_blocks;
}

/**
 * @brief function that checks the target mode, directory or file.
 * 
 * @param target target to check
 * @return the target as a mode_t
 */
mode_t check_target_mode(const char *target)
{
    struct stat file_information;
    
    //check if target stats was returned correctly.
    if (lstat(target, &file_information) < 0)
    {
        perror(target);
        exit(EXIT_FAILURE);
    }

    return file_information.st_mode;
}

/**
 * @brief Function that reads from dir
 * 
 * @param target_dir dir to open
 * @param d data structure
 */
void dir_check(const char *target_dir, data *d)
{
    DIR *dir;
    struct dirent* direntp;

    //checks if directory is vaild or not.
    if ((dir = opendir(target_dir)) == NULL)
    {
        fprintf(stderr, "du: cannot read directory '%s': Permission denied\n", target_dir);
        //only adding mutex lock and unlock here to remove errors when using helgrind. 
        pthread_mutex_lock(&d->mutex);
        d->exit_code = EXIT_FAILURE;
        pthread_mutex_unlock(&d->mutex);
    }
    else
    {   
        //read all files in directory.
        while ((direntp = readdir(dir)) != NULL)
        {   
            //removes the "." and ".." from the directory.
            if ((strcmp(direntp->d_name, ".") == 0) || (strcmp(direntp->d_name, "..") == 0))
            {
                continue;
            }

            //allocate memory for file_name and add null terminator to the first bit.
            char *file_name = malloc(sizeof(*file_name)*PATH_MAX);

            if (file_name == NULL)
            {
                perror("Failed to allocate!");
                exit(EXIT_FAILURE);
            }

            file_name[0] = '\0';

            //copys directory and file name and adds / to file_name.
            strcat(file_name, target_dir);
            if (file_name[strlen(file_name) -1] != '/')
            {
                strcat(file_name, "/");
            }
            strcat(file_name, direntp->d_name);

            //add target to queue.
            queue_enqueue(d->queue, file_name);
        }

        closedir(dir);
    }
}

/**
 * @brief Function that checks the target size.
 * 
 * @param ptr target to check
 * @return void* 
 */
void *check_target(void *ptr)
{   
    data *d = ptr;
    int *size = malloc(sizeof(*size));
    *size = 0;
    mode_t mode_of_target;
    char *target;

    //loop to check if the target is a file or a directory.
    while (!queue_is_done(d->queue, &d->semaphore, d->number_of_threads))
    {   
        //lock 
        sem_wait(&d->semaphore);

        //get the target from queue.
        target = queue_dequeue(d->queue);
        if(target != NULL) 
        {
            //check the target mode
            mode_of_target = check_target_mode(target);

            //if target is a file or a symbolic link.
            if (S_ISREG(mode_of_target) || S_ISLNK(mode_of_target))
            {
                *size += check_target_size(target);        
            }

            //if target is a directory .
            if (S_ISDIR(mode_of_target))
            {
                dir_check(target, d);
                *size += check_target_size(target);  
            }

            free(target);
        }

        sem_post(&d->semaphore);
    }
    return size;
}

/**
 * @brief Function that creates the threads
 * 
 * @param d data structure
 * @return int 
 */
int thread_maker(data *d)
{
    pthread_t thread[d->number_of_threads];
    
    int size = 0;
    int *size_catch;

    //create threads
    for (int i = 0; i < d->number_of_threads; i++) 
    {   
        if (pthread_create(&thread[i], NULL, *check_target, d) != 0)
        {
            perror("Thread create failed!");
            exit(EXIT_FAILURE);
        }
    }

    //join threads
    for (int i = 0; i < d->number_of_threads; i++) 
    {
        if (pthread_join(thread[i], (void **)&size_catch) != 0)
        {
            perror("Thread join failed!");
            exit(EXIT_FAILURE);
        }

        size += *size_catch;
        free(size_catch);
    }

    return size;
}

/**
 * @brief Function to initilize the mutex 
 * 
 * @param d data structure
 */
void mutex_init(data *d)
{
    //init of mutex 
	if (pthread_mutex_init(&d->mutex, NULL) != 0) 
	{
		perror("Mutex failed!");
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief function that adds target to queue
 * 
 * @param d data structure
 * @param argc amount in arg
 * @param argv whats in arg
 */
void add_target(data *d, int argc, char *argv[])
{
    char *file;

    //add targets to queue.
    for (int i = optind; i < argc ; i++)
    {   
         //create empty queue
        d->queue = queue_empty(NULL);

        if (sem_init(&d->semaphore, 0, d->number_of_threads) == -1)
        {
            perror("Semaphore init failed!");
            exit(EXIT_FAILURE);
        }

        //copy argv to file so each target can  be freed after being dequeued.
        file = strdup(argv[i]);
        queue_enqueue(d->queue, file);

        if (d->number_of_threads == 1)
        {
            //check if target is a file or directory without threads.
            int *size = check_target(d);
            fprintf(stdout, "%d      ", *size);
            fprintf(stdout, "%s\n", argv[i]);
            free(size);
        }
        else
        {   
            //check if target is a file or directory with threads.
            int size = thread_maker(d);
            fprintf(stdout, "%d      ", size);
            fprintf(stdout, "%s\n", argv[i]);
        }

        queue_kill(d->queue);
        sem_destroy(&d->semaphore);
        pthread_mutex_destroy(&d->mutex);
    }
}