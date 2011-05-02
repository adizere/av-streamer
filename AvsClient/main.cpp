/* 
 * File:   main.cpp
 * Author: megabyte
 *
 * Created on March 30, 2011, 10:02 PM
 */

#include "common.h"

/* global socket */
int socket_handle;


void sigint_handler(int signal) {
    printf("Client shutting down..\n");

    // close the player
    
    exit(EXIT_SUCCESS);
}


/* Thread used to receive incoming data streams */
void* recv_thread(void* arg) {
    rtp_packet* packet = (rtp_packet*)malloc(sizeof(rtp_packet));

    /* allocate space for payload if needed.. */
    //packet->payload = (packet_payload*)malloc(sizeof(packet_payload));

    int rec_size = recv(socket_handle, packet, sizeof(rtp_packet), 0);

    
}

/* Thread used to send feedback to the server */
void* send_thread(void* arg) {

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

    pthread_t *rthread_id, *sthread_id;
    /* initializing the recv and send threads */
    int rc = pthread_create(rthread_id, NULL, recv_thread, NULL);
    if(rc < 0)
        error("Error creating Receive thread\n", 1);

    rc = pthread_create(sthread_id, NULL, send_thread, NULL);
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

    

    /* TODO: error handling for all system calls */
    printf("Success\n");
    return (EXIT_SUCCESS);
}

