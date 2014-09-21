/*
    rump.h

    header for RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#ifndef RUM_RUMP__H
#define RUM_RUMP__H

#include <stdio.h>
#include <rum_types.h>
#include <rum_buffer.h>
#include <rum_parser.h>
#include <rum_language.h>
#include <rum_document.h>

/* return the last error message from a RuM parser library function */
char *rum_last_error();

/* return a document object, parsed from an open file stream according to a language */
rum_element_t *rum_parse_file(FILE *fp, const rum_tag_t *language, int print_input_on_error);

#endif /* RUM_RUMP__H */
