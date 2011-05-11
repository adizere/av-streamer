/* 
 * File:   newfile.h
 * Author: megabyte
 *
 * Created on May 2, 2011, 6:03 PM
 */

#ifndef _CLHANDLER_H
#define	_CLHANDLER_H

#include "globals.h"
#include "../common/common.h"

//TODO - add queue and queue protection(mutex)
// client handler data
typedef struct
{
    int sock;
    int pool_index;
    const char* filename;
    struct sockaddr_in rem_addr;
} ch_data;


// client handler thread
void* ch_thread(void* data);

/* Thread used to send audio video streams */
void* send_thread(void* data);

/* Thread used to receive feedback from client */
void* recv_thread(void* data);

#endif	/* _NEWFILE_H */

