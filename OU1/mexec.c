/**
 * @file mexec.c
 * @author Jaffar El-Tai (hed20jei)
 * @brief Program that emulates a unix pipeline using C. 
 * @version 2.0
 * @date 2021-09-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#define MAX_LENGTH 1024
#define MAX_LENGTH_WITH_TERMINATORS 1026

//decliration of functions.
int number_of_commands(int argc, char *argv[], char *buffer, char **string_buffer);
void command_list_func(char *string_buffer, char command_list[][MAX_LENGTH]);
void tokenizing(char command_list[][MAX_LENGTH], int i,  char *command[]);
void child_process(int pipes[][2], pid_t p[], int numb_of_commands, char **string_buffer, char *buffer, int i);
int number_of_commands_counter(char *buffer, char **string_buffer, size_t size_of_string, FILE *file);

/**
 * @brief Main function that runs the pipeline program.
 * 
 * @param argc argument
 * @param argv number of arguments
 * @return Exit success if nothing went wrong. 
 */
int main(int argc, char *argv[])
{   
    //allocate memory and check if allocation was succsessfull for buffers
    char *buffer = malloc(sizeof(*buffer)*MAX_LENGTH_WITH_TERMINATORS);
    char *string_buffer = NULL;
    if (buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed!\n");
        return EXIT_FAILURE;
    }

    //Calling number of commands function and returns the number of commands to a variable.
    int numb_of_commands = number_of_commands(argc, argv, buffer, &string_buffer);
    
    //check if file is empty or not.
    if (numb_of_commands == 0)
    {
        fprintf(stderr, "File is empty\n");
        free(buffer);
        free(string_buffer);
        return EXIT_FAILURE;
    }
    
    // init of pipe and fork
    int pipes[numb_of_commands - 1][2];
    pid_t p[numb_of_commands];

    //checks if pipe was successfull or not
    for (int i = 0; i < numb_of_commands - 1; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("Pipe failed!");
            free(buffer);
            free(string_buffer);
            return EXIT_FAILURE;
        }
    }
    
    //checks if fork was successfull or not
    for (int i = 0; i < numb_of_commands; i++)
    {   
        if ((p[i] = fork()) < 0)
        {
            perror("Fork failed!");
            free(buffer);
            free(string_buffer);
            return EXIT_FAILURE;
        }
        //child process
        if (p[i] == 0)
        {   
            child_process(pipes, p, numb_of_commands, &string_buffer, buffer, i);
        }   
    }
    
    //closes pipes
    for (int i = 0; i < numb_of_commands - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    //wait until process has finished running.
    int wait_check;
    for (int i = 0; i < numb_of_commands; i++)
    {   
        wait(&wait_check);
        if (wait_check > 0)
        {
            free(buffer);
            free(string_buffer);
            return EXIT_FAILURE;
        }
    }
    
    //de-allocate memory and exit program.
    free(buffer);
    free(string_buffer);            
    return EXIT_SUCCESS;
}
/**
 * @brief Function that tokenizes the commandlist into commands
 * 
 * @param command_list list of commands
 * @param i number of commands
 * @param command pointer to the command that will be used.
 */
void tokenizing(char command_list[][MAX_LENGTH], int i, char *command[])
{
    char *p = command_list[i];
    char *token = strtok(p, " ");

    int j = 0;
    while (token != NULL)
    {
        command[j] = token;
        token = strtok(NULL, " ");
        j++;
    }
    //adding null at the end of the commands. 
    command[j] = NULL;
}

/**
 * @brief Function that makes a command list from string_buffer
 * 
 * @param string_buffer the old list of commands
 * @param command_list the new list of commands
 */
void command_list_func(char *string_buffer, char command_list[][MAX_LENGTH])
{   
    int index = 0;
    int numb_of_commands_check = 0;
    
    for (size_t i = 0; i <= strlen(string_buffer); i++)
    {
        if (string_buffer[i] == '\n' || string_buffer[i] == '\0')
        {
            command_list[numb_of_commands_check][index] = '\0';
            
            numb_of_commands_check++;
            index = 0;
        }
        else
        { 
            command_list[numb_of_commands_check][index] = string_buffer[i];
            index++;
        }
    }
}

/**
 * @brief Function that calculates the number of commands in file. 
 * 
 * @param argc number of arguments, ie file or commands
 * @param argv the actual command
 * @param buffer that holds commands
 * @param string_buffer the buffer that the @buffer sends commands to so it doesn't overwrite it. 
 * @return number of commands. 
 */
