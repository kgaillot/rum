/*
    rum_document.c

    document object functions for RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rump.h>
#include "rum_private.h"

rum_element_t *
rum_element_new(rum_element_t *parent, const rum_tag_t *language, const char *tag_name)
{
    const rum_tag_t *tag;
    rum_element_t *element, *sibling;
    int i;

    rum_set_error(NULL);

    if ((language == NULL) || (tag_name == NULL)) {
        rum_set_error("Programmer error: Unable to create new element from nonexistent settings");
        return NULL;
    }

    /* if this is the root element, ensure that it is an instance of the root tag */
    if (parent == NULL) {
        if (strcmp(rum_tag_get_name(language), tag_name)) {
            rum_set_error("First tag must be root tag");
            return NULL;
        }
        tag = language;

    /* otherwise ensure that it is an instance of a child of the parent tag */
    } else if ((tag = rum_tag_get_child(parent->tag, tag_name)) == NULL) {
        return NULL;
    }

    /* allocate and initialize new element */
    if ((element = malloc(sizeof(rum_element_t))) == NULL) {
        rum_set_error("Unable to allocate memory for new document element");
        return NULL;
    }
    element->tag = tag;
    element->content = NULL;

    /* allocate a (NULL) value for each of the tag's attributes */
    if (rum_tag_get_nattrs(tag)) {
        if ((element->values = malloc(sizeof(char*) * rum_tag_get_nattrs(tag))) == NULL) {
            free(element);
            rum_set_error("Unable to allocate memory for new document element");
            return NULL;
        }
        for (i = 0; i < rum_tag_get_nattrs(tag); ++i) {
            element->values[i] = NULL;
        }
    } else {
        element->values = NULL;
    }

    /* insert the element into the tree structure */
    element->parent = parent;
    element->next_sibling = NULL;
    element->first_child = NULL;
    if (parent) {
        if (parent->first_child == NULL) {
            parent->first_child = element;
        } else {
            for (sibling = parent->first_child; sibling->next_sibling != NULL; sibling = sibling->next_sibling);
            sibling->next_sibling = element;
        }
    }
    return element;
}

const char *
rum_element_get_name(const rum_element_t *element)
{
    rum_set_error(NULL);
    if (element == NULL) {
        rum_set_error("Programmer error: Unable to get name of nonexistent document element");
        return NULL;
    }
    return rum_tag_get_name(element->tag);
}

int
rum_element_get_is_empty(const rum_element_t *element)
{
    rum_set_error(NULL);
    if (element == NULL) {
        rum_set_error("Programmer error: Unable to get settings of nonexistent document element");
        return 0;
    }
    return rum_tag_get_is_empty(element->tag);
}

const char *
rum_element_get_content(const rum_element_t *element)
{
    rum_set_error(NULL);
    if (element == NULL) {
        rum_set_error("Programmer error: Unable to get content of nonexistent document element");
        return NULL;
    }
    return element->content;
}

const char *
rum_element_get_value(const rum_element_t *element, const char *attr_name)
{
    int i;
    const rum_tag_t *tag;
    const char *attr_name2;

    rum_set_error(NULL);
    if ((element == NULL) || (attr_name == NULL)) {
        rum_set_error("Programmer error: Unable to get value of nonexistent attribute");
        return NULL;
    }
    tag = element->tag;
    for (i = 0; i < rum_tag_get_nattrs(tag); ++i) {
        if ((attr_name2 = rum_tag_get_attr_name(tag, i)) != NULL) {
            if (!strcmp(attr_name, attr_name2)) {
                return element->values[i];
            }
        }
    }
    rum_set_error("Programmer error: Unable to get value of unsupported attribute");
    return NULL;
}

rum_element_t *
rum_element_get_parent(const rum_element_t *element)
{
    rum_set_error(NULL);
    if (element == NULL) {
        rum_set_error("Programmer error: Unable to get parent of nonexistent document element");
        return NULL;
    }
    return element->parent;
}

rum_element_t *
rum_element_get_next_sibling(const rum_element_t *element)
{
    rum_set_error(NULL);
    if (element == NULL) {
        rum_set_error("Programmer error: Unable to get sibling of nonexistent document element");
        return NULL;
    }
    return element->next_sibling;
}

rum_element_t *
rum_element_get_first_child(const rum_element_t *element)
{
    rum_set_error(NULL);
    if (element == NULL) {
        rum_set_error("Programmer error: Unable to get child of nonexistent document element");
        return NULL;
    }
    return element->first_child;
}

