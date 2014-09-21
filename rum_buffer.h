/*
    rum_buffer.h

    buffer handling for RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#ifndef RUM_BUFFER__H
#define RUM_BUFFER__H

#include <stddef.h>
#include <rum_types.h>

/* buffers will be allocated in chunks of this many bytes */
#define CHUNKSIZE (1024)

/* dynamically sized character buffer, with a current position and a current substring */
struct rum_buffer_s {
    char *buf;
    int nchunks;
    size_t pos;
    size_t substr_start;
    size_t substr_end;
};

/* constructor */
rum_buffer_t *rum_buffer_new();

/* destructor */
void rum_buffer_free(rum_buffer_t *buffer);

/* start or extend a substring of a buffer */
void rum_buffer_track_substr(rum_buffer_t *buffer);

/* stop tracking a substring of a buffer */
void rum_buffer_reset_substr(rum_buffer_t *buffer);

/* strncmp against a substring of a buffer */
int rum_buffer_substrncmp(rum_buffer_t *buffer, const char *str, size_t n);

/* return a newly allocated buffer with a copy of the current substring */
char *rum_buffer_clone_substr(rum_buffer_t * buffer);

/* add character to input buffer */
int rum_buffer_add_char(rum_buffer_t *buffer, int c);

/* print raw input parsed so far */
void rum_buffer_print(rum_buffer_t *buffer, FILE *fp);

#endif /* RUM_BUFFER__H */
