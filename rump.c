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
    /* save error message because later calls will wipe it */
    char *errmsg = rum_last_error();

    if (print_input_on_error) {
        rum_buffer_print(buffer, stderr);
        fprintf(stderr, "\n\n");
    }
    rum_parser_free(headp);
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

        /* the first parser state (before any tag is encountered) will have an empty element;
         * the second parser state will have the root element, and will be the last
         * to be popped off, so save the last non-NULL element, which is the document
         */
        if (element) {
            document = element;
        }
    }

    if (document == NULL) {
        rum_set_error("Root tag not found in input");
        return rum_parse_error(&head, buffer, print_input_on_error);
    }

    /* if the last parsed element has a parent, then it's not the root tag;
     * if the last parsed element is also the last element, then the root tag wasn't closed
     */
    if ((element == document) || (rum_element_get_parent(document) != NULL)) {
        rum_set_error("All tags not closed");
        return rum_parse_error(&head, buffer, print_input_on_error);
    }

    rum_parser_free(&head);
    rum_buffer_free(buffer);
    return document;
}
