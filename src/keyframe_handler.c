#ifndef KEYFRAME_HANDLER_DEF
#define KEYFRAME_HANDLER_DEF

/*
 File:          keyframe_handler.c
 Description:   Implementation of keyframe animation processing functions.
 Created:       May 11, 2017
 Author:        Matt Mumau
 */

#define _POSIX_C_SOURCE 199309L

/* System includes */
#include <sys/prctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

/* Application includes */
#include "main.h"
#include "config_defaults.h"
#include "config.h"
#include "log.h"
#include "list.h"
#include "easing_utils.h"
#include "utils.h"
#include "robot.h"
#include "keyframe_factory.h"

/* Header */
#include "keyframe_handler.h"

static pthread_t keyhandler_thread;
static bool running;
static bool exec_remove_all = false;
static int error;

static List *keyframes = NULL;

static Keyframe *last_keyfr;
static ServoPos *last_servopos;

/* Forward decs */
static void *keyhandler_main(void *arg);
static double keyhandler_mappos(double perc, ServoPos *servo_pos);
static void keyhandler_exec_removeall();
static void keyhandler_add_transition(size_t len, Keyframe *src, Keyframe *dest);
static void keyhandler_copy_keyfr(Keyframe *dest, Keyframe *src, size_t len);
static void keyhandler_log_keyfr(Keyframe *keyfr);
static void keyhandler_keyfr_destroy(Keyframe *keyfr);
static void keyhandler_set_robot(Keyframe *keyfr, size_t len, double time);
static void keyhandler_resey_keyfr(Keyframe *keyfr, size_t len);

void keyhandler_init()
{
    unsigned short *servos_num = (unsigned short *) config_get(CONF_SERVOS_NUM);

    last_keyfr = calloc(1, sizeof(Keyframe));
    if (!last_keyfr)
        APP_ERROR("Could not allocate memory.", error);

    last_servopos = calloc(*servos_num, sizeof(ServoPos));
    if (!last_servopos)
        APP_ERROR("Could not allocate memory.", error);

    keyhandler_resey_keyfr(last_keyfr, *servos_num);
    last_keyfr->servo_pos = last_servopos;

    #ifdef PEABOT_DBG
    printf("-----ORIGINAL KEYFR-----\n");
    keyhandler_print_keyfr(last_keyfr, *servos_num); 
    #endif  

    running = true;
    error = pthread_create(&keyhandler_thread, NULL, keyhandler_main, NULL);
    if (error)
        APP_ERROR("Could not initialize keyframe thread.", error);
}

void keyhandler_halt()
{
    running = false;
    pthread_join(keyhandler_thread, NULL);

    if (last_keyfr->servo_pos)
        free(last_keyfr->servo_pos);
    last_keyfr->servo_pos = NULL;

    if (last_keyfr)
        free(last_keyfr);
    last_keyfr = NULL;
}

void keyhandler_add(unsigned short keyfr_type, void *data, bool reverse, bool skip_transitions)
{
    unsigned short *servos_num = (unsigned short *) config_get(CONF_SERVOS_NUM);
    bool *transitions_enable = (bool *) config_get(CONF_TRANSITIONS_ENABLE);
    
    Keyframe *keyfr = calloc(1, sizeof(Keyframe));
    if (!keyfr)
        APP_ERROR("Could not allocate memory.", 1);

    ServoPos *servo_pos = calloc(*servos_num, sizeof(ServoPos));
    if (!servo_pos)
        APP_ERROR("Could not allocate memory.", 1);    
    keyfr->servo_pos = servo_pos;

    bool (*keyfactory_cb)(Keyframe *keyfr, size_t len, void *data, bool reverse);
    keyfactory_cb = NULL;

    if (keyfr_type == KEYFR_RESET)
        keyfactory_cb = keyfactory_reset;

    if (keyfr_type == KEYFR_DELAY)
        keyfactory_cb = keyfactory_delay;

    if (keyfr_type == KEYFR_ELEVATE)
        keyfactory_cb = keyfactory_elevate;

    if (keyfr_type == KEYFR_WALK)
        keyfactory_cb = keyfactory_walk;

    if (keyfr_type == KEYFR_EXTEND)
        keyfactory_cb = keyfactory_extend;

    if (keyfr_type == KEYFR_TURN)
        keyfactory_cb = keyfactory_turnsegment;

    if (keyfr_type == KEYFR_STRAFE)
        keyfactory_cb = keyfactory_strafe;

    bool success = false;
    if (keyfactory_cb != NULL)
        success = (*keyfactory_cb)(keyfr, *servos_num, data, reverse);

    if (data)
        free(data);
    data = NULL;

    if (!success)
    {
        if (keyfr != NULL)
            free(keyfr);
        keyfr = NULL;

        if (servo_pos != NULL)
            free(servo_pos);
        servo_pos = NULL;

        return;
    }

    // Remember, the transition should come before the keyframe...
    if (keyfr_type != KEYFR_DELAY && *transitions_enable && !skip_transitions)
        keyhandler_add_transition(*servos_num, last_keyfr, keyfr);

    list_push(&keyframes, (void *) keyfr);

    #ifdef PEABOT_DBG
    printf("-----ACTIVE KEYFR-----\n");
    keyhandler_print_keyfr(keyfr, *servos_num);
    #endif
    
    keyhandler_copy_keyfr(last_keyfr, keyfr, *servos_num);
}

