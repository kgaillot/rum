/*
    rump_private.h

    internal definitions for library to parse Rudimentary Markup

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#ifndef RUM_RUMP_PRIVATE__H
#define RUM_RUMP_PRIVATE__H

#include <rump.h>

/*
 * macros for determining valid characters in various XML contexts
 *
 * do not pass an expression with side effects to these
 */

/* return true if c is legal as a character in an XML document */
#define ISLEGAL(c) (((c) == 0x9) || ((c) == 0xA) || ((c) == 0xD) \
    || (((c) >= 0x20) && ((c) <= 0xD7FF)) \
    || (((c) >= 0xE000) && ((c) <= 0xFFFD)) \
    || (((c) >= 0x10000) && ((c) <= 0x10FFFF)))

/* return true if c is legal as the first character in an XML name */
#define ISNAMESTARTCHAR(c) (((c) == ':') || ((c) == '_') \
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
#define ISNAMECHAR(c) (((c) == '-') || ((c) == '.') || (((c) >= '0') && ((c) <= '9')) || ((c) == 0xB7) \
    || (((c) >= 0x0300) && ((c) <= 0x036F)) \
    || (((c) >= 0x203F) && ((c) <= 0x2040)) \
    || ISNAMESTARTCHAR(c))

/* return true if c is an XML whitespace character (more restrictive than C isspace()) */
#define ISSPACE(c) (((c) == 0x20) || ((c) == 0x9) || ((c) == 0xD) || ((c) == 0xA))

/*
 * other definitions
 */

/* the raw XML input will be stored in an input buffer, allocated in chunks of this many bytes */
#define CHUNKSIZE (1024)

/* enumerate the possible states of the parsing engine */
typedef enum {
    OUTSIDE_MARKUP,        /* not within a tag, e.g. the initial open state */
    OPENTAG_START,         /* first "<" of an open tag has been encountered */
    OPENTAG_NAME,          /* portion of the tag name in an open tag has been encountered */
    OPENTAG_HAVE_NAME,     /* entire tag name in an open tag has been encountered */
    OPENTAG_EMPTY,         /* "/" of the end of an empty tag has been encountered */
    OPENTAG_ATTRNAME,      /* portion of an attribute name in an open tag has been encountered */
    OPENTAG_ATTREQUALS,    /* "=" in an open tag attribute has been encountered */
    OPENTAG_ATTRVALUE,     /* portion of an attribute value in an open tag has been encountered */
    OPENTAG_HAVEVALUE,     /* complete attribute name/value pair in an open tag has been encountered */
    OPENPI_START,          /* "<?" of a processing instruction has been encountered */
    CLOSEPI_START,         /* "?" of the "?>" at the end of a processing instruction has been encountered */
    OPENCOMMENT_BANG,      /* "<!" of a comment has been encountered */
    OPENCOMMENT_BANGDASH,  /* "<!-" of a comment has been encountered */
    COMMENT,               /* portion of the body of a comment has been encountered */
    CLOSECOMMENT_DASH,     /* "-" of the "-->" at the end of a comment has been encountered */
    CLOSECOMMENT_DASHDASH, /* "--" of the "-->" at the end of a comment has been encountered */
    CONTENT_START,         /* entire open tag has been encountered */
} rum_state_t;

/* parser engine */
typedef struct rum_parser_s {
    /* the language this parser parses */
    const rum_tag_t *language;

    /* current state of engine */
    rum_state_t state;

    /* keep the already-processed XML in a buffer, for back references and error reporting */
    char *buf;
    int nchunks;
    size_t pos;

    /* tag name, attribute name or attribute value currently being parsed, if any */
    size_t substr_start;
    size_t substr_end;

    /* attribute values can use either single or double quotes, so remember which one */
    int quote_char;

    /* the parsed document object (= pointer to root element) */
    rum_element_t *document;

    /* the element currently being parsed */
    rum_element_t *element;

    /* the parent element of the input currently being parsed */
    rum_element_t *parent;
} rum_parser_t;

/*
 * parser functions
 */

/* given an XML entity, return the corresponding plain character */
static char entity2char(char *entity);

/* copy a substring into a newly allocated buffer */
static char *copy_substring(const char *s, size_t start, size_t end);

/* constructor */
static rum_parser_t *new_parser(const rum_tag_t *language);

/* destructor */
static void free_parser(rum_parser_t *parser);

/* add character to input buffer */
static int add_char(rum_parser_t *parser, int c);

/* print raw input parsed so far */
static void print_input(rum_parser_t *parser, FILE *fp);

/* update parser once an element's tag name is known */
static int start_tag(rum_parser_t *parser);

#endif /* RUM_RUMP_PRIVATE__H */
