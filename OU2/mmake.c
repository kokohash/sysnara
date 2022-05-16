/**
 * @file mmake.c
 * @author Jaffar El-Tai (hed20jei)
 * @brief Program that emulates make command i unix
 * @version 1.0
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
#include <sys/stat.h>
#include <stdbool.h>
#include "parser.h"

//decliration of functions.
bool check_files(const char *target, const char **prereq);
void target_builder(const char *target, makefile *m_file, int f, int B, int s);
FILE *open_default(FILE *file);
makefile *parse_makefile_func(FILE *file);
void target_builder_func(makefile *m_file, int f, int B, int s, char *argv[]);

/**
 * @brief Main function that runs the program
 * 
 * @param argc number of arguments
 * @param argv the argument
 * @return int 
 */
int main(int argc, char *argv[])
{
    char flag;
    int f = 0;
    int B = 0; 
    int s = 0;
    FILE *file;
    makefile *m_file;
    
    //loop to catch the correct flags
    while ((flag = getopt(argc, argv, "f:Bs")) != -1)
    {
        switch (flag)
        {
        case 'f':
            f = 1;

            if ((file = fopen(optarg, "r")) == NULL)
            {
                perror(optarg);
                return EXIT_FAILURE;
            }       
            break;
        case 'B':
            B = 1;
            break;
        case 's':
            s = 1;
            break;
        default:
            fprintf(stderr, "No valid flag!\n");
            return EXIT_FAILURE;
        }
    }
    
    //if no flag is used, open default 
    if (f == 0)
    {
        file = open_default(file);
    }
    
    //parse the makefile
    m_file = parse_makefile_func(file);

    //build the target
    target_builder_func(m_file, f, B, s, argv);
    
    //close files and close program
    fclose(file);
    makefile_del(m_file);
    return EXIT_SUCCESS;
}

/**
 * @brief Function that checks if the target file has been edited
 * 
 * @param target target to check
 * @param prereq prerequsits to check
 * @return true 
 * @return false 
 */
bool check_files(const char *target, const char **prereq)
{
    struct stat file_information;

    //check if target stats was returned correctly
    if (stat(target, &file_information) < 0)
    {
        perror(target);
        exit(EXIT_FAILURE);
    }

    //set time_target and time_prereq
    time_t time_target = file_information.st_mtime;
    time_t time_prereq;

    //loop through every prerequisit
    for (int i = 0; prereq[i] != NULL; i++)
    {
        //check if target stats was returned correctly
        if (stat(prereq[i], &file_information) < 0)
        {
            perror(target);
            exit(EXIT_FAILURE);
        }

        time_prereq = file_information.st_mtime;

        //if there is a time differance in the files
        if (time_prereq > time_target)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Function that sets the default target if no target is set.
 * 
 * @param file file to set as default
 * @return FILE* as default
 */
FILE *open_default(FILE *file)
{
    if ((file = fopen("mmakefile", "r")) == NULL)
    {
        perror(optarg);
        exit(EXIT_FAILURE);
    }

    return file;
}

/**
 * @brief Function that parses the makefile from file
 * 
 * @param file to parse
 * @return makefile* 
 */
makefile *parse_makefile_func(FILE *file)
{   
    makefile *m_file;

    if ((m_file = parse_makefile(file)) == NULL)
    {
        fprintf(stderr, "mmakefile: Could not parse makefile\n");
        exit(EXIT_FAILURE);
    }

    return m_file;
}

/**
 * @brief Function that build the target
 * 
 * @param m_file the makefile 
 * @param f flag
 * @param B flag
 * @param s flag
 * @param argv argument
 */
void target_builder_func(makefile *m_file, int f, int B, int s, char *argv[])
{
    int index = optind;
    char *target = argv[index];

    if (target == NULL)
    {
        target_builder(makefile_default_target(m_file), m_file, f, B, s);
    }
    else
    {
        while (target != NULL)
        {
            target_builder(target, m_file, f, B, s);
            index++;
            target = argv[index];
        }
    }
}

/**
 * @brief Function that builds the target from rules and prerequisit.
 * 
 * @param target target to build
 * @param m_file the makefile 
 * @param f flag
 * @param B flag
 * @param s flag
 */
void target_builder(const char *target, makefile *m_file, int f, int B, int s)
{
    rule *rules = makefile_rule(m_file, target);

    //get the rules 
    if (rules == NULL)
    {   
        //check if the target is accessable
        if (!(access(target, F_OK) == 0))
        {
            fprintf(stderr, "No target!\n");
            exit(EXIT_FAILURE);
        }    
       return; 
    }
    
    const char **prereq = rule_prereq(rules);

    //send in the prerequisits 
    for (int i = 0; prereq[i] != NULL; i++)
    {
        target_builder(prereq[i], m_file, f, B, s);
    }

    //check if file is accessable or B flag or if file has been edited
    if (!(access(target, F_OK) == 0) || B || check_files(target, prereq))
    {
        char **command = rule_cmd(rules);

        pid_t pid = fork();

        //checks if fork failed or not
        if (pid < 0)
        {
            perror("Fork failed!\n");
            exit(EXIT_FAILURE);
        }

        //child process
        else if (pid == 0)
        {
            //execute command
            if (execvp(*command, command) < 0)
            {
                perror(*command);
                exit(EXIT_FAILURE);
            }
        }
        if (s == 0)
        {   
            //print to stdout
            for (int i = 0; command[i] != NULL; i++) 
            {
                if (i != 0)
                {
                    printf(" ");
                }

                printf("%s", command[i]);
            }
            printf("\n");
        }
        
        //wait loop
        int wait_check;
        wait(&wait_check);
        if (wait_check > 0)
        {
            exit(EXIT_FAILURE);
        }
    }  
}