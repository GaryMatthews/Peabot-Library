#ifndef LIST_H_DEF
#define LIST_H_DEF

/*
 File:          list.h
 Description:   Linked list library definition
 Created:       March 21, 2017
 Author:        Matt Mumau
 */

typedef struct List
{
    void *data;
    struct List *next;
} List;

/*
 Push data to the end of the list.
 */
void list_push(List **head, void *data);

/*
 Remove the first element of the list and return its data.
 */
void *list_pop(List **head);

/*
 Insert the data object to the beginning of the list.
 */
void list_insert(List **head, void *data);

/*
 Get the size of the list.
 */
unsigned int list_sizeof(List *head);

/*
 Get the data of the node at the given ordinal number in the list.
 */
List *list_get(List *head, unsigned int index);

/*
 Delete the item in the list at the given ordinal location (1 is the first).
 */
void list_remove(List **head, unsigned int index);

/*
 Get the data of the last item in the list.
 */
void *list_last(List *head)

#endif