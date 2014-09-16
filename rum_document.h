/*
    rump.h

    header for language document portion of RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#ifndef RUM_DOCUMENT__H
#define RUM_DOCUMENT__H

#include <rum_language.h>

/* XML element: a tag instance and its associated attribute values and content */
typedef struct rum_element_s {
    /* this tag's place in the document's tag tree */
    struct rum_element_s *next_sibling;
    struct rum_element_s *first_child;

    /* tag that this element is an instance of */
    const rum_tag_t *tag;

    /* list of attribute values, one per attribute supported by the tag
     *
     * this assumes that the tag is immutable by the time this element is created
     * (adding or removing attributes in the tag would break this)
     *
     * NULL indicates the attribute was not specified;
     * empty string indicates the attribute was specified with no value
     * (allows enforcement of requirement that an XML attribute can only be specified once per tag)
     */
    char **values;

    /* this element's content (NULL for empty tags) */
    char *content;
} rum_element_t;

/* a document is simply a pointer to the root element */

/* constructor: create a new element instance and insert into document model */
rum_element_t *rum_element_new(rum_element_t *parent, const rum_tag_t *language, const char *tag_name);

/* destructor */
/* not implementing: would remove element from document model and free its allocated memory */
/*void rum_element_free(rum_element_t *parent, rum_element_t *element);*/

/* accessors */
const char *rum_element_get_name(const rum_element_t *element);
int rum_element_get_is_empty(const rum_element_t *element);

/* add a value to an attribute */
int rum_element_set_value(rum_element_t *element, const char *attr_name, const char *attr_value);

#endif /* RUM_DOCUMENT__H */
