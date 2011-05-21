/* 
 * File:   main.cpp
 * Author: megabyte
 *
 * Created on March 30, 2011, 10:02 PM
 */

#include "globals.h"
#include "../common/common.h"


int socket_handle;      /* global socket */

fifo* packets_queue;
pthread_mutex_t p_queue_mutex = PTHREAD_MUTEX_INITIALIZER;


void sigint_handler(int signal) {
    printf("Client shutting down..\n");

    close(socket_handle);
    
    // close the player
    
    exit(EXIT_SUCCESS);
}


/* Thread used to receive incoming data streams */
void* recv_thread(void*)
{
    fifo_elem fe;
    rtp_packet* packet = (rtp_packet*)&fe;

    packets_queue = create_fifo(FIFO_DEFAULT_CAPACITY);

    // add a method to determine when we should stop recv()-ing
    while(1)
    {
        int rc = recv(socket_handle, &fe, sizeof(fifo_elem), 0);
        if (rc == 0)                                    /* peer has shutdown the socket */
        {
            printf("Server was shutdown.. \n");
            break;
        }

        LOCK(&p_queue_mutex);

        rc = enqueue(packets_queue, fe);//TODO handle this return code

        UNLOCK(&p_queue_mutex);

        // dummy code begin
        // TODO move this code in a new thread that dequeues packets and displays the video
        printf("Packet: seq:%d payload:%d\n",packet->header.seq, packet->payload);
        fflush(stdout);

        // dummy code end
    }
    
    destroy_fifo(packets_queue);
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

    pthread_t rthread_id, sthread_id;
    /* initializing the recv and send threads */
    int rc = pthread_create(&rthread_id, NULL, recv_thread, NULL);
    if(rc < 0)
        error("Error creating Receive thread\n", 1);

    rc = pthread_create(&sthread_id, NULL, send_thread, NULL);
    if(rc < 0)
        error("Error creating Send thread\n", 1);


    pthread_join(rthread_id, NULL);
    pthread_join(sthread_id, NULL);
    
    /* TODO: error handling for all system calls */
    printf("Success\n");
    return (EXIT_SUCCESS);
}

