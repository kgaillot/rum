/*
    rum_types.h

    object types for RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#ifndef RUM_TYPES__H
#define RUM_TYPES__H

typedef struct rum_tag_s rum_tag_t;
typedef struct rum_attr_s rum_attr_t;
typedef struct rum_element_s rum_element_t;
typedef int (*rum_tag_display_method_t)(const rum_element_t *element);

#endif /* RUM_TYPES__H */
