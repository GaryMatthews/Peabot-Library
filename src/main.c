#ifndef MAIN_DEF
#define MAIN_DEF

/*
 File:          main.c
 Description:   Main object for the Peabot application.
 Created:       May 5, 2017
 Author:        Matt Mumau
 */

//#define PEABOT_DBG

/* System dependencies */
#include <sys/prctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

/* Raspberry Pi includes */
#include <wiringPi.h>

/* Application includes */
#include "config.h"
#include "log.h"
#include "console.h"
#include "prompt.h"
#include "events.h"
#include "keyframe_handler.h"
#include "robot.h"
#include "http_server.h"
#include "usd_sensor.h"

/* Header */
#include "main.h"

static bool running = true;
static unsigned short exit_val = 1;

/* Forward decs */
static void signal_handler(int signum);

void app_exit(int retval)
{
    log_event("[MAIN] Shutting down. Bye!");

    http_halt();
    prompt_halt();
    event_halt();
    keyhandler_halt();
    robot_halt();
    usd_sensor_halt();
    config_destroy();
    log_close();

    exit_val = retval;
    running = false;  
}

void app_error(const char *file, unsigned int lineno, const char *msg, unsigned short error_code)
{
    char err_msg[256];
    snprintf(err_msg, sizeof(err_msg), "[ERROR!] %s [f:%s,l:%d,e:%d]", msg, file, lineno, error_code);
    log_event(err_msg);   
    app_exit(error_code);
}

/*
 Handles all posix signals.
 */
static void signal_handler(int signum)
{
    if (signum == SIGINT)
    {
        log_event("POSIX SIGINT received. Exiting...");
        app_exit(0);
    }
}

/* Application main */
int main(int argc, char *argv[])
{
    prctl(PR_SET_NAME, "PEABOT_MAIN\0", NULL, NULL, NULL);
    config_init(argc, argv);
    log_init();
    signal(SIGINT, signal_handler);
    console_h("Peabot Server: " APP_VERSION); 
    wiringPiSetup();

    usd_sensor_init();
    robot_init();
    keyhandler_init();
    event_init();
    prompt_init();
    http_init();

    log_event("[MAIN] Peabot server initialized.");

    while (running) 
        sleep(1);

    exit(exit_val);
    return exit_val;
}

#endif