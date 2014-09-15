/*
    rump.h

    header for RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#ifndef RUM_RUMP__H
#define RUM_RUMP__H

#include <stdio.h>
#include <rum_language.h>
#include <rum_document.h>

/* return a document object, parsed from an open file stream according to a language */
rum_element_t *rum_parse_file(FILE *fp, const rum_tag_t *language);

#endif /* RUM_RUMP__H */
