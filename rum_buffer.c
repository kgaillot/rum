/*
    rum_buffer.c

    library to parse Rudimentary Markup 

    Copyright (c)2014 Ken Gaillot <kg@boogieonline.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rum_buffer.h>

rum_buffer_t *
rum_buffer_new()
{
    rum_buffer_t *buffer;

    if ((buffer = malloc(sizeof(rum_buffer_t))) == NULL) {
        return NULL;
    }
    if ((buffer->buf = malloc(CHUNKSIZE)) == NULL) {
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
    if (buffer) {
        buffer->substr_start = buffer->substr_end = 0;
    }
}

int
rum_buffer_substrncmp(rum_buffer_t *buffer, const char *str, size_t n)
{
    size_t len;

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
    char *str = NULL;
    size_t len;

    if (buffer && buffer->buf) {
        len = buffer->substr_end - buffer->substr_start + 1;
        if ((str = malloc(len + 1)) != NULL) {
            strncpy(str, buffer->buf + buffer->substr_start, len);
            str[len] = 0;
        }
    }
    return(str);
}

int
rum_buffer_add_char(rum_buffer_t *buffer, int c)
{
    char *newbuf;

    buffer->buf[(buffer->pos)++] = c;

    /* grow the buffer if needed (leaving room for a null byte) */
    if (buffer->pos == ((buffer->nchunks * CHUNKSIZE) - 1)) {
        ++(buffer->nchunks);
        if ((newbuf = realloc(buffer->buf, buffer->nchunks * CHUNKSIZE)) == NULL) {
            return -1;
        }
        buffer->buf = newbuf;
    }
    return 0;
}

void
rum_buffer_print(rum_buffer_t *buffer, FILE *fp)
{
    if (buffer && buffer->buf) {
        buffer->buf[buffer->pos] = 0;
        fputs(buffer->buf, fp);
    }
}
