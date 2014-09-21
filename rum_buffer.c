/*
    rum_buffer.c

    buffer functions for RuM parser library

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rum_buffer.h>
#include "rum_private.h"

rum_buffer_t *
rum_buffer_new()
{
    rum_buffer_t *buffer;

    rum_set_error(NULL);
    if ((buffer = malloc(sizeof(rum_buffer_t))) == NULL) {
        rum_set_error("Unable to allocate memory for buffer");
        return NULL;
    }
    if ((buffer->buf = malloc(CHUNKSIZE)) == NULL) {
        rum_set_error("Unable to allocate memory for buffer");
        free(buffer);
        return NULL;
    }
    buffer->nchunks = 1;
    buffer->pos = 0;
    buffer->substr_start = 0;
    buffer->substr_end = 0;
    return buffer;
}

void
rum_buffer_free(rum_buffer_t *buffer)
{
    rum_set_error(NULL);
    if (buffer) {
        if (buffer->buf) {
            free(buffer->buf);
        }
        free(buffer);
    }
}

void
rum_buffer_track_substr(rum_buffer_t *buffer)
{
    rum_set_error(NULL);
    if (buffer) {
        if (!buffer->substr_start) {
            buffer->substr_start = buffer->pos;
        }
        buffer->substr_end = buffer->pos;
    }
}

void
rum_buffer_reset_substr(rum_buffer_t *buffer)
{
    rum_set_error(NULL);
    if (buffer) {
        buffer->substr_start = buffer->substr_end = 0;
    }
}

int
rum_buffer_substrncmp(rum_buffer_t *buffer, const char *str, size_t n)
{
    size_t len;

    rum_set_error(NULL);
    if (buffer == NULL) {
        return (str == NULL)? 0 : -1;
    }
    if (str == NULL) {
        return 1;
    }
    len = buffer->substr_end - buffer->substr_start + 1;
    return strncmp(str, buffer->buf + buffer->substr_start, (n < len)? n : len);
}

char*
rum_buffer_clone_substr(rum_buffer_t *buffer)
{
    char *str;
    size_t len;

    rum_set_error(NULL);
    if ((buffer == NULL) || (buffer->buf == NULL)) {
        rum_set_error("Programmer error: Unable to clone nonexistent buffer");
        return NULL;
    }

    /* special case: start and stop = 0 means empty string */
    if ((buffer->substr_start == 0) && (buffer->substr_end) == 0) {
        len = 0;
    } else {
        len = buffer->substr_end - buffer->substr_start + 1;
    }
    if ((str = malloc(len + 1)) == NULL) {
        rum_set_error("Programmer error: Unable to clone nonexistent buffer");
        return NULL;
    }
    if (buffer->substr_start && buffer->substr_end) {
        strncpy(str, buffer->buf + buffer->substr_start, len);
    }
    str[len] = 0;
    return(str);
}

int
rum_buffer_add_char(rum_buffer_t *buffer, int c)
{
    char *newbuf;

    rum_set_error(NULL);
    if ((buffer == NULL) || (buffer->buf == NULL)) {
        rum_set_error("Programmer error: Unable to add to nonexistent buffer");
        return -1;
    }

    buffer->buf[(buffer->pos)++] = c;

    /* grow the buffer if needed (leaving room for a null byte) */
    if (buffer->pos == ((buffer->nchunks * CHUNKSIZE) - 1)) {
        ++(buffer->nchunks);
        if ((newbuf = realloc(buffer->buf, buffer->nchunks * CHUNKSIZE)) == NULL) {
            rum_set_error("Unable to allocate memory to extend buffer");
            return -1;
        }
        buffer->buf = newbuf;
    }
    return 0;
}

void
rum_buffer_print(rum_buffer_t *buffer, FILE *fp)
{
    rum_set_error(NULL);
    if (buffer && buffer->buf) {
        buffer->buf[buffer->pos] = 0;
        fputs(buffer->buf, fp);
    }
}