void keyhandler_removeall()
{
    exec_remove_all = true;
}

static void keyhandler_add_transition(size_t len, Keyframe *src, Keyframe *dest)
{
    Keyframe *keyfr = calloc(1, sizeof(Keyframe));
    if (!keyfr)
        APP_ERROR("Could not allocate memory.", 1);

    ServoPos *servo_pos = calloc(len, sizeof(ServoPos));
    if (!servo_pos)
        APP_ERROR("Could not allocate memory.", 1); 

    keyfr->servo_pos = servo_pos;

    double *trans_duration = (double *) config_get(CONF_TRANSITIONS_TIME);
    keyfr->duration = *trans_duration;    

    bool success = keyfactory_transition(keyfr, len, src, dest);

    if (!success)
    {
        if (keyfr)
            free(keyfr);
        keyfr = NULL;

        if (servo_pos)
            free(servo_pos);
        servo_pos = NULL;

        return;
    } 

    list_push(&keyframes, (void *) keyfr);    
}

static void keyhandler_exec_removeall()
{
    Keyframe *keyfr_popped = NULL;
    keyfr_popped = (Keyframe *) list_pop(&keyframes);
    while (keyfr_popped != NULL)
    {
        if (keyfr_popped)
            free(keyfr_popped);    
        keyfr_popped = (Keyframe *) list_pop(&keyframes);
    }
    keyframes = NULL;
}

static void *keyhandler_main(void *arg)
{
    prctl(PR_SET_NAME, "PEABOT_KEYFR\0", NULL, NULL, NULL);

    unsigned short *servos_num = (unsigned short *) config_get(CONF_SERVOS_NUM);

    struct timespec time;
    struct timespec last_time;
    double next = 0.0;
    
    Keyframe *keyfr;
    Keyframe *tmp_key;
    ServoPos *servo_pos;
    
    clock_gettime(CLOCK_MONOTONIC, &last_time);
    while (running)
    {
        clock_gettime(CLOCK_MONOTONIC, &time);
        next += utils_timediff(time, last_time);
        last_time = time;  

        if (exec_remove_all)
        {
            keyhandler_exec_removeall();
            exec_remove_all = false;
            next = 0.0;
            continue;
        }

        if (!keyframes)
        {
            next = 0.0;
            continue;       
        }        

        keyfr = (Keyframe *) keyframes->data;
        servo_pos = keyfr->servo_pos != NULL ? keyfr->servo_pos : NULL;        

        if (next > keyfr->duration)
        {
            tmp_key = (Keyframe *) list_pop(&keyframes);
            keyhandler_log_keyfr(tmp_key);  
            keyhandler_keyfr_destroy(tmp_key);
            next = 0.0;
            continue;
        }           

        if (!keyfr->is_delay && servo_pos)
            keyhandler_set_robot(keyfr, *servos_num, next);
    }

    return (void *) NULL;
}

