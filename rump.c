/*
    rump.c

    high-level functions for RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rump.h>
#include "rum_private.h"

/* error handling for the library consists of a global error message;
 * all library functions must set this to NULL on success, and error message otherwise
 */
static char *rum_errmsg;

char *
rum_last_error()
{
    return rum_errmsg;
}

void
rum_set_error(char *errmsg)
{
    rum_errmsg = errmsg;
}

/* error handling: ensure last error message is retained, free allocated memory, return NULL */
static rum_element_t *
rum_parse_error(rum_parser_t **headp, rum_buffer_t *buffer, int print_input_on_error)
{
    char *errmsg = rum_last_error();

    if (print_input_on_error) {
        rum_buffer_print(buffer, stderr);
        fprintf(stderr, "\n\n");
    }
    /*@TODO pop all method and use it to free head */
    rum_buffer_free(buffer);
    rum_set_error(errmsg);
    return NULL;
}

rum_element_t *
rum_parse_file(FILE *fp, const rum_tag_t *language, int print_input_on_error)
{
    int c;
    rum_parser_t *head = NULL;
    rum_buffer_t *buffer = NULL;
    rum_element_t *element, *document = NULL;

    rum_set_error(NULL);
    if ((head = rum_parser_new()) == NULL) {
        return rum_parse_error(&head, buffer, print_input_on_error);
    }

    /* keep the already-processed XML in a buffer, for back references and error reporting */
    if ((buffer = rum_buffer_new()) == NULL) {
        return rum_parse_error(&head, buffer, print_input_on_error);
    }

    /* parse input a character at a time */
    while ((c = getc(fp)) != EOF) {
        element = rum_parser_parse_char(&head, language, buffer, c);
        if (rum_last_error() || (rum_buffer_add_char(buffer, c) < 0)) {
            return rum_parse_error(&head, buffer, print_input_on_error);
        }
        if (element) {
            document = element;
        }
    }

    /* @TODO: return error if all tags not closed */

    /*@TODO pop all */
    rum_buffer_free(buffer);
    return document;
}
