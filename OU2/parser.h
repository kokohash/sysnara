/**
 * Parser to parse a minimal makefile.  This file is provided to students along
 * with its implementation file parser.c to solve the mmake laboration in the
 * course C Programming and Unix (5DV088).
 *
 * @file parse.h
 * @author Elias Åström, Fredrik Peteri
 * @date 2020-09-04
 */
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

typedef struct makefile makefile;
typedef struct rule rule;

/**
 * Parse a makefile.
 *
 * @param fp    The file to parse.
 * @return      The makefile.
 */
makefile *parse_makefile(FILE *fp);

/**
 * Get the default target for a makefile.  The default target is the target
 * from the first rule.
 *
 * @param make  The makefile.
 * @return      Name of the default target for the makefile.
 */
const char *makefile_default_target(makefile *make);

/**
 * Get the rule for building a specific target in a makefile.
 *
 * @param make      The makefile.
 * @param target    Name of a target.
 * @return          The rule for building the target.
 */
rule *makefile_rule(makefile *make, const char *target);

/**
 * Get the prerequisites for a rule.
 *
 * @param rule  The rule.
 * @return      Array containing the prerequisites for the rule.  The array is
 *              terminated with NULL.
 */
const char **rule_prereq(rule *rule);

/**
 * Get the command for a rule.
 *
 * @param rule  The rule.
 * @return      Array containing the arguments for the command used to build
 *              the rule.  The first argument is the name of the command.  The
 *              array is terminated with NULL.
 */
char **rule_cmd(rule *rule);

/**
 * Free the memory of a makefile.  This will also delete the rules returned by
 * makefile_rule.
 *
 * @param make  Makefile to delete.
 */
void makefile_del(makefile *make);

#endif // !defined PARSER_H
