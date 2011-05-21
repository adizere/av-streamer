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
    int rc, seq;
    ch_data* dp = (ch_data*)data;
    pthread_mutex_t* p_fifo_mutex = dp->p_fifo_mutex;
    
    int client_sock = dp->sock;

    fifo_elem fe;
    rtp_packet* packet = (rtp_packet*)&fe;

    seq = 0;
    while(1)
    {

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
        // TODO: sizeof(fe) is wrong..
        // TODO: fill protocol information here

        // dummy code begin
        packet->header.seq = seq;
        seq++;

        printf("Packet: seq:%d payload:%d\n",packet->header.seq, packet->payload);
        // dummy code end

        int rc = send(client_sock, &fe, sizeof(fe), 0);
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
        packet->payload = i; // TODO fill payload here

        LOCK(p_fifo_mutex);

        enqueue(private_fifo, fe);

        UNLOCK(p_fifo_mutex);
    }
    // dummy code end
}
