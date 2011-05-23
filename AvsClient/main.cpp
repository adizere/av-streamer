/* 
 * File:   main.cpp
 * Author: megabyte
 *
 * Created on March 30, 2011, 10:02 PM
 */

#include "globals.h"
#include "../common/common.h"


int socket_handle;      /* global socket */

fifo* av_packets_queue;
pthread_mutex_t p_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

/* This flag is marked when the recv_thread has finished receiving from the server
 * In this way, we know when we should stop trying to dequeue from av_packets_queue
 */
int flag_transmission_finished = 0;


void sigint_handler(int signal) {
    printf("Client shutting down..\n");

    close(socket_handle);
    
    // close the player
    
    
    destroy_fifo(av_packets_queue);
    
    exit(EXIT_SUCCESS);
}


/* Thread used to receive incoming data streams */
void* recv_thread(void*)
{
    rtp_packet* packet = create_packet();

    while(1)
    {
        int rc = recv(socket_handle, packet, sizeof(rtp_packet), 0);
        if (rc == 0)  /* peer has shutdown the socket */
        {
            printf("Server was shutdown.. \n");
            break;
        }

        LOCK(&p_queue_mutex);

        rc = enqueue(av_packets_queue, packet->payload);
        if (rc < 0)
            error("The queue is full - adjust the FIFO_DEFAULT_CAPACITY to a bigger value. Quitting!", 1);

        UNLOCK(&p_queue_mutex);
        
        if (packet->header.M == MARKER_LAST_PACKET)
            break;
    }
    printf("Transmission from server finished.\n");
    flag_transmission_finished = 1;
}


/* Thread used to send feedback to the server */
void* send_thread(void*)
{

    int count = 0;
    char buff[512];
    while(1)
    {
        sprintf(buff, "dummy feedback %d", count++);
        int rc = send(socket_handle, buff, strlen(buff)+1, 0);
        if (rc < 0)
            break;
        sleep(5);
        printf("Client feedback sent: %d bytes\n", rc);
    }

}


/* Take packets from the audio and video queues */
void* play_thread(void*)
{
    int rc;
    fifo_elem fe;
    while(flag_transmission_finished == 0)
    {
        LOCK(&p_queue_mutex);
        
        rc = dequeue(av_packets_queue, &fe);
        
        UNLOCK(&p_queue_mutex);
        
        if (rc < 0) {
             // usleep(1000);
             continue;
        }
        
        // play...
    }
}


int main(int argc, char** argv)
{
    signal(SIGINT, sigint_handler);
    
    socket_handle = socket(PF_INET, SOCK_DCCP, IPPROTO_DCCP);

    /* socket setup */
    int on = 1;
    int result = setsockopt(socket_handle, SOL_DCCP, SO_REUSEADDR, (const char *) &on, sizeof(on));

    struct sockaddr_in conn_address;
    conn_address.sin_family = AF_INET;
    conn_address.sin_addr.s_addr = inet_addr(LOCAL_IP);
    conn_address.sin_port = htons(LOCAL_PORT);

    result = connect(socket_handle, (struct sockaddr *)&conn_address, sizeof(conn_address));
    if (result < 0)
        error("Error connecting to the Server", 1);

    /* general declares and initializations */
    pthread_t rthread_id, sthread_id, pthread_id;
    av_packets_queue = create_fifo(FIFO_DEFAULT_CAPACITY);
    
    
    /* initializing the recv, send and play threads */
    int rc = pthread_create(&rthread_id, NULL, recv_thread, NULL);
    if(rc < 0)
        error("Error creating Receive thread\n", 1);

    rc = pthread_create(&sthread_id, NULL, send_thread, NULL);
    if(rc < 0)
        error("Error creating Send thread\n", 1);
    
    rc = pthread_create(&pthread_id, NULL, play_thread, NULL);
    if(rc < 0)
        error("Error creating Play thread\n", 1);


    pthread_join(rthread_id, NULL);
    pthread_join(sthread_id, NULL);
    
    /* TODO: error handling for all system calls */
    printf("Success\n");
    
    /* cleanup */
    sigint_handler(0);
    
    return (EXIT_SUCCESS);
}

