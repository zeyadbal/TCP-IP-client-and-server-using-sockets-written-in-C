#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "line_parser.h"

#ifndef NULL
    #define NULL 0
#endif

#define FREE(X) if(X) free((void*)X)

static char *clone_first_word(char *str)
{
    char *start = NULL;
    char *end = NULL;
    char *word;

    while (!end) {
        switch (*str) {
            case '>':
            case '<':
            case 0:
                end = str - 1;
                break;
            case ' ':
                if (start)
                    end = str - 1;
                break;
            default:
                if (!start)
                    start = str;
                break;
        }
        str++;
    }

    if (start == NULL) {
        return NULL;
	}

    word = (char*) malloc(end-start+2);
    strncpy(word, start, ((int)(end-start)+1)) ;
    word[ (int)((end-start)+1)] = 0;

    return word;
}

static void extract_redirections(char *str_line, cmd_line *line)
{
    char *s = str_line;

    while ( (s = strpbrk(s,"<>")) ) {
        if (*s == '<') {
            FREE(line->input_redirect);
            line->input_redirect = clone_first_word(s+1);
        }
        else {
            FREE(line->output_redirect);
            line->output_redirect = clone_first_word(s+1);
        }

        *s++ = 0;
    }
}

static char *str_clone(const char *source)
{
    char* clone = (char*)malloc(strlen(source) + 1);
    strcpy(clone, source);
    return clone;
}

static int is_empty(const char *str)
{
  if (!str)
    return 1;
  
  while (*str) {
    if (!isspace(*(str++))) {
      return 0;
	}
  }  
  return 1;
}

static cmd_line *parse_single_cmd_line(const char *str_line)
{
    char *delimiter = " ";
    char *line_cpy, *result;
    
    if (is_empty(str_line)){
      return NULL;
	}
    
    cmd_line* line = (cmd_line*)malloc( sizeof(cmd_line) ) ;
    memset(line, 0, sizeof(cmd_line));
    
    line_cpy = str_clone(str_line);
         
    extract_redirections(line_cpy, line);
    
    result = strtok(line_cpy, delimiter);    
    while( result && line->arg_count < MAX_ARGUMENTS-1) {
        ((char**)line->arguments)[line->arg_count++] = str_clone(result);
        result = strtok (NULL, delimiter);
    }

    FREE(line_cpy);
    return line;
}

static cmd_line* _parse_cmd_lines(char *line_str)
{
	char *next_str_cmd;
	cmd_line *line;
	char pipe_delimiter = '|';
	
	if (is_empty(line_str)) {
	  return NULL;
	}
	
	next_str_cmd = strchr(line_str , pipe_delimiter);
	if (next_str_cmd) {
	  *next_str_cmd = 0;
	}
	
	line = parse_single_cmd_line(line_str);
	if (!line) {
	  return NULL;
	}
	
	if (next_str_cmd)
	  line->next = _parse_cmd_lines(next_str_cmd+1);

	return line;
}

cmd_line *parse_cmd_lines(const char *str_line)
{
	char* line, *ampersand;
	cmd_line *head, *last;
	int idx = 0;
	
	if (is_empty(str_line)) {
	  return NULL;
	}
	
	line = str_clone(str_line);
	if (line[strlen(line)-1] == '\n') {
	  line[strlen(line)-1] = 0;
	}
	
	ampersand = strchr( line,  '&');
	if (ampersand) {
	  *(ampersand) = 0;
	}
		
	if ( (last = head = _parse_cmd_lines(line)) )
	{	
	  while (last->next) {
	    last = last->next;
	  }
	  last->blocking = ampersand? 0:1;
	}
	
	for (last = head; last; last = last->next) {
		last->idx = idx++;
	}
			
	FREE(line);
	return head;
}


void free_cmd_lines(cmd_line *line)
{
  int i;
  if (!line)
    return;

  FREE(line->input_redirect);
  FREE(line->output_redirect);
  for (i=0; i<line->arg_count; ++i)
      FREE(line->arguments[i]);

  if (line->next)
	  free_cmd_lines(line->next);

  FREE(line);
}

int replace_cmd_arg(cmd_line *line, int num, const char *new_string)
{
  if (num >= line->arg_count)
    return 0;
  
  FREE(line->arguments[num]);
  ((char**)line->arguments)[num] = str_clone(new_string);
  return 1;
}
