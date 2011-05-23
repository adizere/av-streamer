#include "clhandler.h"


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
    free(dp);


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

    sequence_nr = SEQUENCE_START;
    timestamp_nr = 1;
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
        
        /* Create new rtp packet to be sent..
        */
        
        /* Adi: Nu-mi merge ceva la compilare legat de audiovideo.h si NU am 
         * testat crearea pachetelor si punerea lor pe retea.. deci e un TODO mare aici:
         */
        if (sizeof(fe) > PAYLOAD_MAX_SIZE) {
            int done = 0;
            
            while(done < sizeof(fe)) {
                memset(packet->payload, 0, PAYLOAD_MAX_SIZE);
                memcpy(packet->payload, (char*)&fe + done, PAYLOAD_MAX_SIZE - 1);
                
                packet->header.seq = sequence_nr++;
                packet->header.timestamp = timestamp_nr;
                packet->header.M = done == 0 ? MARKER_FIRST : MARKER_FRAGMENT;
                 
                done = done + PAYLOAD_MAX_SIZE - 1;
                
                if (done >= sizeof(fe))
                    packet->header.M = MARKER_LAST;
                
                int rc = send(client_sock, packet, sizeof(packet), 0);
                if (rc < 0)
                    error("Error sending a packet to the client");
            }
        } else {
            packet->header.seq = sequence_nr++;
            packet->header.timestamp = timestamp_nr;
            packet->header.M = MARKER_ALONE;
            
            memcpy(packet->payload, (char*)&fe, sizeof(fe));
            
            int rc = send(client_sock, packet, sizeof(packet), 0);
            if (rc < 0)
                error("Error sending a packet to the client");
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
    ch_data* dp = (ch_data*)data;
    fifo* private_fifo = dp->private_fifo;
    pthread_mutex_t* p_fifo_mutex = dp->p_fifo_mutex;

    fifo_elem fe;
    rtp_packet* packet = (rtp_packet*)&fe;
    /* Adi:
     * Johannes should open dp->filename and populate the queue held in
     * dp->private_fifo;
     * Example: enqueue(dp->private_fifo, pkg), where pkg is of type rtp_packet
     */

    // dummy code begin
    for(int i = 1; i <= 10; ++i)
    {

        LOCK(p_fifo_mutex);

        enqueue(private_fifo, fe);

        UNLOCK(p_fifo_mutex);
    }
    // dummy code end
}
