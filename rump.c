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
    parser->state = CONTENT;
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

static int
set_content(rum_buffer_t *buffer, rum_element_t *element)
{
    char *content;

    if ((content = rum_buffer_clone_substr(buffer)) == NULL) {
        fprintf(stderr, "\n\n*** ERROR: Could not allocate memory for tag content\n");
        return -1;
    }
    if (rum_element_set_content(element, content) < 0) {
        rum_buffer_print(buffer, stderr);
        fprintf(stderr, "\n\n*** ERROR: Invalid content\n");
        return -1;
    }
    free(content);
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
    /*@TODO current thingy.rum is not parsed correctly - content is marked as close tag */
    while ((c = getc(fp)) != EOF) {
        if (!ISLEGAL(c)) {
            rum_buffer_print(buffer, stderr);
            fprintf(stderr, "\n\n*** ERROR: Illegal character 0x%x in input\n", c);
            return NULL;
        }

        switch (parser->state) {
            case CONTENT: /* not within any tag */

                /* '<' ends this stretch of content and starts a new tag */
                if (c == '<') {
                    parser->state = START_TAG;

                    /* if we haven't already stored content for this element,
                     * then this is the first stretch of content, so store it */
                    if ((parser->element != NULL) && (rum_element_get_content(parser->element) == NULL)) {
                        if (set_content(buffer, parser->element) < 0) {
                            return NULL;
                        }
                    }

                /* otherwise we are continuing a stretch of content */
                } else {

                    /* if content is not contained by a tag, only spaces are valid */
                    if (parser->parent == NULL) {
                        if (!ISSPACE(c)) {
                            rum_buffer_print(buffer, stderr);
                            fprintf(stderr, "\n\n*** ERROR: Content found outside any containing tag\n");
                            return NULL;
                        }

                    /* if content is contained by a tag, we only track the first stretch
                     * (we ignore content after any nested tags) */
                    } else if (rum_element_get_content(parser->element) == NULL) {
                        rum_buffer_track_substr(buffer);
                    }
                }
                break;

            case START_TAG: /* < */
                if (c == '?') {
                    parser->state = OPENPI;
                } else if (c == '!') {
                    parser->state = OPENCOMMENT_BANG;
                } else if (c == '/') {
                    parser->state = CLOSETAG_START;
                    if (parser->element == NULL) {
                        rum_buffer_print(buffer, stderr);
                        fprintf(stderr, "\n\n*** ERROR: Close tag without open tag\n");
                        return NULL;
                    }
                } else if (ISNAMESTARTCHAR(c)) {
                    parser->state = OPENTAG_NAME;
                    rum_buffer_track_substr(buffer);
                } else {
                    /* printing the character in hex is not terribly user-friendly;
                     * a fuller implementation would translate newline, tab, etc.
                     * to user-friendly strings and print the character otherwise */
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Character 0x%x not allowed after '<'\n", c);
                    return NULL;
                }
                break;

            case OPENPI: /* <?... */
                if (c == '?') {
                    parser->state = CLOSEPI;
                }
                break;

            case CLOSEPI: /* <?...? */
                if (c == '>') {
                    parser->state = CONTENT;

                    /* if a PI interrupts content, and we haven't already stored content for this element,
                     * then this is the first stretch of content, so store it */
                    if ((parser->element != NULL) && (rum_element_get_content(parser->element) == NULL)) {
                        if (set_content(buffer, parser->element) < 0) {
                            return NULL;
                        }
                    }
                } else {
                    parser->state = OPENPI;
                }
                break;

            case OPENCOMMENT_BANG: /* <! */
                if (c == '-') {
                    parser->state = OPENCOMMENT_BANGDASH;
                } else {
                    /* RuM diverges from XML by disallowing non-comment <! elements (e.g. <!ENTITY ...>) */
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
                    parser->state = CONTENT;

                    /* if a comment interrupts content, and we haven't already stored content for this element,
                     * then this is the first stretch of content, so store it */
                    if ((parser->element != NULL) && (rum_element_get_content(parser->element) == NULL)) {
                        if (set_content(buffer, parser->element) < 0) {
                            return NULL;
                        }
                    }
                } else {
                    /* per XML spec, -- is not allowed in comments */
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: '--' not allowed within comment\n");
                    return NULL;
                }
                break;

            case CLOSETAG_START: /* </ */
                if (ISNAMESTARTCHAR(c)) {
                    parser->state = CLOSETAG_NAME;
                    rum_buffer_track_substr(buffer);
                } else {
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character 0x%x in close tag\n", c);
                    return NULL;
                }
                break;

            case CLOSETAG_NAME: /* </T... */
                if (c == '>') {
                    parser->state = CONTENT;
                    if (parser->element == NULL) {
                        rum_buffer_print(buffer, stderr);
                        fprintf(stderr, "\n\n*** ERROR: Close tag found without open tag\n");
                        return NULL;
                    }
                    if (rum_buffer_substrncmp(buffer,
                        rum_element_get_name(parser->element),
                        strlen(rum_element_get_name(parser->element)))) {
                            rum_buffer_print(buffer, stderr);
                            fprintf(stderr, "\n\n*** ERROR: Close tag does not match open tag '%s'\n",
                                rum_element_get_name(parser->element));
                            return NULL;
                    }
                    parser->element = NULL;
                } else if (ISNAMECHAR(c)) {
                    rum_buffer_track_substr(buffer);
                } else {
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character 0x%x in close tag\n", c);
                    return NULL;
                }
                break;

            case OPENTAG_NAME: /* <T... */
                if (ISNAMECHAR(c)) {
                    rum_buffer_track_substr(buffer);
                } else if (ISSPACE(c)) {
                    parser->state = OPENTAG_HAVE_NAME;
                    if ((document = start_tag(parser, buffer, document)) == NULL) {
                        return NULL;
                    }
                } else if (c == '>') {
                    parser->state = CONTENT;
                    if ((document = start_tag(parser, buffer, document)) == NULL) {
                        return NULL;
                    }
                    if (rum_element_get_is_empty(parser->element)) {
                        rum_buffer_print(buffer, stderr);
                        fprintf(stderr, "\n\n*** ERROR: %s must be an empty tag\n",
                            rum_element_get_name(parser->element));
                        return NULL;
                    }
                } else {
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character 0x%x in tag name\n", c);
                    return NULL;
                }
                break;

            case OPENTAG_HAVE_NAME: /* <TAG */
                if (c == '/') {
                    parser->state = OPENTAG_EMPTY;
                } else if (c == '>') {
                    parser->state = CONTENT;
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
                    parser->state = CONTENT;
                    if (!rum_element_get_is_empty(parser->element)) {
                        rum_buffer_print(buffer, stderr);
                        fprintf(stderr, "\n\n*** ERROR: %s cannot be an empty tag\n",
                            rum_element_get_name(parser->element));
                        return NULL;
                    }
                    parser->element = NULL;
                } else {
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character '/' in attribute name\n");
                    return NULL;
                }
                break;

            case OPENTAG_ATTRNAME: /* <TAG ... A... */
                if (ISNAMECHAR(c)) {
                    rum_buffer_track_substr(buffer);
                } else if (ISSPACE(c)) {
                    parser->state = OPENTAG_HAVE_NAME;
                    if (add_empty_value(buffer, parser->element) < 0) {
                        return NULL;
                    }
                } else if (c == '>') {
                    parser->state = CONTENT;
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
                        free(attr_name);
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
                    parser->state = CONTENT;
                    if (rum_element_get_is_empty(parser->element)) {
                        rum_buffer_print(buffer, stderr);
                        fprintf(stderr, "\n\n*** ERROR: %s must be an empty tag\n",
                            rum_element_get_name(parser->element));
                        return NULL;
                    }
                } else if (ISSPACE(c)) {
                    parser->state = OPENTAG_HAVE_NAME;
                } else {
                    rum_buffer_print(buffer, stderr);
                    fprintf(stderr, "\n\n*** ERROR: Invalid character 0x%x after end quote in attribute value\n", c);
                    return NULL;
                }
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
