#include "clhandler.h"


// Securely free client handler data.
void ch_data_free(ch_data *data)
{
    if (data == NULL)
        return;

    if (data->avmanager != NULL)
        free (data->avmanager);
    free (data);
}


/* Client handler thread
 */
void* ch_thread(void* data)
{
    ch_data* dp = (ch_data*)data;

    int client_sock = dp->sock;
    int pool_index = dp->pool_index;
    struct sockaddr_in rem_addr = dp->rem_addr;

    int rc;

    printf("Client connected: %s\n", inet_ntoa(rem_addr.sin_addr));

    pthread_t rthread_id, sthread_id, stream_thread_id;
    
    /* initializing the stream reading thread */
    rc = pthread_create(&stream_thread_id, NULL, stream_read_thread, (void*)dp);
    if(rc < 0)
        error("Error creating Stream Reader thread\n", 1);
    
    /* initializing the recv and send threads */
    rc = pthread_create(&rthread_id, NULL, recv_thread, (void*)dp);
    if(rc < 0)
        error("Error creating Receive thread\n", 1);

    rc = pthread_create(&sthread_id, NULL, send_thread, (void*)dp);
    if(rc < 0)
        error("Error creating Send thread\n", 1);


    pthread_join(rthread_id, NULL);
    pthread_join(sthread_id, NULL);
    pthread_join(stream_thread_id, NULL);

    close(client_sock);
    destroy_fifo(dp->private_fifo);
    ch_data_free(dp);


    LOCK(&server_mutex);
    thread_pool[pool_index].alive = false;
    UNLOCK(&server_mutex);

    return NULL;
}


/* Thread used to send audio video streams */
void* send_thread(void* data)
{
    int rc, sequence_nr, timestamp_nr;
    ch_data* dp = (ch_data*)data;
    pthread_mutex_t* p_fifo_mutex = dp->p_fifo_mutex;
    
    int client_sock = dp->sock;

    fifo_elem fe;
    rtp_packet* packet = create_packet();
    AVMediaPacket *media_packet = NULL; // AVMediaPacket alias/pointer to fifo_elem.

    sequence_nr = SEQUENCE_START;
    timestamp_nr = TIMESTAMP_START;
    while(1)
    {
        memset(packet->payload, 0, PAYLOAD_MAX_SIZE);
        
        LOCK(p_fifo_mutex);

        rc = dequeue(dp->private_fifo, &fe);

        UNLOCK(p_fifo_mutex);

        if (rc == -1)
        {
            usleep(5 * dp->st_rate);
            continue;
        }

        media_packet = fe;
        
        /* Create new rtp packet to be sent.. */
        if (media_packet->entire_media_packet_size > PAYLOAD_MAX_SIZE) {
            int done = 0;
            
            while(done < media_packet->entire_media_packet_size) {
                memset(packet->payload, 0, PAYLOAD_MAX_SIZE);
                memcpy(packet->payload, (char*)fe + done, PAYLOAD_MAX_SIZE - 1);
                
                packet->header.seq = sequence_nr++;
                packet->header.timestamp = timestamp_nr;
                packet->header.M = done == 0 ? MARKER_FIRST : MARKER_FRAGMENT;
                
                done = done + PAYLOAD_MAX_SIZE - 1;
                
                if (done >= media_packet->entire_media_packet_size)
                    packet->header.M = MARKER_LAST;
                
                int rc = send(client_sock, packet, sizeof(*packet), 0);
                if (rc < 0)
                    error("Error sending a packet to the client");
            }
        } else {
            packet->header.seq = sequence_nr++;
            packet->header.timestamp = timestamp_nr;
            packet->header.M = MARKER_ALONE;
            
            memcpy(packet->payload, (char*)fe, media_packet->entire_media_packet_size);
            
            int rc = send(client_sock, packet, sizeof(*packet), 0);
            if (rc < 0)
                error("Error sending a packet to the client");
            
            // Free the AVMediaPacket dequeued from the queue.
            AVManager::free_packet (media_packet);
            media_packet = NULL;
            fe = NULL;
        }
        timestamp_nr++;

        usleep(dp->st_rate);
    }
}


/* Thread used to receive feedback from client */
void* recv_thread(void* data)
{
    ch_data* dp = (ch_data*)data;

    int client_sock = dp->sock;

    char buff[512];

    while(1) {
        memset(buff, 0, 512);
        int rc = recv(client_sock, buff, 512, 0);
        if (rc == 0)            /* peer has shutdown the socket */
        {
            printf("Client shutdown\n");
            break;
        }
        
        printf("Feedback size %d: %s\n", rc, buff);
        fflush(stdout);
    }

}


/* Thread used to read the data streams and put them in the queue */
void* stream_read_thread(void* data)
{
    ch_data* dp = (ch_data *)data;
    pthread_mutex_t *p_fifo_mutex = dp->p_fifo_mutex;
    AVMediaPacket *media_packet = NULL;
    int queue_status = 0;
    int max_enqueue_tries = MAX_ENQUEUE_TRIES;

    // Init server-size codecs and open media file.
    if (dp->avmanager->init_send (dp->filename) == false)
        goto CLEAN_AND_EXIT;

    // Get useful media information for client and push them in the queue.
    if (dp->avmanager->get_stream_info (&dp->stream_info) == false)
        goto CLEAN_AND_EXIT;
    if (AVManager::stream_info_to_packet (&dp->stream_info, &media_packet))
        goto CLEAN_AND_EXIT;
    queue_status = enqueue (dp->private_fifo, (fifo_elem)media_packet);
    if (queue_status < 0) {
        DBGPRINT ("[stream_read_thread] Queue is full!");
        goto CLEAN_AND_EXIT;
    }

    // Loop for media packets.
    while (dp->avmanager->read_packet_from_file (&media_packet))
    {
        
        
        max_enqueue_tries = MAX_ENQUEUE_TRIES;
        do {
            LOCK (p_fifo_mutex);
            queue_status = enqueue (dp->private_fifo, (fifo_elem)media_packet);
            UNLOCK (p_fifo_mutex);
            if (queue_status < 0)
                sleep (1);
            max_enqueue_tries--;
        } while (queue_status < 0 && max_enqueue_tries > 0);

        if (queue_status < 0) {
            DBGPRINT ("[stream_read_thread] Queue is full!");
            UNLOCK (p_fifo_mutex);
            break;
        }
        
        // Don't free the media packet each time, because the enqueue()
        // method copies the reference, NOT the content of the fifo_elem.
        
        media_packet = NULL;
    }

CLEAN_AND_EXIT:
    if (dp->avmanager != NULL) {
        if (media_packet != NULL)
            AVManager::free_packet (media_packet);
            media_packet = NULL;
        dp->avmanager->end_send ();
    }

/*
    Dummy :p code. That's why we don't use it.

    fifo_elem fe;
    rtp_packet *packet = (rtp_packet *)&fe;

    // dummy code begin
    FILE* fh = fopen(dp->filename, "r");
    if (NULL == fh)
        error("Unable to open the stream file", 1);
    
    while(fread(&fe, sizeof(fe), 1, fh))
    {
        LOCK(p_fifo_mutex);

        int st = enqueue(dp->private_fifo, fe);
        if (st < 0)
            error("[stream_read_thread] Queue is full!", 1);

        UNLOCK(p_fifo_mutex);
    }
    
    // dummy code end
*/
}