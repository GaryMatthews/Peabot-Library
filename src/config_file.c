#ifndef CONFIG_FILE_DEF
#define CONFIG_FILE_DEF

/*
 File:          config_file.c
 Description:   Implementation of config file functionality.
 Created:       May 19, 2017
 Author:        Matt Mumau
 */

/* System libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Application includes */
#include "main.h"
#include "config.h"
#include "string_utils.h"

/* Headxer */
#include "config_file.h"

/* Forward decs */
static void configfile_parse(FILE *config_file);
static void configfile_handle_line(char *arg, char *val);

void configfile_process(char *config_file_fullpath)
{
    FILE *config_file = fopen(config_file_fullpath, "r");
    if (!config_file)
        APP_ERROR("Could not open config file.", 1);

    configfile_parse(config_file);
    fclose(config_file);
}

static void configfile_parse(FILE *config_file)
{
    if (!config_file)
        return;

    int buffer_size = 128;
    char buffer[buffer_size];

    char *arg = NULL;
    char *val = NULL;

    char *comment_character = "#";
    char *config_delim = " ";

    while (fgets(buffer, buffer_size, config_file) != NULL)
    {
        if (str_starts(buffer, comment_character) || str_empty(buffer))
            continue;
     
        str_removenl(buffer);
        val = str_tabval(buffer, buffer_size);
        arg = strtok(buffer, config_delim);

        configfile_handle_line(arg, val);
    } 
}

static void configfile_handle_line(char *arg, char *val)
{
    if (arg == NULL)
        return;

    if (str_equals(arg, "log_file_dir"))
        config_set(CONF_LOG_FILE_DIR, (void *) val, true);

    if (str_equals(arg, "log_stdin"))
        config_set(CONF_LOG_STDIN, (void *) val, true);

    if (str_equals(arg, "log_prompt_commands"))
        config_set(CONF_LOG_PROMPT_COMMANDS, (void *) val, true);

    if (str_equals(arg, "log_event_add"))
        config_set(CONF_LOG_EVENT_ADD, (void *) val, true);

    if (str_equals(arg, "log_event_callbacks"))
        config_set(CONF_LOG_EVENT_CALLBACKS, (void *) val, true);  

    if (str_equals(arg, "log_keyframes"))
        config_set(CONF_LOG_KEYFRAMES, (void *) val, true);

    if (str_equals(arg, "pca_9685_pin_base"))
        config_set(CONF_PCA_9685_PIN_BASE, (void *) val, true);

    if (str_equals(arg, "pca_9685_max_pwm"))
        config_set(CONF_PCA_9685_MAX_PWM, (void *) val, true); 

    if (str_equals(arg, "pca_9685_hertz"))
        config_set(CONF_PCA_9685_HERTZ, (void *) val, true);

    if (str_equals(arg, "servos_num"))
        config_set(CONF_SERVOS_NUM, (void *) val, true);                      

    if (str_equals(arg, "robot_tick"))
        config_set(CONF_ROBOT_TICK, (void *) val, true);

    if (str_equals(arg, "transitions_enable"))
        config_set(CONF_TRANSITIONS_ENABLE, (void *) val, true);

    if (str_equals(arg, "transition_time"))
        config_set(CONF_TRANSITIONS_TIME, (void *) val, true);

    if (str_equals(arg, "walk_hip_delta"))
        config_set(CONF_WALK_HIP_DELTA, (void *) val, true);

    if (str_equals(arg, "walk_knee_delta"))
        config_set(CONF_WALK_KNEE_DELTA, (void *) val, true);

    if (str_equals(arg, "walk_knee_pad_a"))
        config_set(CONF_WALK_KNEE_PAD_A, (void *) val, true);

    if (str_equals(arg, "walk_knee_pad_b"))
        config_set(CONF_WALK_KNEE_PAD_B, (void *) val, true);

    if (str_equals(arg, "http_enabled"))
        config_set(CONF_HTTP_ENABLED, (void *) val, true);

    if (str_equals(arg, "http_port"))
        config_set(CONF_HTTP_PORT, (void *) val, true);

    if (str_equals(arg, "back_left_knee") ||
        str_equals(arg, "back_left_hip") ||
        str_equals(arg, "front_left_knee") ||
        str_equals(arg, "front_left_hip") ||
        str_equals(arg, "back_right_knee") ||
        str_equals(arg, "back_right_hip") ||
        str_equals(arg, "front_right_knee") ||
        str_equals(arg, "front_right_hip")
        )
    {
        int servo_index = config_str_to_servo_index(arg);
        int servo_val = (int) atoi(val);
        ServoPinData servo_pin_data = (ServoPinData) { servo_index, servo_val };
        config_set(CONF_SERVO_PINS, (void *) &servo_pin_data, false);
    }

    if (str_equals(arg, "back_left_knee_limits") ||
        str_equals(arg, "back_left_hip_limits") ||
        str_equals(arg, "front_left_knee_limits") ||
        str_equals(arg, "front_left_hip_limits") ||
        str_equals(arg, "back_right_knee_limits") ||
        str_equals(arg, "back_right_hip_limits") ||
        str_equals(arg, "front_right_knee_limits") ||
        str_equals(arg, "front_right_hip_limits")
        )
    {    
        int servo_index = config_str_to_servo_index(arg);

        char *min_tok = strtok(val, "-");
        char *max_tok = strtok(NULL, "-");

        int min = (int) atoi(min_tok);
        int max = (int) atoi(max_tok);

        ServoLimitData servo_limit_data = (ServoLimitData) { servo_index, min, max };
        config_set(CONF_SERVO_LIMITS, (void *) &servo_limit_data, false);
    }  
}

#endif