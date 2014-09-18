/*
    rum.c

    application to parse and display rudimentary markup 

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <rump.h>

#define DEBUG 0

static int
display_cabinet(const rum_element_t *cabinet)
{
    if (rum_element_get_first_child(cabinet)) {
        printf("The cabinet contains:\n");
    } else {
        printf("The cabinet is empty.\n");
    }
    return 0;
}

static int
display_shelf(const rum_element_t *shelf)
{
    const char *id = rum_element_get_value(shelf, "id");

    printf("   The");
    if (id) {
        printf(" %s", id);
    }
    if (rum_element_get_first_child(shelf)) {
        printf(" shelf contains:\n");
    } else {
        printf(" shelf is empty.\n");
    }
    return 0;
}

static int
display_bottle(const rum_element_t *bottle)
{
    const char *bottle_type = rum_element_get_value(bottle, "type");
    const char *aged = rum_element_get_value(bottle, "aged");
    const char *vintage = rum_element_get_value(bottle, "vintage");
    const char *maker = rum_element_get_content(bottle);

    printf("      A");
    if (vintage) {
        printf(" %s", vintage);
    }
    if (aged) {
        printf(" %s-year-old", vintage);
    }
    printf(" bottle");
    if (maker || bottle_type) {
        printf(" of");
    }
    if (maker) {
        printf(" %s", maker);
    }
    if (bottle_type) {
        printf(" %s", bottle_type);
    }
    printf("\n");
    return 0;
}

static int
display_glass(const rum_element_t *glass)
{
    const char *glass_type = rum_element_get_value(glass, "type");
    printf("      A %s\n", glass_type? glass_type : "glass");
    return 0;
}

static rum_tag_t *
define_language()
{
    rum_tag_t *cabinet, *shelf, *bottle, *glass;
    rum_attr_t shelf_attrs[] = { { "id", 0 } };
    rum_attr_t bottle_attrs[] = { { "type", 1 }, { "aged", 0 }, { "vintage", 0 } };
    rum_attr_t glass_attrs[] = { { "type", 1 } };
    int shelf_nattrs = sizeof(shelf_attrs) / sizeof(rum_attr_t);
    int bottle_nattrs = sizeof(bottle_attrs) / sizeof(rum_attr_t);
    int glass_nattrs = sizeof(glass_attrs) / sizeof(rum_attr_t);

    if (((cabinet = rum_tag_new(NULL, "cabinet", 0, 0, NULL, &display_cabinet)) == NULL)
    || ((shelf = rum_tag_new(cabinet, "shelf", 0, shelf_nattrs, shelf_attrs, &display_shelf)) == NULL)
    || ((bottle = rum_tag_new(shelf, "bottle", 0, bottle_nattrs, bottle_attrs, &display_bottle)) == NULL)
    || ((glass = rum_tag_new(shelf, "glass", 1, glass_nattrs, glass_attrs, &display_glass)) == NULL)) {
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
        if (infile != stdin) {
            fclose(infile);
        }
        return 1;
    }
    if (DEBUG) {
        rum_display_language(language);
    }

    /* parse file */
    if ((document = rum_parse_file(infile, language)) == NULL) {
        if (infile != stdin) {
            fclose(infile);
        }
        return 1;
    }

    /* display the document in all its exalted glory */
    if (rum_element_display(document) < 0) {
        if (infile != stdin) {
            fclose(infile);
        }
        return 1;
    }

    /* wrap it up */
    if (infile != stdin) {
        fclose(infile);
    }
    return 0;
}
