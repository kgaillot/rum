/*
    rump.c

    library to parse Rudimentary Markup 

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rump.h>
#include "rump_private.h"

static char
entity2char(char *entity)
{
    if (entity == NULL) {
        return 0;
    } else if (!strcmp(entity, "&lt;")) {
        return '<';
    } else if (!strcmp(entity, "&gt;")) {
        return '>';
    } else if (!strcmp(entity, "&amp;")) {
        return '&';
    } else if (!strcmp(entity, "&apos;")) {
        return '\'';
    } else if (!strcmp(entity, "&quot;")) {
        return '\"';
    }
    return 0;
}

static char*
copy_substring(const char *s, size_t start, size_t end)
{
    char *str;
    size_t len = end - start + 1;

    if ((str = malloc(len + 1)) != NULL) {
        strncpy(str, s + start, len);
        str[len] = 0;
    }
    return(str);
}

static rum_parser_t *
new_parser(const rum_tag_t *language)
{
    rum_parser_t *parser;

    if ((language == NULL) || ((parser = malloc(sizeof(rum_parser_t))) == NULL)) {
        return NULL;
    }
    if ((parser->buf = malloc(CHUNKSIZE)) == NULL) {
        free(parser);
        return NULL;
    }

    parser->language = language;
    parser->state = OUTSIDE_MARKUP;
    parser->nchunks = 1;
    parser->pos = 0;
    parser->substr_start = 0;
    parser->substr_end = 0;
    parser->quote_char = 0;
    parser->document = NULL;
    parser->element = NULL;
    parser->parent = NULL;
    return(parser);
}

static void
free_parser(rum_parser_t *parser)
{
    if (parser) {
        if (parser->buf) {
            free(parser->buf);
        }
        free(parser);
    }
}

static void
print_input(rum_parser_t *parser, FILE *fp)
{
    if (parser && parser->buf) {
        parser->buf[parser->pos] = 0;
        fputs(parser->buf, stderr);
    }
}

static int
add_char(rum_parser_t *parser, int c)
{
    char *newbuf;

    parser->buf[(parser->pos)++] = c;

    /* grow the buffer if needed (leaving room for a null byte) */
    if (parser->pos == ((parser->nchunks * CHUNKSIZE) - 1)) {
        ++(parser->nchunks);
        if ((newbuf = realloc(parser->buf, parser->nchunks * CHUNKSIZE)) == NULL) {
            parser->buf[parser->pos] = 0;
            print_input(parser, stderr);
            fprintf(stderr, "\n\n*** ERROR: Could not reallocate input buffer\n");
            return -1;
        }
        parser->buf = newbuf;
    }
    return 0;
}

static int
start_tag(rum_parser_t *parser)
{
    char *tag_name;

    if (parser->element == NULL) {
        if ((tag_name = copy_substring(parser->buf, parser->substr_start, parser->substr_end)) == NULL) {
            fprintf(stderr, "\n\n*** ERROR: Could not allocate memory for tag name\n");
            return -1;
        }
        if ((parser->element = rum_element_insert(parser->parent, parser->language, tag_name)) == NULL) {
            print_input(parser, stderr);
            fprintf(stderr, "\n\n*** ERROR: Invalid tag '%s'\n", tag_name);
            free(tag_name);
            return -1;
        }
        parser->substr_start = parser->substr_end = 0;
        parser->parent = parser->element;

        /* if this is the first tag encountered, it must be the root tag */
        if (parser->document == NULL) {
            if (parser->element->tag != parser->language) {
                fprintf(stderr, "\n\n*** ERROR: Tag '%s' found outside root tag\n", tag_name);
                return -1;
            }
            parser->document = parser->element;
        }
        free(tag_name);
    }
    return 0;
}

