/*
    rum_language.c

    language definition functions for library to parse Rudimentary Markup 

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rump.h>

rum_tag_t *
rum_tag_new(rum_tag_t *parent, const char *name, int is_empty, int nattrs, rum_attr_t *attrs,
    rum_tag_display_method_t display_method)
{
    rum_tag_t *tag;

    /* can't add a child tag to an empty tag */
    if (parent && parent->is_empty) {
        return NULL;
    }

    /* create a tag instance */
    if ((tag = malloc(sizeof(rum_tag_t))) == NULL) {
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
    return tag? tag->parent : NULL;
}

rum_tag_t *
rum_tag_get_next_sibling(const rum_tag_t *tag)
{
    return tag? tag->next_sibling : NULL;
}

rum_tag_t *
rum_tag_get_first_child(const rum_tag_t *tag)
{
    return tag? tag->first_child : NULL;
}

const char *
rum_tag_get_name(const rum_tag_t *tag)
{
    return tag? tag->name : NULL;
}

int
rum_tag_get_is_empty(const rum_tag_t *tag)
{
    /* no good default if tag is NULL, just pick one */
    return tag? tag->is_empty : 0;
}

int
rum_tag_get_nattrs(const rum_tag_t *tag)
{
    return tag? tag->nattrs : 0;
}

const char *
rum_tag_get_attr_name(const rum_tag_t *tag, int index)
{
        return tag && (index < tag->nattrs)? tag->attrs[index].name : NULL;
}

const rum_tag_t *
rum_tag_get(const rum_tag_t *root, const char *tag_name)
{
    const rum_tag_t *tag;

    if ((root == NULL) || !strcmp(root->name, tag_name)) {
        return root;
    }
    if ((root->first_child != NULL)
    && ((tag = rum_tag_get(root->first_child, tag_name)) != NULL)) {
        return tag;
    }
    if ((root->next_sibling != NULL)
    && ((tag = rum_tag_get(root->next_sibling, tag_name)) != NULL)) {
        return tag;
    }
    return NULL;
}

static void
rum_display_language_subtree(const rum_tag_t *root, int indent_level)
{
    const rum_tag_t *tag = root;
    int i;

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
    if (root == NULL) {
        printf("The language is undefined.\n\n");
    } else {
        printf("The language consists of these tags and attributes:\n\n");
        rum_display_language_subtree(root, 0);
    }
}

int
rum_tag_display_element(const rum_tag_t *tag, const rum_element_t *element)
{
    return(tag->display(element));
}
