/* 
 * File:   main.cpp
 * Author: megabyte
 *
 * Created on March 30, 2011, 10:01 PM
 */

#include "common.h"

int32_t g_sum;
int rv_sock;

void handler(int sig){
	close(rv_sock);
}


/*
 * 
 */
int main(int argc, char** argv)
{

    struct sockaddr_in addr, rem_addr;
    socklen_t len, rem_len;

    signal(SIGINT, handler);

    printf("Server started on port: %d\n", LOCAL_PORT);

    //----------------------------//
    //DCCP Server inititialization
    //get a socket handle just like TCP Server
    //RENDEZ-VOUS ?
    rv_sock = socket(PF_INET, SOCK_DCCP, IPPROTO_DCCP);
    if(rv_sock < 0)
        error("Socket init error!\n");


    //turn off bind address checking, and allow port numbers to be reused - otherwise
    //  the TIME_WAIT phenomenon will prevent  binding to these address.port combinations for (2 * MSL) seconds.
    int on = 1;
    int rc = setsockopt(rv_sock, SOL_DCCP, SO_REUSEADDR, (const char *) &on, sizeof(on));
    if(rc < 0)
        error("Error setting socket option!\n");

    addr.sin_family = PF_INET;
    addr.sin_port = htons( LOCAL_PORT );
    addr.sin_addr.s_addr = htonl( INADDR_ANY );


    //bind the socket to the local address and port. 
    rc = bind(rv_sock, (struct sockaddr *)&addr, sizeof(addr));
    if(rc < 0)
        error("Unable to bind!\n");

    //listen on that port for incoming connections
    rc = listen(rv_sock, MAX_DCCP_CONNECTION_BACK_LOG);
    if(rc < 0)
        error("Unable to set queue size!\n");

    printf("Waiting for client to connect...\n");

    //wait to accept a client connecting. When a client joins, mRemoteName and mRemoteLength are filled out by accept()
    int client_sock;
    rem_len = sizeof(rem_addr);
    client_sock = accept(rv_sock, (struct sockaddr *)&rem_addr, &rem_len);
    printf("Client connected: %s\n", inet_ntoa(rem_addr.sin_addr));

    g_sum = 10;

    int32_t msg;

    //receive msg from client
    rc = recv(client_sock, &msg, sizeof(int32_t), 0);
    if(rc < 0)
        error("Error on recv!\n");

    msg = ntohl(msg);
    printf("Received from client:%d\n", msg);

    g_sum += msg;

    char send_buff[100];
    sprintf(send_buff, "%d", g_sum);
    printf("Reply with:%s\n", send_buff);

    rc = send(client_sock, send_buff, strlen(send_buff)+1, 0);
    if(rc < 0)
        error("Error on send!\n");
    printf("Done;\n");

    close(client_sock);
    close(rv_sock);

    printf("Success\n");
    return (EXIT_SUCCESS);
}

