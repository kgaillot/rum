/*
    rum_document.c

    document object functions for library to parse Rudimentary Markup 

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <rump.h>

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

rum_element_t *
rum_element_insert(rum_element_t *parent, const rum_tag_t *language, const char *tag_name)
{
    const rum_tag_t *tag;
    rum_element_t *element, *sibling;

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
    element->attrs = NULL;
    element->content = NULL;

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
