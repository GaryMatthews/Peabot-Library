#ifndef EVENTS_DEF
#define EVENTS_DEF

/*
 File:          events.c
 Description:   Implementations for handling an event loop.
 Created:       May 8, 2017
 Author:        Matt Mumau
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

/* Application includes */
#include "main.h"
#include "config.h"
#include "log.h"
#include "list.h"
#include "event_callbacks.h"

/* Header */
#include "events.h"

static pthread_t event_thread;
static bool running = true;
static List *events;

/* Forward decs */
static void *event_main(void *arg);
static char *event_getname(int event_type);

void event_init()
{
    int error = pthread_create(&event_thread, NULL, event_main, NULL);
    if (error)
        app_exit("[ERROR!] Could not create event thread.", 1);
}

void event_halt()
{
    running = false;
    int error = pthread_join(event_thread, NULL);
    if (error)
        log_event("[ERROR!] Could not rejoin from robot thread.");
}

static void *event_main(void *arg)
{
    Event *event;
    void (*event_callback)(void *arg);

    while (running)
    {
        event = (Event *) list_pop(&events);
        event_callback = NULL;

        if (!event)
            continue;

        if (event->type == EVENT_RESET)
            event_callback = eventcb_reset;

        if (event->type == EVENT_DELAY)
            event_callback = eventcb_delay;

        if (event->type == EVENT_ELEVATE)
            event_callback = eventcb_elevate;

        if (event->type == EVENT_WALK)
            event_callback = eventcb_walk;

        if (!event_callback)
            continue;

        (*event_callback)(event->data);

        if (event->data)
            free(event->data);
        free(event);
    }

    return (void *) NULL;
}

void event_add(int event_type, void *data)
{
    Event *event = malloc(sizeof(Event));

    event->type = event_type;
    event->data = data;

    if (LOG_EVENT_ADD)
    {
        char *log_msg = malloc(sizeof(char) * LOG_LINE_MAXLEN);
        snprintf(log_msg, LOG_LINE_MAXLEN, "[Event] Added event. (type: %s)", event_getname(event_type));
        log_event(log_msg);
        free(log_msg);
    }

    list_push(&events, (void *) event);
}

static char *event_getname(int event_type)
{
    switch (event_type)
    {
        case EVENT_RESET:
            return "EVENT_RESET";
        case EVENT_DELAY:
            return "EVENT_DELAY";
        case EVENT_ELEVATE:
            return "EVENT_ELEVATE";
        case EVENT_WALK:
            return "EVENT_WALK";
    }

    return NULL;
}

#endif