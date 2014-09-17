/*
    rum_document.c

    document object functions for library to parse Rudimentary Markup 

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rump.h>

rum_element_t *
rum_element_new(rum_element_t *parent, const rum_tag_t *language, const char *tag_name)
{
    const rum_tag_t *tag;
    rum_element_t *element, *sibling;
    int i;

    /* validate tag name first so we can error out quickly if needed */
    if ((tag = rum_tag_get(language, tag_name)) == NULL) {
        return NULL;
    }

    /* allocate and initialize new element */
    if ((element = malloc(sizeof(rum_element_t))) == NULL) {
        /* a "real" library would distinguish malloc failure from tag not found */
        return NULL;
    }
    element->next_sibling = NULL;
    element->first_child = NULL;
    element->tag = tag;
    element->values = NULL;
    element->content = NULL;

    /* allocate a (NULL) value for each of the tag's attributes */
    if (rum_tag_get_nattrs(tag)) {
        if ((element->values = malloc(sizeof(char*) * rum_tag_get_nattrs(tag))) == NULL) {
            free(element);
            return NULL;
        }
        for (i = 0; i < rum_tag_get_nattrs(tag); ++i) {
            element->values[i] = NULL;
        }
    } else {
        element->values = NULL;
    }

    /* insert the element into the tree structure */
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
    return ((element == NULL) || (element->tag == NULL))? 0 : rum_tag_get_name(element->tag);
}

int
rum_element_get_is_empty(const rum_element_t *element)
{
    return ((element == NULL) || (element->tag == NULL))? 0 : rum_tag_get_is_empty(element->tag);
}

const char *
rum_element_get_content(const rum_element_t *element)
{
        return (element == NULL)? NULL : element->content;
}

/* clone XML content, replacing entity references and verifying well-formedness */
static char *
xmlcontent2plaintext(const char *content)
{
    const char *lookahead, *amp;
    char c, *translated, *cur;

    /* if no content, return empty string */
    if (content == NULL) {
        return "";
    }

    /* a production library would have an errno/errmsg facility for improved error messages */

    /* allocate space for the value
     *
     * if there are entity replacements, this will end up wasting some space
     */
    if ((translated = malloc(strlen(content) + 1)) == NULL) {
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
                return NULL;

            /* per XML spec, & is only allowed as part of entity reference */
            case '&':
                if (amp) {
                    free(translated);
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
    const rum_attr_t *attr;

    /* assert(element has been constructed) */
    if (element && element->tag && element->values && attr_name) {

        /* loop through the tag's attributes */
        for (i = 0; i < rum_tag_get_nattrs(element->tag); ++i) {

            /* assert(this index has a matching attribute) */
            if ((attr = rum_tag_get_attr_by_index(element->tag, i)) == NULL) {
                return -1;
            }

            /* if this is the attribute we're looking for ... */
            if (!strcmp(attr_name, attr->name)) {

                /* per XML spec, error if a value has already been set for this attribute in this tag */
                if (element->values[i] != NULL) {
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
    return -1;
}

int
rum_element_set_content(rum_element_t *element, const char *content)
{
        if (!element) {
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
