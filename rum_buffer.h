/*
    rum_buffer.h

    buffer handling for library to parse Rudimentary Markup

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#ifndef RUM_BUFFER__H
#define RUM_BUFFER__H

/* buffers will be allocated in chunks of this many bytes */
#define CHUNKSIZE (1024)

/* dynamically sized character buffer, with a current position and a current substring */
typedef struct rum_buffer_s {
    /* members */
    char *buf;
    int nchunks;
    size_t pos;
    size_t substr_start;
    size_t substr_end;
} rum_buffer_t;

/* constructor */
rum_buffer_t *rum_buffer_new();

/* destructor */
void rum_buffer_free(rum_buffer_t *buffer);

/* start or extend a substring of a buffer */
void rum_buffer_track_substr(rum_buffer_t *buffer);

/* stop tracking a substring of a buffer */
void rum_buffer_reset_substr(rum_buffer_t *buffer);

/* return a newly allocated buffer with a copy of the current substring */
char *rum_buffer_clone_substr(rum_buffer_t * buffer);

/* add character to input buffer */
int rum_buffer_add_char(rum_buffer_t *buffer, int c);

/* print raw input parsed so far */
void rum_buffer_print(rum_buffer_t *buffer, FILE *fp);

#endif /* RUM_BUFFER__H */