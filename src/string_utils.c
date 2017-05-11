#ifndef STRING_UTILS_DEF
#define STRING_UTILS_DEF

/*
 File:          string_utils.c
 Description:   Various utility functions defs for dealing with strings.
 Created:       May 5, 2017
 Author:        Matt Mumau
 */

/* System includes */
#include <string.h>
#include <stdbool.h>

#include "string_utils.h"

bool str_starts(char *string_a, char *string_b)
{
    if (strncmp(string_a, string_b, strlen(string_b)) == 0) 
        return true;
    return false;
}

bool str_equals(char *string_a, char *string_b)
{
    if (strcmp(string_a, string_b) == 0)
        return true;

    return false;
}

void str_removenl(char *string)
{
    char *nl = strchr(string, '\n');
    if (nl != NULL)
        *nl = '\0';
}

void str_copy(char *dest, char *src)
{
    while (*src) {
        *dest = *src;
        src++;
        dest++;
    }
    *dest = '\0';
}

#endif