rum_element_t *
rum_parse_file(FILE *fp, const rum_tag_t *language)
{
    int c;
    rum_parser_t *parser;

    if ((parser = new_parser(language)) == NULL) {
        fprintf(stderr, "*** ERROR: Could not allocate memory for parser engine\n");
        return NULL;
    }

    /* parse input a character at a time */
    while ((c = getc(fp)) != EOF) {
        if (!ISLEGAL(c)) {
            print_input(parser, stderr);
            fprintf(stderr, "\n\n*** ERROR: Illegal character 0x%x in input\n", c);
            return NULL;
        }

        switch (parser->state) {
            case OUTSIDE_MARKUP: /* not within any tag */
                if (c == '<') {
                    parser->state = OPENTAG_START;
                } else if (!ISSPACE(c)) {
                    print_input(parser, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Content found outside any containing tag\n");
                    return NULL;
                }
                break;

            case OPENTAG_START: /* initial < of open tag */
                if (c == '?') {
                    parser->state = OPENPI_START;
                } else if (c == '!') {
                    parser->state = OPENCOMMENT_BANG;
                } else if (ISNAMESTARTCHAR(c)) {
                    parser->state = OPENTAG_NAME;
                    parser->substr_start = parser->substr_end = parser->pos;
                } else {
                    print_input(parser, stderr);
                    /* printing the character in hex is not terribly user-friendly;
                     * a fuller implementation would translate newline, tab, etc.
                     * to user-friendly strings and print the character otherwise */
                    fprintf(stderr, "\n\n*** ERROR: Character 0x%x not allowed after '<'\n", c);
                    return NULL;
                }
                break;

            case OPENPI_START: /* <?... */
                if (c == '?') {
                    parser->state = CLOSEPI_START;
                }
                break;

            case CLOSEPI_START: /* <?...? */
                if (c == '>') {
                    parser->state = OUTSIDE_MARKUP;
                } else {
                    parser->state = OPENPI_START;
                }
                break;

            case OPENCOMMENT_BANG: /* <! */
                if (c == '-') {
                    parser->state = OPENCOMMENT_BANGDASH;
                } else {
                    /* this diverges from XML spec by disallowing non-comment <! elements (e.g. <!ENTITY ...>) */
                    print_input(parser, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid '<!' element\n");
                    return NULL;
                }
                break;

            case OPENCOMMENT_BANGDASH: /* <!- */
                if (c == '-') {
                    parser->state = COMMENT;
                } else {
                    print_input(parser, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Malformed comment\n");
                    return NULL;
                }
                break;

            case COMMENT: /* <!--... */
                if (c == '-') {
                    parser->state = CLOSECOMMENT_DASH;
                }
                break;

            case CLOSECOMMENT_DASH: /* <!--...- */
                if (c == '-') {
                    parser->state = CLOSECOMMENT_DASHDASH;
                } else {
                    parser->state = COMMENT;
                }
                break;

            case CLOSECOMMENT_DASHDASH: /* <!--...-- */
                if (c == '>') {
                    parser->state = OUTSIDE_MARKUP;
                } else {
                    /* per XML spec, -- is not allowed in comments */
                    print_input(parser, stderr);
                    fprintf(stderr, "\n\n*** ERROR: '--' not allowed within comment\n");
                    return NULL;
                }
                break;

            case OPENTAG_NAME: /* <T... (partial tag name) */
                if (c == '>') {
                    parser->state = CONTENT_START;
                    if (start_tag(parser) < 0) {
                        return NULL;
                    }
                } else if (ISNAMECHAR(c)) {
                    parser->substr_end = parser->pos;
                } else if (ISSPACE(c)) {
                    parser->state = OPENTAG_HAVE_NAME;
                    if (start_tag(parser) < 0) {
                        return NULL;
                    }
                } else {
                    print_input(parser, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character 0x%x in tag name\n", c);
                    return NULL;
                }
                break;

            case OPENTAG_HAVE_NAME: /* <TAG */
                if (start_tag(parser) < 0) {
                    return NULL;
                }
                if (c == '/') {
                    parser->state = OPENTAG_EMPTY;
                } else if (c == '>') {
                    parser->state = CONTENT_START;
                } else if (ISNAMESTARTCHAR(c)) {
                    parser->state = OPENTAG_ATTRNAME;
                    parser->substr_start = parser->substr_end = parser->pos;
                } else if (!ISSPACE(c)) {
                    print_input(parser, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character 0x%x in attribute name\n", c);
                    return NULL;
                }
                break;

            case OPENTAG_EMPTY: /* <TAG ... / */
                if (c == '>') {
                    if (!rum_element_get_is_empty(parser->element)) {
                        print_input(parser, stderr);
                        fprintf(stderr, "\n\n*** ERROR: %s cannot be an empty tag\n",
                            rum_element_get_name(parser->element));
                        return NULL;
                    }
                    parser->state = OUTSIDE_MARKUP;
                    parser->element = NULL;
                } else {
                    print_input(parser, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character 0x%x in attribute name\n", c);
                    return NULL;
                }
                break;

            case OPENTAG_ATTRNAME: /* <TAG ... A... (partial attribute name) */
                if (ISNAMECHAR(c)) {
                    parser->substr_end = parser->pos;
                } else if (ISSPACE(c)) {
                    parser->state = OPENTAG_HAVE_NAME;
                    /* @TODO add attribute with null value to element */
                    /* @TODO when adding attributes, verify that doesn't already exist (i.e. can only appear once per tag) */
                    parser->substr_start = parser->substr_end = 0;
                } else if (c == '=') {
                    parser->state = OPENTAG_ATTREQUALS;
                    /* @TODO add attribute with null value to element */
                    parser->substr_start = parser->substr_end = 0;
                } else if (c == '>') {
                    parser->state = CONTENT_START;
                    /* @TODO add attribute with null value to element */
                    parser->substr_start = parser->substr_end = 0;
                } else {
                    print_input(parser, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character 0x%x in attribute name\n", c);
                    return NULL;
                }
                break;

            case OPENTAG_ATTREQUALS: /* <TAG ... ATTR= */
                if ((c == '\'') || (c == '\"')) {
                    parser->state = OPENTAG_ATTRVALUE;
                    parser->quote_char = c;
                } else {
                    print_input(parser, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Attribute values must be quoted\n");
                    return NULL;
                }
                break;

            case OPENTAG_ATTRVALUE: /* <TAG ... ATTR=Q where Q is parser->quote_char */
                if (c == parser->quote_char) {
                    parser->state = OPENTAG_HAVEVALUE;
                    /* @TODO
                    validate value (any legal character except < and &, and entities)
                    add value to element attr
                    */
                } else {
                    if (!parser->substr_start) {
                        parser->substr_start = parser->pos;
                    }
                    parser->substr_end = parser->pos;
                }
                break;

            case OPENTAG_HAVEVALUE: /* <TAG ... ATTR=QVALUEQ where Q is parser->quote_char */
                if (c == '/') {
                    parser->state = OPENTAG_EMPTY;
                } else if (c == '>') {
                    parser->state = CONTENT_START;
                } else if (ISSPACE(c)) {
                    parser->state = OPENTAG_HAVE_NAME;
                } else {
                    print_input(parser, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid attribute value\n");
                    return NULL;
                }
                break;

            case CONTENT_START: /* <TAG ...> */
                if (rum_element_get_is_empty(parser->element)) {
                    print_input(parser, stderr);
                    fprintf(stderr, "\n\n*** ERROR: %s must be an empty tag\n", rum_element_get_name(parser->element));
                    return NULL;
                }
                /*
                    @TODO:
                    accept and ignore <?...?> and <!--...-->
                    accept and translate predefined entities
                    if </ encountered, validate close tag and start over
                    if < encountered, recurse
                    set element = NULL after close tag found
                 */
                break;

            default:
                fprintf(stderr, "\n\n*** Sorry, parser is not yet complete.\n");
                return NULL;
        }

        if (add_char(parser, c) < 0) {
            return NULL;
        }
    }

    /* @TODO: return error if all tags not closed */
    /* @TODO: separate document from parser so can free parser here */
    return parser->document;
}
