/*
    rump.c

    library to parse Rudimentary Markup 

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rump.h>
#include <rum_buffer.h>
#include "rump_private.h"

static rum_parser_t *
rum_parser_new(const rum_tag_t *language)
{
    rum_parser_t *parser;

    if ((language == NULL) || ((parser = malloc(sizeof(rum_parser_t))) == NULL)) {
        return NULL;
    }

    parser->language = language;
    parser->state = OUTSIDE_MARKUP;
    parser->quote_char = 0;
    parser->element = NULL;
    parser->parent = NULL;
    return(parser);
}

static rum_element_t *
start_tag(rum_parser_t *parser, rum_buffer_t *buffer, rum_element_t *document)
{
    char *tag_name;

    if (parser->element == NULL) {
        if ((tag_name = rum_buffer_clone_substr(buffer)) == NULL) {
            rum_buffer_print(buffer, stderr);
            fprintf(stderr, "\n\n*** ERROR: Could not allocate memory for tag name\n");
            return NULL;
        }
        if ((parser->element = rum_element_new(parser->parent, parser->language, tag_name)) == NULL) {
            rum_buffer_print(buffer, stderr);
            fprintf(stderr, "\n\n*** ERROR: Invalid tag '%s'\n", tag_name);
            free(tag_name);
            return NULL;
        }
        rum_buffer_reset_substr(buffer);
        parser->parent = parser->element;

        /* if this is the first tag encountered, it must be the root tag */
        if (document == NULL) {
            if (parser->element->tag != parser->language) {
                rum_buffer_print(buffer, stderr);
                fprintf(stderr, "\n\n*** ERROR: Tag '%s' found outside root tag\n", tag_name);
                return NULL;
            }
            document = parser->element;
        }
        free(tag_name);
    }
    return document;
}

static int
add_empty_value(rum_buffer_t *buffer, rum_element_t *element)
{
    char *attr_name;

    if ((attr_name = rum_buffer_clone_substr(buffer)) == NULL) {
        fprintf(stderr, "\n\n*** ERROR: Could not allocate memory for attribute name\n");
        return -1;
    }
    if (rum_element_set_value(element, attr_name, "") < 0) {
        rum_buffer_print(buffer, stderr);
        fprintf(stderr, "\n\n*** ERROR: Invalid attribute '%s'\n", attr_name);
        free(attr_name);
        return -1;
    }
    free(attr_name);
    rum_buffer_reset_substr(buffer);
    return 0;
}