static void keyhandler_set_robot(Keyframe *keyfr, size_t len, double time)
{
    if (!keyfr || !keyfr->servo_pos)
        return;

    ServoPos * servo_pos = keyfr->servo_pos;
    double perc, pos, begin_time, end_time, adjusted_duration;

    for (unsigned short i = 0; i < len; i++)
    {
        begin_time = keyfr->duration * servo_pos[i].begin_pad;
        end_time = keyfr->duration * servo_pos[i].end_pad;
        adjusted_duration = keyfr->duration - begin_time - end_time;  
        
        perc = (time - begin_time) / adjusted_duration;

        if (perc < 0.0)
            perc = 0.0;
        if (perc > 1.0)
            perc = 1.0;

        pos = keyhandler_mappos(perc, &servo_pos[i]);
        robot_setservo(i, pos);
    }    
}

static void keyhandler_keyfr_destroy(Keyframe *keyfr)
{
    if (!keyfr)
        return;

    if (keyfr->servo_pos)
        free(keyfr->servo_pos);
    keyfr->servo_pos = NULL; 

    free(keyfr);        
}

static double keyhandler_mappos(double perc, ServoPos *servo_pos)
{
    double diff, modifier, delta, final;

    diff = servo_pos->end_pos - servo_pos->start_pos;
    modifier = easing_calc(servo_pos->easing, perc);
    delta = diff * modifier;    
    final = servo_pos->start_pos + delta;

    // todo: remove
    //printf("diff: %f, modifier: %f, delta: %f, final: %f\n", diff, modifier, delta, final);

    return final;
}

static void keyhandler_copy_keyfr(Keyframe *dest, Keyframe *src, size_t len)
{
    if (!dest || !src)
        return;

    dest->duration = src->duration;
    dest->is_delay = src->is_delay;
    
    if (src->servo_pos == NULL || dest->servo_pos == NULL)
    {
        dest->servo_pos = NULL;
        return;
    }

    ServoPos *tmp_src_srv;
    ServoPos *tmp_dest_srv;
    for (unsigned short i = 0; i < len; i++)
    {
        tmp_src_srv = (ServoPos *) &(src->servo_pos[i]);
        tmp_dest_srv = (ServoPos *) &(dest->servo_pos[i]);

        tmp_dest_srv->easing = tmp_src_srv->easing;
        tmp_dest_srv->start_pos = tmp_src_srv->start_pos;
        tmp_dest_srv->end_pos = tmp_src_srv->end_pos;
        tmp_dest_srv->begin_pad = tmp_src_srv->begin_pad;
        tmp_dest_srv->end_pad = tmp_src_srv->end_pad; 
    }
}

static void keyhandler_log_keyfr(Keyframe *keyfr)
{
    bool *log_keyframes = (bool *) config_get(CONF_LOG_KEYFRAMES);
    if (!*log_keyframes)
        return;

    char msg[LOG_LINE_MAXLEN];
    snprintf(msg, sizeof(msg), "[KYFR] Completed keyframe. (duration: %f, is_delay: %s)", keyfr->duration, keyfr->is_delay ? "true" : "false");
    log_event(msg);
}

void keyhandler_print_keyfr(Keyframe *keyfr, size_t len)
{
    if (!keyfr)
        return;

    printf("keyfr->duration: %f\n", keyfr->duration);
    printf("keyfr->is_delay: %s\n", keyfr->is_delay ? "true" : "false");

    if (keyfr->servo_pos)
    {
        ServoPos *tmp_srv;
        printf("keyfr->servo_pos:\n");
        for (int i = 0; i < len; i++)
        {
            tmp_srv = (ServoPos *) &(keyfr->servo_pos[i]);
            printf("\t[%d] easing: %d, \tstart_pos: %f, \t\tend_pos: %f, \t\tbegin_pad: %f, \t\tend_pad: %f\n", 
                i, (unsigned short) tmp_srv->easing, (double) tmp_srv->start_pos, (double) tmp_srv->end_pos, (double) tmp_srv->begin_pad, (double) tmp_srv->end_pad);
        }
    }   
    else
    {
        printf("keyfr->servo_pos: NULL\n");
    }
}

static void keyhandler_resey_keyfr(Keyframe *keyfr, size_t len)
{
    if (!keyfr)
        return;

    keyfr->duration = 0.0;
    keyfr->is_delay = false; 
    keyfr->servo_pos = NULL;
}

#endif