/*
    rum_parser.h

    declarations for parser state engine portion of RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#ifndef RUM_PARSER__H
#define RUM_PARSER__H

#include <rum_types.h>

/*
 * macros for determining valid characters in various XML contexts
 *
 * do not pass an expression with side effects to these
 */

/* return true if c is legal as a character in an XML document */
#define RUM_PARSER_IS_LEGAL_CHAR(c) (((c) == 0x9) || ((c) == 0xA) || ((c) == 0xD) \
    || (((c) >= 0x20) && ((c) <= 0xD7FF)) \
    || (((c) >= 0xE000) && ((c) <= 0xFFFD)) \
    || (((c) >= 0x10000) && ((c) <= 0x10FFFF)))

/* return true if c is legal as the first character in an XML name */
#define RUM_PARSER_IS_LEGAL_FIRST_CHAR(c) (((c) == ':') || ((c) == '_') \
    || (((c) >= 'A') && ((c) <= 'Z')) \
    || (((c) >= 'a') && ((c) <= 'z')) \
    || (((c) >= 0xC0) && ((c) <= 0xD6)) \
    || (((c) >= 0xD8) && ((c) <= 0xF6)) \
    || (((c) >= 0xF8) && ((c) <= 0x2FF)) \
    || (((c) >= 0x370) && ((c) <= 0x37D)) \
    || (((c) >= 0x37F) && ((c) <= 0x1FFF)) \
    || (((c) >= 0x200C) && ((c) <= 0x200D)) \
    || (((c) >= 0x2070) && ((c) <= 0x218F)) \
    || (((c) >= 0x2C00) && ((c) <= 0x2FEF)) \
    || (((c) >= 0x3001) && ((c) <= 0xD7FF)) \
    || (((c) >= 0xF900) && ((c) <= 0xFDCF)) \
    || (((c) >= 0xFDF0) && ((c) <= 0xFFFD)) \
    || (((c) >= 0x10000) && ((c) <= 0xEFFFF)))

/* return true if c is legal as a character in an XML name token (i.e. name characters after the first) */
#define RUM_PARSER_IS_LEGAL_NAME_CHAR(c) (((c) == '-') || ((c) == '.') || (((c) >= '0') && ((c) <= '9')) \
    || ((c) == 0xB7) \
    || (((c) >= 0x0300) && ((c) <= 0x036F)) \
    || (((c) >= 0x203F) && ((c) <= 0x2040)) \
    || RUM_PARSER_IS_LEGAL_FIRST_CHAR(c))

/* return true if c is an XML whitespace character (more restrictive than C isspace()) */
#define RUM_PARSER_IS_SPACE(c) (((c) == 0x20) || ((c) == 0x9) || ((c) == 0xD) || ((c) == 0xA))

/*
 * other definitions
 */

/* enumerate the possible states of the parsing engine */
typedef enum {
    RUM_CONTENT,               /* not within a tag, e.g. the initial open state, or between tags */
    RUM_START_TAG,             /* "<" has been encountered */
    RUM_OPENTAG_NAME,          /* portion of the tag name in an open tag has been encountered */
    RUM_OPENTAG_SPACE,         /* entire tag name in an open tag has been encountered */
    RUM_OPENTAG_EMPTY,         /* "/" of the end of an empty tag has been encountered */
    RUM_OPENTAG_ATTRNAME,      /* portion of an attribute name in an open tag has been encountered */
    RUM_OPENTAG_ATTREQUALS,    /* "=" in an open tag attribute has been encountered */
    RUM_OPENTAG_ATTRVALUE,     /* portion of an attribute value in an open tag has been encountered */
    RUM_OPENTAG_HAVEVALUE,     /* complete attribute name/value pair in an open tag has been encountered */
    RUM_OPENPI,                /* "<?" of a processing instruction has been encountered */
    RUM_CLOSEPI,               /* "?" of the "?>" at the end of a processing instruction has been encountered */
    RUM_OPENCOMMENT_BANG,      /* "<!" of a comment has been encountered */
    RUM_OPENCOMMENT_BANGDASH,  /* "<!-" of a comment has been encountered */
    RUM_COMMENT,               /* portion of the body of a comment has been encountered */
    RUM_CLOSECOMMENT_DASH,     /* "-" of the "-->" at the end of a comment has been encountered */
    RUM_CLOSECOMMENT_DASHDASH, /* "--" of the "-->" at the end of a comment has been encountered */
    RUM_CLOSETAG_START,        /* "</" of close tag has been encountered */
    RUM_CLOSETAG_NAME          /* portion of the tag name in a close tag has been encountered */
} rum_state_t;

/* parser engine */
struct rum_parser_s {
    /* current state of engine */
    rum_state_t state;

    /* attribute values can use either single or double quotes, so remember which one */
    int quote_char;

    /* the most recently parsed attribute name */
    char *attr_name;

    /* the element currently being parsed */
    rum_element_t *element;

    /* the position of this parser state in the stack */
    rum_parser_t *prev;
    rum_parser_t *next;
};

/* return a string representation of a parser state */
char *rum_state_str(rum_state_t state);

/* stack constructor */
rum_parser_t *rum_parser_new();

/* stack push/pop */
int rum_parser_push(rum_parser_t **headp, rum_state_t state);
rum_element_t *rum_parser_pop(rum_parser_t **headp);

/* free any memory allocated for the last attribute name, and reset it to NULL */
void rum_parser_clear_attr_name(rum_parser_t *parser);

/* parse a character according to the current state */
rum_element_t *rum_parser_parse_char(rum_parser_t **headp, const rum_tag_t *language, rum_buffer_t *buffer, int c);

#endif /* RUM_PARSER__H */
