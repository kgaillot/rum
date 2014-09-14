/*
    rump.h

    header for RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

/*
    Declarations for language definition
*/

/* XML attribute: name="value" pair */
typedef struct rum_attr_s {
    /* this attribute's name */
    const char *name;

    /* whether this attribute is required to be present in its tag (boolean) */
    int is_required;
} rum_attr_t;

/* XML tag: <tag [attrs] /> if empty, <tag [attrs]> ... </tag> otherwise */
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

    /* list of attributes accepted by this tag (null attribute name indicates end of list) */
    rum_attr_t *attrs;
} rum_tag_t;

/* a language definition is simply a pointer to the root tag */

rum_tag_t *rum_tag_insert(rum_tag_t *parent, const char *name, int is_empty, rum_attr_t *attrs);
rum_tag_t *rum_tag_get_parent(const rum_tag_t *tag);
rum_tag_t *rum_tag_get_next_sibling(const rum_tag_t *tag);
rum_tag_t *rum_tag_get_first_child(const rum_tag_t *tag);
void rum_display_language(const rum_tag_t *root);

/*
    Declarations for language parsing
    (TBD)
*/

/* XML element: a tag instance and its associated attribute values and content */
typedef struct rum_element_s {
    /* this tag's place in the document's tag tree;
     * parent is NULL if this is the root tag, otherwise this tag is contained within parent */
    struct rum_element_s *parent;
    struct rum_element_s *next_sibling;
    struct rum_element_s *first_child;

    /* tag that this element is an instance of */
    const rum_tag_t *tag;

    /* a list of attribute name-value pairs; null name indicates end of list */
    char **attrs;

    /* this element's content (NULL for empty tags) */
    char *content;
} rum_element_t;
