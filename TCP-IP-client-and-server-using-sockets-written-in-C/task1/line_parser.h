#define MAX_ARGUMENTS 256

typedef struct cmd_line
{
    char * const arguments[MAX_ARGUMENTS]; /* command line arguments (arg 0 is the command)*/
    int arg_count;		/* number of arguments */
    char const *input_redirect;	/* input redirection path. NULL if no input redirection */
    char const *output_redirect;	/* output redirection path. NULL if no output redirection */
    char blocking;	/* boolean indicating blocking/non-blocking */
    int idx;				/* index of current command in the chain of cmd_lines (0 for the first) */
    struct cmd_line *next;	/* next cmd_line in chain */
} cmd_line;

/* Parses a given string to arguments and other indicators */
/* Returns NULL when there's nothing to parse */ 
/* When successful, returns a pointer to cmd_line (in case of a pipe, this will be the head of a linked list) */
cmd_line *parse_cmd_lines(const char *str_line);	/* Parse string line */

/* Releases all allocated memory for the chain (linked list) */
void free_cmd_lines(cmd_line *line);		/* Free parsed line */

/* Replaces arguments[num] with new_string */
/* Returns 0 if num is out-of-range, otherwise - returns 1 */
int replace_cmd_arg(cmd_line *line, int num, const char *new_string);
