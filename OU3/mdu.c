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

//decliration of functions.
int check_target_size(const char *target_file);
int dir_check(const char *target_dir);
mode_t check_target_mode(const char *target);
void* check_target(char *argv[]);
void thread_func(int num_of_threads);

void print(const void *data);

int main(int argc, char *argv[])
{
    char flag; 
    bool j_flag = false;
    int number_of_threads = 0;

    // loop to catch j flag.
    while ((flag = getopt(argc, argv, "j")) != -1)
    {   
        // j flag caught
        if (flag == 'j')
        {   
            j_flag = true;
            //check how many threads to make
            if (atoi(argv[2]) > 0)
            {   
                number_of_threads = atoi(argv[2]);
            }
            //if no amount is specified 
            else
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

    //if no arguments(files or directories) are sent.
    if (argc < 2)
    {
        fprintf(stderr, "No files or directories!\n");
        return EXIT_FAILURE;
    }
    else
    {
        if (!j_flag)
        {
            //check if target is a file or directory without threads.
            check_target(argv);
        }
        else
        {   
            //check if target is a file or directory with threads.
            thread_func(number_of_threads);
        }
    }
            
    return EXIT_SUCCESS;
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

    //get the blocksize of the file
    blkcnt_t size_of_file = file_information.st_blocks;
    
    return size_of_file;
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

    //get the mode of the target
    mode_t mode_of_file = file_information.st_mode;

    return mode_of_file;
}

/**
 * @brief Function that checks the size of target when its a directory.
 * 
 * @param target_dir directory to check
 * @return size of directory as an int
 */
int dir_check(const char *target_dir)
{
    DIR *dir;
    struct dirent* direntp;
    int size = 0;

    //checks if directory is vaild or not.
    if (!(dir = opendir(target_dir)))
    {
        perror("Cant open directory!");
        exit(EXIT_FAILURE);
    }
    else
    {   
        //read all files in directory.
        while ((direntp = readdir(dir)) != NULL)
        {   
            //allocate memory for file_name and add null terminator to the first bit. 
            char *file_name = malloc(sizeof(*file_name)*1024);
            file_name[0] = '\0';

            //removes the "." and ".." from the directory.
            if ((strcmp(direntp->d_name, ".") == 0) || (strcmp(direntp->d_name, "..") == 0))
            {
                continue;
            }

            //copys directory and file name and adds / to file_name.
            strcat(file_name, target_dir);
            strcat(file_name, "/");
            strcat(file_name, direntp->d_name);

            //checks if file_name is a directory or not.
            if (S_ISDIR(check_target_mode(file_name)))
            {          
                size += dir_check(file_name);
            }
            else
            {   
                size += check_target_size(file_name);
            }

            free(file_name); 
        }

       closedir(dir);
    }

    return size;
}

void* check_target(char *argv[])
{

    int i = 1;
    int size = 0;
    mode_t mode_of_target;

    //loop to check if the target is a file or a directory.
    while (argv[i] != NULL)
    {  
        mode_of_target = check_target_mode(argv[i]);

        //if target is a file.
        if (S_ISREG(mode_of_target))
        {
            size = check_target_size(argv[i]);
            fprintf(stdout, "%d      ", size);
            fprintf(stdout, "%s\n", argv[i]);
            i++;
        }
        //if target is a directory.
        if (S_ISDIR(mode_of_target))
        {
            size += dir_check(argv[i]);
            fprintf(stdout, "%d      ", size);
            fprintf(stdout, "%s\n", argv[i]);
            i++;
        }
    }
}

void thread_func(int num_of_threads)
{   
    printf("%d\n", num_of_threads);

    //create empty queue
    queue *q = queue_empty(NULL);

    //check if queue was successfully created
    if (!queue_is_empty(q))
    {
        fprintf(stderr, "Empty queue creation failed!");
        exit(EXIT_FAILURE);
    }   

    pthread_t thread[num_of_threads]; 

    //create threads
    for (int i = 0; i < num_of_threads; i++)
    {
        if (pthread_create(&thread[i], NULL, &check_target, NULL) != 0)
        {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    //join threads
    for (int i = 0; i < num_of_threads; i++)
    {
        if (pthread_join(thread[i], NULL) != 0)
        {
            perror("Failed to join thread");
            exit(EXIT_FAILURE);
        }
    }
    

    



    
}

void print(const void *data)
{
    printf("[%d]", *(int*)data);
}