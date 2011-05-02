/* 
 * File:   newfile.h
 * Author: megabyte
 *
 * Created on May 2, 2011, 6:03 PM
 */

#ifndef _CLHANDLER_H
#define	_CLHANDLER_H

#include "globals.h"
#include "common.h"


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


#endif	/* _NEWFILE_H */

