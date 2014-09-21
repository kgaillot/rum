/*
    rum_document.h

    header for language document portion of RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#ifndef RUM_DOCUMENT__H
#define RUM_DOCUMENT__H

#include <rum_types.h>

/* XML element: a tag instance and its associated attribute values and content */
struct rum_element_s {
    /* tag that this element is an instance of */
    const rum_tag_t *tag;

    /* list of attribute values, one per attribute supported by the tag
     *
     * this assumes that the tag spec is immutable by the time this element is created
     * (adding or removing attributes in the tag spec would break this)
     *
     * NULL indicates the attribute was not specified;
     * empty string indicates the attribute was specified with no value
     * (allows enforcement of requirement that an XML attribute can only be specified once per tag)
     */
    char **values;

    /* this element's content (NULL for empty tags) */
    char *content;

    /* this tag's place in the document's tag tree */
    rum_element_t *parent;
    rum_element_t *next_sibling;
    rum_element_t *first_child;
};

/* a document is simply a pointer to the root element */

/* constructor: create a new element instance and insert into document model */
rum_element_t *rum_element_new(rum_element_t *parent, const rum_tag_t *language, const char *tag_name);

/* accessors */
const char *rum_element_get_name(const rum_element_t *element);
int rum_element_get_is_empty(const rum_element_t *element);
const char *rum_element_get_content(const rum_element_t *element);
const char *rum_element_get_value(const rum_element_t *element, const char *attr_name);
rum_element_t *rum_element_get_parent(const rum_element_t *element);
rum_element_t *rum_element_get_next_sibling(const rum_element_t *element);
rum_element_t *rum_element_get_first_child(const rum_element_t *element);

/* add a value to an attribute of the element */
int rum_element_set_value(rum_element_t *element, const char *attr_name, const char *attr_value);

/* add content to the element */
int rum_element_set_content(rum_element_t *element, const char *content);

/* display this element and its siblings and children
 *
 * each element's display method is called in sequence, starting with this element itself,
 * then all its children, then all its siblings (each displayed in the same manner)
 */
void rum_element_display(const rum_element_t *element);

#endif /* RUM_DOCUMENT__H */
