/* 
 * File:   common.h
 * Author: megabyte
 *
 * Created on March 30, 2011, 10:08 PM
 */

#ifndef _COMMON_H
#define	_COMMON_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <errno.h>

#define SOL_DCCP 269
#define MAX_DCCP_CONNECTION_BACK_LOG 5
#define LOCAL_PORT 25555

#define FIFO_DEFAULT_CAPACITY 1000      // TODO: decide how big should this be

#define CLIENT_RECV_TIMEOUT 5           /* how much time do we wait for a packet? */
#define CLIENT_FIFO_POLL_TIMEOUT 1000   /* how much time do we wait for data in queue 
                                         * interpreted in miliseconds
                                         */

#define LOCK(x) pthread_mutex_lock(x)
#define UNLOCK(x) pthread_mutex_unlock(x)

const char* const LOCAL_IP = "127.0.0.1" ;


#define DBGPRINT(format, ...) fprintf (stderr, format "\n", ## __VA_ARGS__)


static void error(const char* msg, int exit_flag = 0)
{
    /* sometimes perror() writes Success, example: 
     * "Please specify the name of the file and the transmission rate as parameters: Success"
     */
    
    if (errno == 0) {
        fprintf(stderr, "Error: %s\n", msg);
    } else {
        perror(msg);
    }
    
    fflush(stderr);
    fflush(stdout);

    if (exit_flag ==  1)
        exit(EXIT_FAILURE);
}


#include "audiovideo.h"
#include "fifo.h"
#include "rtp.h"


#endif	/* _COMMON_H */