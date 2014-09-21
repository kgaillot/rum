/*
    rum_parser.c

    parser state functions for RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rump.h>
#include "rum_private.h"

#define DEBUG 0

char *
rum_state_str(rum_state_t state)
{
    switch (state) {
        case RUM_CONTENT:               return("RUM_CONTENT");
        case RUM_START_TAG:             return("RUM_START_TAG");
        case RUM_OPENTAG_NAME:          return("RUM_OPENTAG_NAME");
        case RUM_OPENTAG_SPACE:         return("RUM_OPENTAG_SPACE");
        case RUM_OPENTAG_EMPTY:         return("RUM_OPENTAG_EMPTY");
        case RUM_OPENTAG_ATTRNAME:      return("RUM_OPENTAG_ATTRNAME");
        case RUM_OPENTAG_ATTREQUALS:    return("RUM_OPENTAG_ATTREQUALS");
        case RUM_OPENTAG_ATTRVALUE:     return("RUM_OPENTAG_ATTRVALUE");
        case RUM_OPENTAG_HAVEVALUE:     return("RUM_OPENTAG_HAVEVALUE");
        case RUM_OPENPI:                return("RUM_OPENPI");
        case RUM_CLOSEPI:               return("RUM_CLOSEPI");
        case RUM_OPENCOMMENT_BANG:      return("RUM_OPENCOMMENT_BANG");
        case RUM_OPENCOMMENT_BANGDASH:  return("RUM_OPENCOMMENT_BANGDASH");
        case RUM_COMMENT:               return("RUM_COMMENT");
        case RUM_CLOSECOMMENT_DASH:     return("RUM_CLOSECOMMENT_DASH");
        case RUM_CLOSECOMMENT_DASHDASH: return("RUM_CLOSECOMMENT_DASHDASH");
        case RUM_CLOSETAG_START:        return("RUM_CLOSETAG_START");
        case RUM_CLOSETAG_NAME:         return("RUM_CLOSETAG_NAME");
    }
    return("(invalid state)");
}

rum_parser_t *
rum_parser_new()
{
    rum_parser_t *head = NULL;

    rum_set_error(NULL);
    return (rum_parser_push(&head, RUM_CONTENT) < 0)? NULL : head;
}

void
rum_parser_free(rum_parser_t **headp)
{
    rum_set_error(NULL);
    if (headp) {
        while (*headp) {
            rum_parser_pop(headp);
        }
    }
}

int
rum_parser_push(rum_parser_t **headp, rum_state_t state)
{
    rum_parser_t *parser;

    rum_set_error(NULL);
    if ((headp == NULL) || ((parser = malloc(sizeof(rum_parser_t))) == NULL)) {
        rum_set_error("Unable to allocate memory for parser state");
        return -1;
    }
    parser->state = state;
    parser->quote_char = 0;
    parser->attr_name = NULL;
    parser->element = NULL;
    parser->prev = *headp;
    parser->next = NULL;
    if (*headp) {
        (*headp)->next = parser;
    }
    *headp = parser;
    return 0;
}

rum_element_t *
rum_parser_pop(rum_parser_t **headp)
{
    rum_parser_t *old_head;
    rum_element_t *element;

    rum_set_error(NULL);

    /* assert(headp is top of stack) */
    if ((headp == NULL) || (*headp == NULL) || ((*headp)->next != NULL)) {
        rum_set_error("Programmer error: Unable to pop except at top of stack");
        return NULL;
    }

    old_head = *headp;
    element = old_head->element;
    *headp = old_head->prev;
    if (*headp) {
        (*headp)->next = NULL;
    }
    rum_parser_clear_attr_name(old_head);
    free(old_head);
    return element;
}

void
rum_parser_clear_attr_name(rum_parser_t *parser)
{
    rum_set_error(NULL);
    if (parser) {
        if (parser->attr_name) {
            free(parser->attr_name);
        }
        parser->attr_name = NULL;
    }
}

/* push a new parser state on the stack when a new element is encountered */
static int
start_element(rum_parser_t **headp, rum_state_t state, const rum_tag_t *language, rum_buffer_t *buffer)
{
    char *tag_name;
    rum_element_t *parent = (*headp)->element;

    rum_set_error(NULL);
    if ((tag_name = rum_buffer_clone_substr(buffer)) == NULL) {
        return -1;
    }
    rum_buffer_reset_substr(buffer);
    if (rum_parser_push(headp, state) < 0) {
        free(tag_name);
        return -1;
    }
    if (((*headp)->element = rum_element_new(parent, language, tag_name)) == NULL) {
        free(tag_name);
        return -1;
    }
    free(tag_name);
    return 0;
}

