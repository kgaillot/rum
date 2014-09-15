/*
    rum.c

    application to parse and display rudimentary markup 

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <rump.h>

#define DEBUG 0

static rum_tag_t *
define_language()
{
    rum_tag_t *cabinet, *shelf, *bottle, *glass;
    static rum_attr_t shelf_attrs[] = { { "id", 0 }, { NULL, 0 } };
    static rum_attr_t bottle_attrs[] = {
        { "type", 1 }, { "aged", 0 }, { "vintage", 0 }, { NULL, 0 }
    };
    static rum_attr_t glass_attrs[] = { { "type", 1 }, { NULL, 0 } };

    if (((cabinet = rum_tag_insert(NULL, "cabinet", 0, NULL)) == NULL)
    || ((shelf = rum_tag_insert(cabinet, "shelf", 0, shelf_attrs)) == NULL)
    || ((bottle = rum_tag_insert(shelf, "bottle", 0, bottle_attrs)) == NULL)
    || ((glass = rum_tag_insert(shelf, "glass", 1, glass_attrs)) == NULL)) {
        return NULL;
    }
    return(cabinet);
}

int
main(int argc, char **argv)
{
    rum_tag_t *language;
    rum_element_t *document;
    FILE *infile;

    /* trivial command line parsing -- read from standard input or filename */
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [<file>]\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        if ((infile = fopen(argv[1], "r")) == NULL) {
            fprintf(stderr, "Could not open %s\n", argv[1]);
            return 1;
        }
    } else {
        infile = stdin;
    }

    /* define the sample language */
    if ((language = define_language()) == NULL) {
        fprintf(stderr, "Internal error: could not define sample language\n");
        return 1;
    }
    if (DEBUG) {
        rum_display_language(language);
    }

    /* parse file */
    if ((document = rum_parse_file(infile, language)) == NULL) {
        return 1;
    }

    /* @TODO display document content */

    /* wrap it up */
    if (infile != stdin) {
        fclose(infile);
    }
    return 0;
}
