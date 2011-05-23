/* 
 * File:   globals.h
 * Author: megabyte
 *
 * Created on May 2, 2011, 6:31 PM
 */

#ifndef __AVSSERVER_GLOBALS_H__
#define	__AVSSERVER_GLOBALS_H__


#include "../common/common.h"

// data about the handler threads
typedef struct
{
    pthread_t tid;
    bool alive;
} client_thread;

static client_thread thread_pool[MAX_DCCP_CONNECTION_BACK_LOG];

static int rv_sock;
static pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

#endif	/* __AVSSERVER_GLOBALS_H__ */
