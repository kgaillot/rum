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
    rum_attr_t shelf_attrs[] = { { "id", 0 } };
    rum_attr_t bottle_attrs[] = { { "type", 1 }, { "aged", 0 }, { "vintage", 0 } };
    rum_attr_t glass_attrs[] = { { "type", 1 } };

    /* memory leak in case of error here, but since the program exits in such a case, not worrying about it */
    if (((cabinet = rum_tag_new(NULL, "cabinet", 0, 0, NULL)) == NULL)
    || ((shelf = rum_tag_new(cabinet, "shelf", 0, sizeof(shelf_attrs) / sizeof(rum_attr_t), shelf_attrs)) == NULL)
    || ((bottle = rum_tag_new(shelf, "bottle", 0, sizeof(bottle_attrs) / sizeof(rum_attr_t), bottle_attrs)) == NULL)
    || ((glass = rum_tag_new(shelf, "glass", 1, sizeof(glass_attrs) / sizeof(rum_attr_t), glass_attrs)) == NULL)) {
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
