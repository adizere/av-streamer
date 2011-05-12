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


void sigint_handler(int signal) {
    printf("Client shutting down..\n");

    close(socket_handle);
    
    // close the player
    
    exit(EXIT_SUCCESS);
}


/* Thread used to receive incoming data streams */
void* recv_thread(void*)
{
    rtp_packet packet;
    packets_queue = create_fifo(FIFO_DEFAULT_CAPACITY);
    char buff[512];

    /* allocate space for payload if needed.. */
    //packet->payload = (packet_payload*)malloc(sizeof(packet_payload));

    // add a method to determine when we should stop recv()-ing
    while(1)
    {
        //int rec_size = recv(socket_handle, &packet, sizeof(rtp_packet), 0);
        //enqueue(packets_queue, packet);

        int rc = recv(socket_handle, buff, 512, 0);
        if (rc == 0)                                    /* peer has shutdown the socket */
        {
            printf("Server was shutdown.. \n");
            break;
        }
        printf("from server:%s\n",buff);
        fflush(stdout);
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
        sleep(5);
        printf("Sent..\n");
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


//    number = htonl(number);
//    do
//    {
//        printf(".");
//        status = send(socket_handle, &number, sizeof(int32_t), 0);
//    }
//    while((status<0)&&(errno == EAGAIN));

//    char receive_buff[100];
//    int rec_size = recv(socket_handle, receive_buff, 100, 0);

    pthread_join(rthread_id, NULL);
    pthread_join(sthread_id, NULL);
    
    /* TODO: error handling for all system calls */
    printf("Success\n");
    return (EXIT_SUCCESS);
}

