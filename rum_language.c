/*
    rum_language.c

    language definition functions for RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rump.h>
#include "rum_private.h"

rum_tag_t *
rum_tag_new(rum_tag_t *parent, const char *name, int is_empty, int nattrs, rum_attr_t *attrs,
    rum_tag_display_method_t display_method)
{
    rum_tag_t *tag;

    rum_set_error(NULL);

    /* can't add a child tag to an empty tag */
    if (parent && parent->is_empty) {
        rum_set_error("Programmer error: Empty tag may not contain nested tags");
        return NULL;
    }

    /* create a tag instance */
    if ((tag = malloc(sizeof(rum_tag_t))) == NULL) {
        rum_set_error("Unable to allocate memory for new tag specification");
        return NULL;
    }
    tag->name = name;
    tag->parent = parent;
    tag->is_empty = is_empty;
    tag->nattrs = nattrs;
    tag->display = display_method;

    /* copy the attribute information */
    if (nattrs && attrs) {
        if ((tag->attrs = malloc(sizeof(rum_attr_t) * nattrs)) == NULL) {
            free(tag);
            rum_set_error("Unable to allocate memory for new tag specification");
            return NULL;
        }
        memcpy(tag->attrs, attrs, sizeof(rum_attr_t) * nattrs);
    } else {
        tag->attrs = NULL;
    }

    /* add this tag to the tree */
    if (parent) {
        if (parent->first_child == NULL) {
            parent->first_child = tag;
        } else {
            rum_tag_t *child = parent->first_child;
            while (child->next_sibling != NULL)
                child = child->next_sibling;
            child->next_sibling = tag;
        }
    }
    return tag;
}

rum_tag_t *
rum_tag_get_parent(const rum_tag_t *tag)
{
    rum_set_error(NULL);
    if (tag == NULL) {
        rum_set_error("Programmer error: Unable to get parent of nonexistent tag specification");
        return NULL;
    }
    return tag->parent;
}

rum_tag_t *
rum_tag_get_next_sibling(const rum_tag_t *tag)
{
    rum_set_error(NULL);
    if (tag == NULL) {
        rum_set_error("Programmer error: Unable to get sibling of nonexistent tag specification");
        return NULL;
    }
    return tag->next_sibling;
}

rum_tag_t *
rum_tag_get_first_child(const rum_tag_t *tag)
{
    rum_set_error(NULL);
    if (tag == NULL) {
        rum_set_error("Programmer error: Unable to get child of nonexistent tag specification");
        return NULL;
    }
    return tag->first_child;
}

const char *
rum_tag_get_name(const rum_tag_t *tag)
{
    rum_set_error(NULL);
    if (tag == NULL) {
        rum_set_error("Programmer error: Unable to get name of nonexistent tag");
        return NULL;
    }
    return tag->name;
}

int
rum_tag_get_is_empty(const rum_tag_t *tag)
{
    rum_set_error(NULL);
    if (tag == NULL) {
        rum_set_error("Programmer error: Unable to get settings of nonexistent tag");
        return 0;
    }
    return tag->is_empty;
}

int
rum_tag_get_nattrs(const rum_tag_t *tag)
{
    rum_set_error(NULL);
    if (tag == NULL) {
        rum_set_error("Programmer error: Unable to get settings of nonexistent tag");
        return 0;
    }
    return tag->nattrs;
}

const char *
rum_tag_get_attr_name(const rum_tag_t *tag, int index)
{
    rum_set_error(NULL);
    if ((tag == NULL) || (index >= tag->nattrs)) {
        rum_set_error("Programmer error: Unable to get nonexistent attribute name for tag");
        return NULL;
    }
    return tag->attrs[index].name;
}

const rum_tag_t *
rum_tag_get_child(const rum_tag_t *root, const char *tag_name)
{
    const rum_tag_t *tag;

    rum_set_error(NULL);
    if (root && tag_name) {
        for (tag = root->first_child; tag; tag = tag->next_sibling) {
            if (!strcmp(tag->name, tag_name)) {
                return tag;
            }
        }
    }
    rum_set_error("Tag encountered that is not allowed here");
    return NULL;
}

static void
rum_display_language_subtree(const rum_tag_t *root, int indent_level)
{
    const rum_tag_t *tag = root;
    int i;

    rum_set_error(NULL);
    while (tag != NULL) {
        printf("%*sTAG %s (%s)\n", (indent_level * 3), " ", tag->name,
            (tag->is_empty? "empty": "nonempty"));
        for (i = 0; i < tag->nattrs; ++i) {
            printf("%*sATTR %s (%s)\n", (indent_level * 3), " ", tag->attrs[i].name,
                (tag->attrs[i].is_required? "required" : "optional"));
        }
        printf("\n");
        if (tag->first_child) {
            rum_display_language_subtree(tag->first_child, indent_level + 1);
        }
        tag = tag->next_sibling;
    }
}

void
rum_display_language(const rum_tag_t *root)
{
    rum_set_error(NULL);
    if (root == NULL) {
        printf("The language is undefined.\n\n");
    } else {
        printf("The language consists of these tags and attributes:\n\n");
        rum_display_language_subtree(root, 0);
    }
}

void
rum_tag_display_element(const rum_tag_t *tag, const rum_element_t *element)
{
    rum_set_error(NULL);
    tag->display(element);
}