/* add an attribute to the element, using the current buffer substring as the name, with an empty value */
static int
add_empty_value(rum_element_t *element, rum_buffer_t *buffer)
{
    char *attr_name;

    rum_set_error(NULL);
    if ((attr_name = rum_buffer_clone_substr(buffer)) == NULL) {
        return -1;
    }
    if (rum_element_set_value(element, attr_name, "") < 0) {
        free(attr_name);
        return -1;
    }
    free(attr_name);
    rum_buffer_reset_substr(buffer);
    return 0;
}

/* if an element does not already have content, set its content to the current buffer substring */
static int
handle_content(rum_element_t *element, rum_buffer_t *buffer)
{
    char *content;

    rum_set_error(NULL);
    if ((element != NULL) && (rum_element_get_content(element) == NULL)) {
        if ((content = rum_buffer_clone_substr(buffer)) == NULL) {
            return -1;
        }
        if (rum_element_set_content(element, content) < 0) {
            free(content);
            return -1;
        }
        free(content);
        rum_buffer_reset_substr(buffer);
    }
    return 0;
}

/* handle errors: set error message, free memory, return NULL */
static rum_element_t *
rum_parser_error(rum_parser_t *parser, char *errmsg)
{
    /* set the error message last, because earlier calls will clear it */
    rum_parser_clear_attr_name(parser);
    rum_set_error(errmsg);
    return NULL;
}

static void
rum_parser_set_state(rum_parser_t *parser, rum_state_t state)
{
    const char *name;

    if (DEBUG) {
        name = parser? rum_element_get_name(parser->element) : NULL;
        printf("TAG %s %s -> %s\n", (name? name : "(none)"), rum_state_str(parser->state), rum_state_str(state));
        rum_set_error(NULL);
    }
    parser->state = state;
}

