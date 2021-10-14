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

}data;

sem_t semaphore;
pthread_mutex_t mutex;

//decliration of functions.
int check_target_size(const char *target_file);
void dir_check(const char *target_dir, data *d);
mode_t check_target_mode(const char *target);
void *check_target(void *ptr);
int thread_maker(data *d);

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

    char *file;

    //add targets to queue.
    for (int i = optind; i < argc ; i++)
    {   
         //create empty queue
        d->queue = queue_empty(NULL);

        if (sem_init(&semaphore, 0, d->number_of_threads) == -1)
        {
            perror("Semaphore init failed!");
            return EXIT_FAILURE;
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
        sem_destroy(&semaphore);
    }
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
 * @brief Function that checks the size of target when its a directory.
 * 
 * @param target_dir directory to check
 * @return size of directory as an int
 */
void dir_check(const char *target_dir, data *d)
{
    DIR *dir;
    struct dirent* direntp;

    //checks if directory is vaild or not.
    if ((dir = opendir(target_dir)) == NULL)
    {
        fprintf(stderr, "du: cannot read directory '%s': Permission denied\n", target_dir);
        pthread_mutex_lock(&mutex);
        d->exit_code = EXIT_FAILURE;
        pthread_mutex_unlock(&mutex);
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

void *check_target(void *ptr)
{   
    data *d = ptr;
    int *size = malloc(sizeof(*size));
    *size = 0;
    mode_t mode_of_target;
    char *target;

    //loop to check if the target is a file or a directory.
    while (!queue_is_done(d->queue, &semaphore, d->number_of_threads))
    {   
        //lock 
        sem_wait(&semaphore);

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

        sem_post(&semaphore);
    }
    return size;
}

int thread_maker(data *d)
{
    pthread_t thread[d->number_of_threads];
    
    int size = 0;
    int *size_catch;

    for (int i = 0; i < d->number_of_threads; i++) 
    {   
        if (pthread_create(&thread[i], NULL, *check_target, d) != 0)
        {
            perror("Thread create failed!");
            exit(EXIT_FAILURE);
        }
    }

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