int number_of_commands(int argc, char *argv[], char *buffer, char **string_buffer)
{
    size_t size_of_string = 1;
    *string_buffer = malloc(sizeof(**string_buffer) * size_of_string);

    if (*string_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }

   *string_buffer[0] = '\0';

    int lines = 0;
     
    //checks amount of arguments sent in. Maximum of 2. 
    if(argc > 2)
    {
        fprintf(stderr, "usage: ./mexec [FILE]\n");
        free(buffer);
        free(*string_buffer);
        exit(EXIT_FAILURE);
    }
    //checks if file is empty or not without the "<". 
    else if(argc == 2)
    {   
        //opens file to check if file is empty or not. 
        FILE *fp = fopen(argv[1], "r");
    
        //check if file exists.
        if (fp == NULL)
        {
            perror(argv[1]);
            free(buffer);
            free(*string_buffer);     
            exit(EXIT_FAILURE);
        }
        else
        {    
            lines = number_of_commands_counter(buffer, string_buffer, size_of_string, fp);
        }

        fclose(fp);
    }
    //with the "<"
    else
    {   
        lines = number_of_commands_counter(buffer, string_buffer, size_of_string, stdin);
    }

    return lines;
}

/**
 * @brief Function that runs everything in the child process.
 * 
 * @param pipes the pipes in use
 * @param p the forks
 * @param numb_of_commands the number of commands in the file 
 * @param string_buffer buffer that stores the strings
 * @param buffer first buffer that stores all data from file
 * @param i index
 */
void child_process(int pipes[][2], pid_t p[], int numb_of_commands, char **string_buffer, char *buffer, int i)
{
    //closes the pipes that are not in use.
    for (int j = 0; j < numb_of_commands - 1; j++)
    {
        if (i != j)
        {
            close(pipes[j][1]);
        }
        if (i - 1 != j)
        {
            close(pipes[j][0]);
        }
    }
    
    char command_list[numb_of_commands][MAX_LENGTH];

    //calling function to get a list of commands.
    command_list_func(*string_buffer, command_list);
    
    char *command[MAX_LENGTH];

    //checks if there are more than just one command that has to run.
    if (numb_of_commands > 1)
    {   
        //dup for first child
        if (i == 0)
        {
            if (dup2(pipes[i][1], STDOUT_FILENO) < 0)
            {
                perror("Dup failed!");
                close(pipes[i][1]);
                free(buffer);
                free(*string_buffer);
                exit(EXIT_FAILURE);
            }
        }
        //dup for last child
        else if (i == (numb_of_commands - 1))
        {
            if (dup2(pipes[i-1][0], STDIN_FILENO) < 0)
            {
                perror("Dup failed!");
                close(pipes[i-1][0]);
                free(buffer);
                free(*string_buffer);
                exit(EXIT_FAILURE);
            }
        }
        //dup for every other child
        else
        {   
            if (dup2(pipes[i-1][0], STDIN_FILENO) < 0)
            {
                perror("Dup failed!");
                close(pipes[i-1][0]);
                free(buffer);
                free(*string_buffer);
                exit(EXIT_FAILURE);
            }
            if (dup2(pipes[i][1], STDOUT_FILENO) < 0)
            {
                perror("Dup failed!");
                close(pipes[i][1]);
                free(buffer);
                free(string_buffer);
                exit(EXIT_FAILURE);
            }
        }
    }

    //tokenizing commands and sending them to a new array.
    tokenizing(command_list, i, command);

    //check if exec returned any errors after running.
    if (execvp(command[0], command) < 0)
    {
        perror(command[0]);
        free(buffer);
        free(*string_buffer);
        exit(EXIT_FAILURE);
    }
}
/**
 * @brief helper function that helps with the counting of lines in number_of_commands function
 * 
 * @param buffer buffer that holds commands
 * @param string_buffer buffer that @buffer sends strings to so it doesn't get overwritten.
 * @param size_of_string checks the size of string
 * @param fp file to check or stdin
 * @return lines
 */
int number_of_commands_counter(char *buffer, char **string_buffer, size_t size_of_string, FILE *fp)
{
    int lines = 0;

    //count amount of commands in the file.     
    while (fgets(buffer, MAX_LENGTH_WITH_TERMINATORS, fp) != NULL)
    {    
        //check the maximum amout of characters.
        if (strlen(buffer) <= MAX_LENGTH_WITH_TERMINATORS)
        {
            *string_buffer = realloc(*string_buffer, sizeof(**string_buffer)*strlen(buffer)+size_of_string);
            if (*string_buffer == NULL)
            {
                fprintf(stderr, "Memory allocation failed!\n");
                exit(EXIT_FAILURE);
            }
            size_of_string = sizeof(**string_buffer)*strlen(buffer)+size_of_string;
            strcat(*string_buffer, buffer);
            lines++;
        }
        else
        {
            fprintf(stderr, "Command length longer than 1024 characters\n");
            free(buffer);
            free(*string_buffer);      
            exit(EXIT_FAILURE);
        } 
    }

    return lines;      
}