/*
	rump.c

	library to parse Rudimentary Markup 

	Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include "rump.h"

/*
	language definition functions

	A more sophisticated library would define the language via DTD or some
	such, but rump uses a function (rum_tag_insert()) to build the language
	description tag by tag.

	A more sophisticated library would also have better error handling and
	reporting.  Most rump functions simply return NULL on error with no
	explanation.
*/

rum_tag_t *
rum_tag_insert(rum_tag_t *parent, const char *name, int is_empty, rum_attr_t *attrs)
{
	rum_tag_t *tag;

	/* can't add a child tag to an empty tag */
	if (parent && parent->is_empty) {
		return NULL;
	}

	/* create a tag instance
	 * (in the interest of brevity, no special memory management is done,
	 * just a bunch of small mallocs) */
	if ((tag = malloc(sizeof(rum_tag_t))) == NULL) {
		return NULL;
	}
	tag->name = name;
	tag->parent = parent;
	tag->is_empty = is_empty;

	/* for simplicity, the attrs argument is required to point to static
	 * memory; a fuller implementation would malloc a copy here */
	tag->attrs = attrs;

	/* a fuller implementation could do further validation here,
	 * such as ensuring that a tag by this name is not already defined */

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

static void
rum_display_language_subtree(const rum_tag_t *root, int indent_level)
{
	const rum_tag_t *tag = root;
	const rum_attr_t *attr = NULL;

	while (tag != NULL) {
		printf("%*sTAG %s (%s)\n", (indent_level * 3), " ", tag->name,
			(tag->is_empty? "empty": "nonempty"));
		for (attr = tag->attrs; attr && attr->name; ++attr) {
			printf("%*sATTR %s (%s)\n", (indent_level * 3), " ", attr->name,
				(attr->is_required? "required" : "optional"));
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

/*
	language parsing functions
	(TBD)
*/
