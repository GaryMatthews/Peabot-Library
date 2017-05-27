#ifndef HTTP_REQUEST_DEF
#define HTTP_REQUEST_DEF

/*
 File:          http_request.h
 Description:   Object for parsing HTTP requests.
 Created:       May 23, 2017
 Author:        Matt Mumau
 */

/* System headers */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* Application headers */
#include "http_server.h"
#include "string_utils.h"

/* Header */
#include "http_request.h"

/* Forward decs */
static void http_request_handle_lines(HTTPRequest *http_request, HTTPRequestLine *lines, unsigned int size);
static void http_request_handle_request_line(HTTPRequest *http_request, HTTPRequestLine *line);
static int http_request_max_body_len();
static int http_request_copy_buffer(char *dest, char *src, size_t size);
static int http_request_copy_body(char *dest, char *src, size_t size);

void http_request_parse(HTTPRequest *http_request, char *raw, int buff_size)
{
    if (http_request == NULL || raw == NULL)
        return;

    int max_body_len = http_request_max_body_len();

    char buffer_cpy[DEFAULT_HTTP_MAX_BUFFER];
    char body_str[max_body_len];

    int buffer_cpy_len = http_request_copy_buffer(buffer_cpy, raw, sizeof(buffer_cpy));
    printf("\n[buffer_cpy:%d]\n%s\n", buffer_cpy_len, buffer_cpy);

    int body_len = http_request_copy_body(body_str, buffer_cpy, max_body_len);
    printf("\n[body:%d]\n%s\n", body_len, body_str);

    // unsigned int i;
    // HTTPRequestLine lines[max_lines];
    // bool add_extra_line = (DEFAULT_HTTP_MAX_BUFFER % DEFAULT_HTTP_LINE_LEN) > 0;
    // int max_lines = (DEFAULT_HTTP_MAX_BUFFER - (DEFAULT_HTTP_MAX_BUFFER % DEFAULT_HTTP_LINE_LEN)) / DEFAULT_HTTP_LINE_LEN;
    
    // if (add_extra_line)
    //     max_lines++;    

    // char *body = NULL;

    // if (body_count > 4)
    // {
    //     body = &(body_str[4]);
    //     printf("[body:%d]\n%s\n", body_count, body);
    // }
    
    // int header_len = buffer_cpy_size - body_count;
    // char header_str[header_len];
    // memset(header_str, '\0', header_len);
    // memcpy(header_str, &(buffer_cpy[0]), header_len);
    // header_str[header_len - 1] = '\0';

    // printf("[header:%d]\n%s\n", header_len, header_str);

    //char *header_lines_p = strtok(buffer_cpy, body_delim);
    // HTTPRequestLine *next_line;
    // for (i = 0; i < max_lines; i++)
    // {
    //     if (line_cursor == NULL)
    //         break;

    //     next_line = &(lines[i]);

    //     memset(next_line, '\0', DEFAULT_HTTP_LINE_LEN);
    //     memcpy(next_line, line_cursor, (size_t) DEFAULT_HTTP_LINE_LEN - 1);

    //     line_cursor = strtok(NULL, delim);
    // }

    // http_request->total_lines = i + 1;

    // http_request_handle_lines(http_request, lines, http_request->total_lines);
}

static void http_request_handle_lines(HTTPRequest *http_request, HTTPRequestLine *lines, unsigned int size)
{
    if (lines == NULL || size <= 0)
        return;

    http_request_handle_request_line(http_request, &(lines[0]));

    if (size == 1)
        return;

    for (int i = 1; i < size; i++)
    {
        // todo: handle headers as neccessary
        printf("[H#%d]: %s\n", i, lines[i]);
    }
}

static void http_request_handle_request_line(HTTPRequest *http_request, HTTPRequestLine *line)
{
    if (line == NULL)
        return;

    char line_cpy[DEFAULT_HTTP_LINE_LEN];
    memset(line_cpy, '\0', DEFAULT_HTTP_LINE_LEN);
    memcpy(line_cpy, line, DEFAULT_HTTP_LINE_LEN - 1);

    char *line_cursor;
    char delim[] = " ";
    
    http_request->method = HTTP_BADREQUEST;
    line_cursor = strtok((char *) line, delim);
    if (line_cursor == NULL)
        return;
    if (strcmp(line_cursor, "POST") == 0)
        http_request->method = HTTP_POST;    
    if (strcmp(line_cursor, "GET") == 0)
        http_request->method = HTTP_GET;
    if (strcmp(line_cursor, "PUT") == 0)
        http_request->method = HTTP_PUT;
    if (strcmp(line_cursor, "DELETE") == 0)
        http_request->method = HTTP_DELETE;

    memset(http_request->uri, '\0', DEFAULT_HTTP_URI_LEN);
    line_cursor = strtok(NULL, delim);
    if (line_cursor == NULL)
        return;
    memcpy(http_request->uri, line_cursor, DEFAULT_HTTP_URI_LEN);

    http_request->v11 = false;
    line_cursor = strtok(NULL, delim);
    if (line_cursor == NULL)
        return;

    if (strcmp(line_cursor, "HTTP/1.1") == 0)
        http_request->v11 = true;
}

static int http_request_copy_buffer(char *dest, char *src, size_t size)
{
    memset(dest, '\0', size);
    memcpy(dest, src, size - 1);
    buffer_cpy[size - 1] = '\0';     

    return strlen(dest);
}

static int http_request_max_body_len()
{
    return DEFAULT_HTTP_MAX_BUFFER - ((DEFAULT_HTTP_MAX_HEADERS + 1) * DEFAULT_HTTP_LINE_LEN);
}

static int http_request_copy_body(char *dest, char *src, size_t size)
{
    char copy[size];
    memset(copy, '\0', size);
    memcpy(copy, src, size - 1);
    copy[size - 1] = '\0';

    char *body_substr = strstr(copy, "\r\n\r\n");

    if (body_substr == NULL)
        return -1;

    memset(dest, '\0', size);
    memcpy(dest, &(copy[4]), size);
    dest[size - 1] = '\0';    

    return strlen(dest);
}

#endif