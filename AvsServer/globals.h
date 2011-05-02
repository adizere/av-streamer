/* 
 * File:   globals.h
 * Author: megabyte
 *
 * Created on May 2, 2011, 6:31 PM
 */

#ifndef _GLOBALS_H
#define	_GLOBALS_H

#include "common.h"

// data about the handler threads
typedef struct
{
    pthread_t tid;
    bool alive;
} client_thread;

static client_thread thread_pool[MAX_DCCP_CONNECTION_BACK_LOG];

static int32_t g_sum = 0;
static int rv_sock;

#endif	/* _GLOBALS_H */

