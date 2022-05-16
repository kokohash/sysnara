/**
 * Parser to parse a minimal makefile.  This file is provided to students along
 * with its header-file parser.h to solve the mmake laboration in the course C
 * Programming and Unix (5DV088).
 *
 * @file parse.h
 * @author Elias Åström, Fredrik Peteri
 * @date 2020-09-04
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

#define MAX_RULES 256
#define MAX_LINE 1024
#define MAX_PREREQ 32
#define MAX_CMD 32

struct makefile {
	struct rule *rules;
};

struct rule {
	char *target;
	char **prereq;
	char **cmd;
	rule *next;
};

/**
 * Check if line is blank.
 */
static bool is_blank_line(const char *s)
{
	for (size_t i = 0; s[i] != '\0'; i++) {
		if (!isspace(s[i]))
			return false;
	}
	return true;
}

/**
 * Parse a word and update p to point to the first character after the word.
 * The word is delimited by whitespace and any character in delim.  The
 * returned string should be freed using free.
 */
static char *parse_word(char **p, char *delim)
{
	size_t n = 0;
	while (!isspace((*p)[n]) && strchr(delim, (*p)[n]) == NULL)
		n++;

	if (n == 0)
		return NULL;

	char *path = strndup(*p, n);
	*p += n;
	return path;
}

/**
 * Fills buf with the next line from fp.  Returns buf if a line was read and
 * NULL otherwise.
 */
static char *next_line(char buf[MAX_LINE], FILE *fp)
{
	do {
		if (fgets(buf, MAX_LINE, fp) == NULL)
			return NULL;
	} while (is_blank_line(buf));

	return buf;
}

/**
 * Duplicate an array of strings.
 *
 * @param n     Size of array to duplicate.
 * @param a     Array to duplicate.
 * @return      NULL-terminated array which should be freed using free.
 */
static char **dupe_str_array(size_t n, char **a)
{
	char **ret = malloc((n + 1) * sizeof *ret);

	for (size_t i = 0; i < n; i++)
		ret[i] = a[i];
	ret[n] = NULL;

	return ret;
}

/**
 * Advance pointer to the next character which is not a space, stops at
 * newline.
 */
static void skipwhite(char **p)
{
	while (isspace(**p) && **p != '\n')
		(*p)++;
}

/**
 * Check that the character pointed to by p is c, and increment p if it is.
 */
static bool expect(char **p, char c)
{
	if (**p != c)
		return false;

	(*p)++;
	return true;
}

/**
 * Parse a rule.
 *
 * @param fp    File to read from.
 * @param err   Pointer to flag which gets set to true on error.
 * @return      A parsed rule or NULL.
 */
static rule *parse_rule(FILE *fp, bool *err)
{
	char buf[MAX_LINE];
	char *p;

	// read line with target and prerequisites
	if ((p = next_line(buf, fp)) == NULL)
		return NULL;

	// line cannot begin with whitespace
	if (isspace(*p))
		goto err0;

	char *target = parse_word(&p, ":");

	skipwhite(&p);

	if (!expect(&p, ':'))
		goto err1;

	skipwhite(&p);

	// parse prerequisites
	char *prereq[MAX_PREREQ];
	size_t n_prereq = 0;
	while (n_prereq < MAX_PREREQ
			&& (prereq[n_prereq] = parse_word(&p, "")) != NULL) {
		n_prereq++;
		skipwhite(&p);
	}
	if (!expect(&p, '\n'))
		goto err2;

	// read line with command
	if ((p = next_line(buf, fp)) == NULL)
		goto err2;

	// command has to begin with tab
	if (!expect(&p, '\t'))
		goto err2;

	skipwhite(&p);

	// parse command
	char *cmd[MAX_CMD];
	size_t n_cmd = 0;
	while (n_cmd < MAX_CMD && (cmd[n_cmd] = parse_word(&p, "")) != NULL) {
		n_cmd++;
		skipwhite(&p);
	}

	// create rule
	rule *r = malloc(sizeof *r);
	r->target = target;
	r->prereq = dupe_str_array(n_prereq, prereq);
	r->cmd = dupe_str_array(n_cmd, cmd);

	return r;

err2:
	for (size_t i = 0; i < n_prereq; i++)
		free(prereq[i]);
err1:
	free(target);
err0:
	*err = true;
	return NULL;
}

/**
 * Parse a makefile.
 *
 * @param path  Path to makefile to parse.
 * @return      The makefile.
 */
makefile *parse_makefile(FILE *fp)
{
	makefile *m = malloc(sizeof *m);
	rule **tailp = &m->rules;

	bool err = false;
	while ((*tailp = parse_rule(fp, &err)) != NULL)
		tailp = &(*tailp)->next;
	*tailp = NULL;

	if (m->rules == NULL || err) {
		makefile_del(m);
		return NULL;
	}

	return m;
}

/**
 * Get the default target for a makefile.  The default target is the target
 * from the first rule.
 *
 * @param make  The makefile.
 * @return      Name of the default target for the makefile.
 */
const char *makefile_default_target(makefile *m)
{
	return m->rules->target;
}

/**
 * Get the rule for building a specific target in a makefile.
 *
 * @param make      The makefile.
 * @param target    Name of a target.
 * @return          The rule for building the target.
 */
rule *makefile_rule(makefile *m, const char *target)
{
	for (rule *i = m->rules; i != NULL; i = i->next)
		if (strcmp(i->target, target) == 0)
			return i;

	return NULL;
}

/**
 * Get the prerequisites for a rule.
 *
 * @param rule  The rule.
 * @return      Array containing the prerequisites for the rule.  The array is
 *              terminated with NULL.
 */
const char **rule_prereq(rule *rule)
{
	return (const char **)rule->prereq;
}

/**
 * Get the command for a rule.
 *
 * @param rule  The rule.
 * @return      Array containing the arguments for the command used to build
 *              the rule.  The first argument is the name of the command.  The
 *              array is terminated with NULL.
 */
char **rule_cmd(rule *rule)
{
	return rule->cmd;
}

/**
 * Recursively delete a list of rules.
 */
static void del_rules(struct rule *rules)
{
	if (rules == NULL)
		return;

	free(rules->target);

	for (size_t i = 0; rules->prereq[i] != NULL; i++)
		free(rules->prereq[i]);
	free(rules->prereq);

	for (size_t i = 0; rules->cmd[i] != NULL; i++)
		free(rules->cmd[i]);
	free(rules->cmd);

	del_rules(rules->next);

	free(rules);
}

/**
 * Free the memory of a makefile.  This will also delete the rules from the
 * makefile returned by makefile_rule.
 *
 * @param make  Makefile to delete.
 */
void makefile_del(makefile *make)
{
	del_rules(make->rules);
	free(make);
}