rum_element_t *
rum_parser_parse_char(rum_parser_t **headp, const rum_tag_t *language, rum_buffer_t *buffer, int c)
{
    char *attr_value;
    const char *tag_name;
    rum_element_t *element;

    rum_set_error(NULL);

    /* assert(this function was called properly) */
    if ((headp == NULL) || (*headp == NULL) || (language == NULL) || (buffer == NULL)) {
        return rum_parser_error(*headp, "Programmer error: Parser not configured properly");
    }

    if (!RUM_PARSER_IS_LEGAL_CHAR(c)) {
        return rum_parser_error(*headp, "Illegal character in input");
    }

    if (DEBUG) {
        printf("[%c] ", c);
        if (buffer->substr_start) {
            int i;
            printf("substr=[");
            for (i = buffer->substr_start; i <= buffer->substr_end; ++i) {
                putchar(buffer->buf[i]);
            }
            printf("]");
        }
        printf("\n");
    }

    element = (*headp)->element;

    switch ((*headp)->state) {
        case RUM_CONTENT: /* not within any tag */

            /* '<' ends this stretch of content and starts a new tag */
            if (c == '<') {
                rum_parser_set_state(*headp, RUM_START_TAG);
                if (handle_content((*headp)->element, buffer) < 0) {
                    return rum_parser_error(*headp, rum_last_error());
                }

            /* otherwise we are continuing a stretch of content */
            } else {

                /* if content is not contained by a tag, only spaces are valid */
                if ((*headp)->element == NULL) {
                    if (!RUM_PARSER_IS_SPACE(c)) {
                        return rum_parser_error(*headp, "Content found outside any containing tag");
                    }

                /* RuM diverges from the XML spec by only returning content to the application
                 * that occurs before any nested tags, so only track this stretch of content
                 * if the current element doesn't already have content set.
                 */
                } else if (rum_element_get_content((*headp)->element) == NULL) {
                    rum_buffer_track_substr(buffer);
                }
            }
            break;

        case RUM_START_TAG: /* < */
            if (c == '?') {
                rum_parser_set_state(*headp, RUM_OPENPI);
            } else if (c == '!') {
                rum_parser_set_state(*headp, RUM_OPENCOMMENT_BANG);
            } else if (c == '/') {
                rum_parser_set_state(*headp, RUM_CLOSETAG_START);
                if ((*headp)->element == NULL) {
                    return rum_parser_error(*headp, "Close tag without open tag");
                }
            } else if (RUM_PARSER_IS_LEGAL_FIRST_CHAR(c)) {
                rum_parser_set_state(*headp, RUM_OPENTAG_NAME);
                rum_buffer_track_substr(buffer);
            } else {
                return rum_parser_error(*headp, "Disallowed character after '<'");
            }
            break;

        case RUM_OPENPI: /* <?... */
            if (c == '?') {
                rum_parser_set_state(*headp, RUM_CLOSEPI);
            }
            break;

        case RUM_CLOSEPI: /* <?...? */
            if (c == '>') {
                rum_parser_set_state(*headp, RUM_CONTENT);
                if (handle_content((*headp)->element, buffer) < 0) {
                    return rum_parser_error(*headp, rum_last_error());
                }
            } else {
                rum_parser_set_state(*headp, RUM_OPENPI);
            }
            break;

        case RUM_OPENCOMMENT_BANG: /* <! */
            if (c == '-') {
                rum_parser_set_state(*headp, RUM_OPENCOMMENT_BANGDASH);
            } else {
                /* RuM diverges from XML by disallowing non-comment <! elements (e.g. <!ENTITY ...>) */
                return rum_parser_error(*headp, "Invalid '<!' element");
            }
            break;

        case RUM_OPENCOMMENT_BANGDASH: /* <!- */
            if (c == '-') {
                rum_parser_set_state(*headp, RUM_COMMENT);
            } else {
                return rum_parser_error(*headp, "Malformed comment");
            }
            break;

        case RUM_COMMENT: /* <!--... */
            if (c == '-') {
                rum_parser_set_state(*headp, RUM_CLOSECOMMENT_DASH);
            }
            break;

        case RUM_CLOSECOMMENT_DASH: /* <!--...- */
            if (c == '-') {
                rum_parser_set_state(*headp, RUM_CLOSECOMMENT_DASHDASH);
            } else {
                rum_parser_set_state(*headp, RUM_COMMENT);
            }
            break;

        case RUM_CLOSECOMMENT_DASHDASH: /* <!--...-- */
            if (c == '>') {
                rum_parser_set_state(*headp, RUM_CONTENT);
                if (handle_content((*headp)->element, buffer) < 0) {
                    return rum_parser_error(*headp, rum_last_error());
                }
            } else {
                /* per XML spec, -- is not allowed in comments */
                return rum_parser_error(*headp, "ERROR: '--' not allowed within comment");
            }
            break;

        case RUM_OPENTAG_NAME: /* <T... */
            if (RUM_PARSER_IS_LEGAL_NAME_CHAR(c)) {
                rum_buffer_track_substr(buffer);
            } else if (RUM_PARSER_IS_SPACE(c)) {
                /* this is the state to return to when the new state is popped */
                rum_parser_set_state(*headp, RUM_CONTENT);
                if (start_element(headp, RUM_OPENTAG_SPACE, language, buffer) < 0) {
                    return rum_parser_error(*headp, rum_last_error());
                }
                element = (*headp)->element;
            } else if (c == '>') {
                /* this is the state to return to when the new state is popped */
                rum_parser_set_state(*headp, RUM_CONTENT);
                if (start_element(headp, RUM_CONTENT, language, buffer) < 0) {
                    return rum_parser_error(*headp, rum_last_error());
                }
                element = (*headp)->element;
                if (rum_element_get_is_empty((*headp)->element)) {
                    return rum_parser_error(*headp, "Empty tag not closed with '/>'");
                }
            } else if (c == '/') {
                /* this is the state to return to when the new state is popped */
                rum_parser_set_state(*headp, RUM_CONTENT);
                if (start_element(headp, RUM_OPENTAG_EMPTY, language, buffer) < 0) {
                    return rum_parser_error(*headp, rum_last_error());
                }
                element = (*headp)->element;
            } else {
                return rum_parser_error(*headp, "Invalid character in tag name");
            }
            break;

        case RUM_OPENTAG_SPACE: /* <TAG ...\s */
            if (c == '/') {
                rum_parser_set_state(*headp, RUM_OPENTAG_EMPTY);
            } else if (c == '>') {
                rum_parser_set_state(*headp, RUM_CONTENT);
                if (rum_element_get_is_empty((*headp)->element)) {
                    return rum_parser_error(*headp, "Empty tag not closed with '/>'");
                }
            } else if (RUM_PARSER_IS_LEGAL_FIRST_CHAR(c)) {
                rum_parser_set_state(*headp, RUM_OPENTAG_ATTRNAME);
                rum_buffer_track_substr(buffer);
            } else if (!RUM_PARSER_IS_SPACE(c)) {
                return rum_parser_error(*headp, "Invalid character in attribute name");
            }
            break;

        case RUM_OPENTAG_EMPTY: /* <TAG ... / */
            if (c == '>') {
                if (!rum_element_get_is_empty((*headp)->element)) {
                    return rum_parser_error(*headp, "Nonempty tag closed with '/>'");
                }
                element = rum_parser_pop(headp);
                rum_buffer_reset_substr(buffer);
            } else {
                return rum_parser_error(*headp, "'/' not followed by '>' in open tag");
            }
            break;

        case RUM_OPENTAG_ATTRNAME: /* <TAG ... A... */
            if (RUM_PARSER_IS_LEGAL_NAME_CHAR(c)) {
                rum_buffer_track_substr(buffer);
            } else if (RUM_PARSER_IS_SPACE(c)) {
                rum_parser_set_state(*headp, RUM_OPENTAG_SPACE);
                if (add_empty_value((*headp)->element, buffer) < 0) {
                    return rum_parser_error(*headp, rum_last_error());
                }
            } else if (c == '>') {
                rum_parser_set_state(*headp, RUM_CONTENT);
                if (add_empty_value((*headp)->element, buffer) < 0) {
                    return rum_parser_error(*headp, rum_last_error());
                }
            } else if (c == '=') {
                rum_parser_set_state(*headp, RUM_OPENTAG_ATTREQUALS);
                if (((*headp)->attr_name = rum_buffer_clone_substr(buffer)) == NULL) {
                    return rum_parser_error(*headp, rum_last_error());
                }
                rum_buffer_reset_substr(buffer);
            } else {
                return rum_parser_error(*headp, "Invalid character in attribute name");
            }
            break;

        case RUM_OPENTAG_ATTREQUALS: /* <TAG ... ATTR= */
            if ((c == '\'') || (c == '\"')) {
                rum_parser_set_state(*headp, RUM_OPENTAG_ATTRVALUE);
                (*headp)->quote_char = c;
            } else {
                return rum_parser_error(*headp, "Attribute values must be quoted");
            }
            break;

        case RUM_OPENTAG_ATTRVALUE: /* <TAG ... ATTR=Q... where Q is (*headp)->quote_char */
            if (c != (*headp)->quote_char) {
                rum_buffer_track_substr(buffer);
            } else /* have end quote */ {
                rum_parser_set_state(*headp, RUM_OPENTAG_HAVEVALUE);
                if ((attr_value = rum_buffer_clone_substr(buffer)) == NULL) {
                    return rum_parser_error(*headp, rum_last_error());
                }
                if (rum_element_set_value((*headp)->element, (*headp)->attr_name, attr_value) < 0) {
                    return rum_parser_error(*headp, rum_last_error());
                }
                free(attr_value);
                rum_parser_clear_attr_name(*headp);
                rum_buffer_reset_substr(buffer);
            }
            break;

        case RUM_OPENTAG_HAVEVALUE: /* <TAG ... ATTR=QVALUEQ where Q is (*headp)->quote_char */
            if (c == '/') {
                rum_parser_set_state(*headp, RUM_OPENTAG_EMPTY);
            } else if (c == '>') {
                rum_parser_set_state(*headp, RUM_CONTENT);
                if (rum_element_get_is_empty((*headp)->element)) {
                    return rum_parser_error(*headp, "Empty tag not closed with '/>'");
                }
            } else if (RUM_PARSER_IS_SPACE(c)) {
                rum_parser_set_state(*headp, RUM_OPENTAG_SPACE);
            } else {
                return rum_parser_error(*headp, "Invalid character after end quote in attribute value");
            }
            (*headp)->quote_char = 0;
            break;

        case RUM_CLOSETAG_START: /* </ */
            if (RUM_PARSER_IS_LEGAL_FIRST_CHAR(c)) {
                rum_parser_set_state(*headp, RUM_CLOSETAG_NAME);
                rum_buffer_track_substr(buffer);
            } else {
                return rum_parser_error(*headp, "Invalid first character in close tag name");
            }
            break;

        case RUM_CLOSETAG_NAME: /* </T... */
            if (RUM_PARSER_IS_LEGAL_NAME_CHAR(c)) {
                rum_buffer_track_substr(buffer);
            } else if (c == '>') {
                if ((*headp)->element == NULL) {
                    return rum_parser_error(*headp, "Close tag found without open tag");
                }
                tag_name = rum_element_get_name((*headp)->element);
                if (rum_buffer_substrncmp(buffer, tag_name, strlen(tag_name))) {
                        return rum_parser_error(*headp, "Close tag does not match open tag");
                }
                element = rum_parser_pop(headp);
                rum_buffer_reset_substr(buffer);
            } else {
                return rum_parser_error(*headp, "Invalid character in close tag");
            }
            break;
    }
    return element;
}