/* clone XML content, replacing entity references and verifying well-formedness */
static char *
xmlcontent2plaintext(const char *content)
{
    const char *lookahead, *amp;
    char c, *translated, *cur;

    rum_set_error(NULL);

    /* if no content, return empty string */
    if (content == NULL) {
        return "";
    }

    /* allocate space for the value
     *
     * if there are entity replacements, this will end up wasting some space
     */
    if ((translated = malloc(strlen(content) + 1)) == NULL) {
        rum_set_error("Unable to allocate memory for parsed text");
        return NULL;
    }

    /* copy the value, replacing entities and ensuring well-formedness */
    lookahead = content;
    amp = NULL;
    cur = translated;
    while (*lookahead) {
        c = *lookahead;
        switch (c) {
            /* per XML spec, < is not allowed */
            case '<':
                free(translated);
                rum_set_error("'<' not allowed here");
                return NULL;

            /* per XML spec, & is only allowed as part of entity reference */
            case '&':
                if (amp) {
                    free(translated);
                    rum_set_error("'&' not allowed here");
                    return NULL;
                }
                amp = lookahead;
                break;

            /* if we have an entity in the source string, replace it */
            case ';':
                if (amp) {
                    /* RuM diverges from XML spec by not allowing numeric entity references */
                    if (!strncmp(amp, "&lt;", (lookahead - amp))) {
                        c = '<';
                    } else if (!strncmp(amp, "&gt;", (lookahead - amp))) {
                        c = '>';
                    } else if (!strncmp(amp, "&amp;", (lookahead - amp))) {
                        c = '&';
                    } else if (!strncmp(amp, "&apos;", (lookahead - amp))) {
                        c = '\'';
                    } else if (!strncmp(amp, "&quot;", (lookahead - amp))) {
                        c = '\"';
                    } else {
                        free(translated);
                        rum_set_error("Unknown entity");
                        return NULL;
                    }
                    amp = NULL;
                }
                break;
        }
        if (!amp) {
            *cur++ = c;
        }
        ++lookahead;
    }
    *cur = 0;

    /* per XML spec, & is only allowed as part of entity reference */
    if (amp) {
        free(translated);
        rum_set_error("'&' not allowed here");
        return NULL;
    }

    return translated;
}

/* set an attribute value for an element, verifying its well-formedness
 *
 * the current implementation has inefficient memory usage; rum_parse_file() clones the buffer substring
 * and passes that clone here, which clones it again to put in the element;
 * a "strn" approach would be better, passing the buffer substring start and length directly
 */
int
rum_element_set_value(rum_element_t *element, const char *attr_name, const char *attr_value)
{
    int i;
    const char *attr_name2;

    rum_set_error(NULL);

    /* assert(element has been constructed) */
    if (element && element->tag && element->values && attr_name) {

        /* loop through the tag's attributes */
        for (i = 0; i < rum_tag_get_nattrs(element->tag); ++i) {

            /* assert(this index has a matching attribute) */
            if ((attr_name2 = rum_tag_get_attr_name(element->tag, i)) == NULL) {
                return -1;
            }

            /* if this is the attribute we're looking for ... */
            if (!strcmp(attr_name, attr_name2)) {

                /* per XML spec, error if a value has already been set for this attribute in this tag */
                if (element->values[i] != NULL) {
                    rum_set_error("Attribute may not be specified twice in same element");
                    return -1;
                }

                /* clone the value as plain text */
                if ((element->values[i] = xmlcontent2plaintext(attr_value)) == NULL) {
                    return -1;
                }

                /* we have a winner */
                return(0);
            }
        }
    }

    /* error if attribute name is not valid for this tag */
    rum_set_error("Attribute not supported for this tag");
    return -1;
}

int
rum_element_set_content(rum_element_t *element, const char *content)
{
    rum_set_error(NULL);
    if (!element) {
        rum_set_error("Programmer error: Unable to set content for nonexistent element");
        return -1;
    }
    if (!content) {
        return 0;
    }
    if ((element->content = xmlcontent2plaintext(content)) == NULL) {
        return -1;
    }
    return 0;
}

void
rum_element_display(const rum_element_t *element)
{
    rum_set_error(NULL);
    if (element) {
        rum_tag_display_element(element->tag, element);
        if (element->first_child) {
            rum_element_display(element->first_child);
        }
        if (element->next_sibling) {
            rum_element_display(element->next_sibling);
        }
    }
}
