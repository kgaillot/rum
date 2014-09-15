/*
    rump.h

    header for language document portion of RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#ifndef RUM_DOCUMENT__H
#define RUM_DOCUMENT__H

/* XML element: a tag instance and its associated attribute values and content */
typedef struct rum_element_s {
    /* this tag's place in the document's tag tree */
    struct rum_element_s *next_sibling;
    struct rum_element_s *first_child;

    /* tag that this element is an instance of */
    const rum_tag_t *tag;

    /* a list of attribute name-value pairs; null name indicates end of list */
    char **attrs;

    /* this element's content (NULL for empty tags) */
    char *content;
} rum_element_t;

/* simple accessors */
const char *rum_element_get_name(const rum_element_t *element);
int rum_element_get_is_empty(const rum_element_t *element);

/* create a new element instance and insert into document */
rum_element_t *rum_element_insert(rum_element_t *parent, const rum_tag_t *language, const char *tag_name);

/* a document is simply a pointer to the root element */

#endif /* RUM_DOCUMENT__H */
