/*
    rum_language.h

    header for language definition portion of RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#ifndef RUM_LANGUAGE__H
#define RUM_LANGUAGE__H

/* language definition of an XML attribute (name="value") */
typedef struct rum_attr_s {
    /* this attribute's name */
    const char *name;

    /* whether this attribute is required to be present in its tag (boolean) */
    int is_required;
} rum_attr_t;

/* language definition of an XML tag: <tag [attrs] /> if empty, <tag [attrs]> ... </tag> otherwise */
typedef struct rum_tag_s {
    /* this tag's place in the language's tag tree;
     * parent is NULL if this is the root tag, otherwise this tag is only valid within parent */
    struct rum_tag_s *parent;
    struct rum_tag_s *next_sibling;
    struct rum_tag_s *first_child;

    /* the tag itself */
    const char *name;

    /* whether this is an empty tag (boolean) */
    int is_empty;

    /* list of attributes accepted by this tag */
    int nattrs; /* pedantic: should be size_t ... */
    rum_attr_t *attrs;
} rum_tag_t;

/* a language definition is simply a pointer to the root tag */

/* constructor */
rum_tag_t *rum_tag_new(rum_tag_t *parent, const char *name, int is_empty, int nattrs, rum_attr_t *attrs);

/* destructor
 * not implementing; would remove tag from tree and free its allocated memory
 */
/*void rum_tag_free(rum_tag_t *ancestor, const char *name);*/

/* accessors */
rum_tag_t *rum_tag_get_parent(const rum_tag_t *tag);
rum_tag_t *rum_tag_get_next_sibling(const rum_tag_t *tag);
rum_tag_t *rum_tag_get_first_child(const rum_tag_t *tag);
const char *rum_tag_get_name(const rum_tag_t *tag);
int rum_tag_get_is_empty(const rum_tag_t *tag);
int rum_tag_get_nattrs(const rum_tag_t *tag);
const rum_attr_t *rum_tag_get_attr_by_index(const rum_tag_t *tag, int index);

/* return language tag corresponding to tag_name, or NULL if tag_name is invalid */
const rum_tag_t *rum_tag_get(const rum_tag_t *root, const char *tag_name);

/* print language in human-readable form */
void rum_display_language(const rum_tag_t *root);

#endif /* RUM_LANGUAGE__H */
