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

// client handler data
typedef struct
{
    int sock;
    int pool_index;
    useconds_t st_rate;                 /* server transmission rate - interpreted in microseconds */
    const char* filename;
    struct sockaddr_in rem_addr;

    fifo* private_fifo;                 /* the queue used for this particular client */
    pthread_mutex_t* p_fifo_mutex;       /* what thread owns the queue ? */

    AVManager  *avmanager;              ///< Audio video manager for coding/decoding media packets.
    streaminfo stream_info;             ///< Audio video stream information shared with client.

    // TODO add fifo+mtx for feedback information
} ch_data;

// Securely free client handler data.
void ch_data_free(ch_data *data);

// client handler thread
void* ch_thread(void* data);

/* Thread used to send audio video streams */
void* send_thread(void* data);

/* Thread used to receive feedback from client */
void* recv_thread(void* data);

/* Thread used to read the data streams and put them in the queue */
void* stream_read_thread(void* data);

#endif	/* _NEWFILE_H */
