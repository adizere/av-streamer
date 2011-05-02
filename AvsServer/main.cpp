/* 
 * File:   main.cpp
 * Author: megabyte
 *
 * Created on March 30, 2011, 10:01 PM
 */

#include "common.h"
#include "globals.h"
#include "clhandler.h"

void shutdown_server(int sig){

    //TODO add clean-up code here
    int i;
    for(i = 0; i < MAX_DCCP_CONNECTION_BACK_LOG; ++i)
        if(thread_pool[i].alive)
        {
            pthread_join(thread_pool[i].tid, NULL);
            thread_pool[i].alive = false;
        }
    close(rv_sock);
}


/*
 * 
 */
int main(int argc, char** argv)
{

    struct sockaddr_in addr, rem_addr;
    socklen_t len, rem_len;

    signal(SIGINT, shutdown_server);

    printf("Server started on port: %d\n", LOCAL_PORT);
    g_sum = 0;

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
    int i, index;
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
    int tid;

    while(true)
    {
        index = -1; //find a free slot for the next client
        for(i = 0; i < MAX_DCCP_CONNECTION_BACK_LOG; ++i)
            if(thread_pool[i].alive == false)
            {
                index = i;
                break;
            }
        if(index < 0)
            error("server busy: no free slots!\n", 1);

        rem_len = sizeof(rem_addr);
        client_sock = accept(rv_sock, (struct sockaddr *)&rem_addr, &rem_len);
        if(client_sock < 0)
            error("accept error!\n", 1);

        ch_data* dp = (ch_data*) malloc(sizeof(ch_data));

        dp->sock = client_sock;
        dp->pool_index = index;
        dp->filename = argv[1];
        dp->rem_addr = rem_addr;

        rc = pthread_create(&(thread_pool[index].tid), NULL, ch_thread, (void*)dp);
        if(rc < 0)
            error("Unable to create thread!\n", 1);

    }

     for(i = 0; i < MAX_DCCP_CONNECTION_BACK_LOG; ++i)
        if(thread_pool[i].alive)//TODO syncronize
        {
            pthread_join(thread_pool[i].tid, NULL);
            thread_pool[i].alive = false;
        }
    close(rv_sock);

    printf("Success\n");
    return (EXIT_SUCCESS);
}

