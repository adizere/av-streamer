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
#include <time.h>

//General defines and includes needed
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <errno.h>

#define SOCK_DCCP 6
#define IPPROTO_DCCP 33  //it must be this number. This number is assigned by IANA to DCCP
#define SOL_DCCP 269
#define MAX_DCCP_CONNECTION_BACK_LOG 5

void error(const char* msg)
{
    perror(msg);
    fflush(stderr);
    fflush(stdout);
}


#include "fifo.h"
#include "rtp.h"
// TODO include fifo.h, rtp.h ...


#endif	/* _COMMON_H */