rum_element_t *
rum_parse_file(FILE *fp, const rum_tag_t *language)
{
    int c;
    rum_parser_t *parser;
    rum_buffer_t *buffer;
    rum_element_t *document = NULL;
    char *attr_name = NULL, *attr_value = NULL;

    if ((parser = rum_parser_new(language)) == NULL) {
        fprintf(stderr, "*** ERROR: Could not allocate memory for parser engine\n");
        return NULL;
    }

    /* keep the already-processed XML in a buffer, for back references and error reporting */
    if ((buffer = rum_buffer_new()) == NULL) {
        fprintf(stderr, "*** ERROR: Could not allocate memory for input buffer\n");
        free(parser);
        return NULL;
    }

    /* parse input a character at a time */
    while ((c = getc(fp)) != EOF) {
        if (!ISLEGAL(c)) {
            rum_buffer_print(buffer, stderr);
            fprintf(stderr, "\n\n*** ERROR: Illegal character 0x%x in input\n", c);
            return NULL;
        }

        switch (parser->state) {
            case OUTSIDE_MARKUP: /* not within any tag */
                if (c == '<') {
                    parser->state = OPENTAG_START;
                } else if (!ISSPACE(c)) {
                    rum_buffer_print(buffer, stderr);
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
                    rum_buffer_track_substr(buffer);
                } else {
                    rum_buffer_print(buffer, stderr);
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
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid '<!' element\n");
                    return NULL;
                }
                break;

            case OPENCOMMENT_BANGDASH: /* <!- */
                if (c == '-') {
                    parser->state = COMMENT;
                } else {
                    rum_buffer_print(buffer, stderr);
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
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: '--' not allowed within comment\n");
                    return NULL;
                }
                break;

            case OPENTAG_NAME: /* <T... (partial tag name) */
                if (c == '>') {
                    parser->state = CONTENT_START;
                    if ((document = start_tag(parser, buffer, document)) == NULL) {
                        return NULL;
                    }
                } else if (ISNAMECHAR(c)) {
                    rum_buffer_track_substr(buffer);
                } else if (ISSPACE(c)) {
                    parser->state = OPENTAG_HAVE_NAME;
                    if ((document = start_tag(parser, buffer, document)) == NULL) {
                        return NULL;
                    }
                } else {
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character 0x%x in tag name\n", c);
                    return NULL;
                }
                break;

            case OPENTAG_HAVE_NAME: /* <TAG */
                if ((document = start_tag(parser, buffer, document)) == NULL) {
                    return NULL;
                }
                if (c == '/') {
                    parser->state = OPENTAG_EMPTY;
                } else if (c == '>') {
                    parser->state = CONTENT_START;
                } else if (ISNAMESTARTCHAR(c)) {
                    parser->state = OPENTAG_ATTRNAME;
                    rum_buffer_track_substr(buffer);
                } else if (!ISSPACE(c)) {
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character 0x%x in attribute name\n", c);
                    return NULL;
                }
                break;

            case OPENTAG_EMPTY: /* <TAG ... / */
                if (c == '>') {
                    if (!rum_element_get_is_empty(parser->element)) {
                        rum_buffer_print(buffer, stderr);
                        fprintf(stderr, "\n\n*** ERROR: %s cannot be an empty tag\n",
                            rum_element_get_name(parser->element));
                        return NULL;
                    }
                    parser->state = OUTSIDE_MARKUP;
                    parser->element = NULL;
                } else {
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character 0x%x in attribute name\n", c);
                    return NULL;
                }
                break;

            case OPENTAG_ATTRNAME: /* <TAG ... A... (partial attribute name) */
                if (ISNAMECHAR(c)) {
                    rum_buffer_track_substr(buffer);
                } else if (ISSPACE(c)) {
                    parser->state = OPENTAG_HAVE_NAME;
                    if (add_empty_value(buffer, parser->element) < 0) {
                        return NULL;
                    }
                } else if (c == '=') {
                    parser->state = OPENTAG_ATTREQUALS;
                    if ((attr_name = rum_buffer_clone_substr(buffer)) == NULL) {
                        fprintf(stderr, "\n\n*** ERROR: Could not allocate memory for attribute name\n");
                        return NULL;
                    }
                    rum_buffer_reset_substr(buffer);
                } else if (c == '>') {
                    parser->state = CONTENT_START;
                    if (add_empty_value(buffer, parser->element) < 0) {
                        return NULL;
                    }
                } else {
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character 0x%x in attribute name\n", c);
                    return NULL;
                }
                break;

            case OPENTAG_ATTREQUALS: /* <TAG ... ATTR= */
                if ((c == '\'') || (c == '\"')) {
                    parser->state = OPENTAG_ATTRVALUE;
                    parser->quote_char = c;
                } else {
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Attribute values must be quoted\n");
                    return NULL;
                }
                break;

            case OPENTAG_ATTRVALUE: /* <TAG ... ATTR=Q where Q is parser->quote_char */
                if (c == parser->quote_char) {
                    parser->state = OPENTAG_HAVEVALUE;
                    if ((attr_value = rum_buffer_clone_substr(buffer)) == NULL) {
                        fprintf(stderr, "\n\n*** ERROR: Could not allocate memory for attribute value\n");
                        return NULL;
                    }
                    if (rum_element_set_value(parser->element, attr_name, attr_value) < 0) {
                        rum_buffer_print(buffer, stderr);
                        fprintf(stderr, "\n\n*** ERROR: Invalid name or value for attribute '%s'\n", attr_name);
                        free(attr_name);
                        return NULL;
                    }
                    free(attr_name);
                    free(attr_value);
                    rum_buffer_reset_substr(buffer);
                } else {
                    rum_buffer_track_substr(buffer);
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
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid attribute value\n");
                    return NULL;
                }
                break;

            case CONTENT_START: /* <TAG ...> */
                if (rum_element_get_is_empty(parser->element)) {
                    rum_buffer_print(buffer, stderr);
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

        if (rum_buffer_add_char(buffer, c) < 0) {
            rum_buffer_print(buffer, stderr);
            fprintf(stderr, "\n\n*** ERROR: Could not reallocate input buffer\n");
            return NULL;
        }
    }

    /* @TODO: return error if all tags not closed */

    free(parser);
    rum_buffer_free(buffer);
    return document;
}
