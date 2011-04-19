/* 
 * File:   main.cpp
 * Author: megabyte
 *
 * Created on March 30, 2011, 10:01 PM
 */

#include "common.h"

int32_t g_sum;
int rv_sock;

// data about the handler threads
typedef struct
{
    pthread_t tid;
    bool alive;
} client_thread;

client_thread thread_pool[MAX_DCCP_CONNECTION_BACK_LOG];

// client handler data
typedef struct
{
    int sock;
    int pool_index;
    const char* filename;
    struct sockaddr_in rem_addr;
} ch_data; 

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



// client handler thread
void* ch_thread(void* data)
{
    ch_data* dp = (ch_data*)data;

    int client_sock = dp->sock;
    int pool_index = dp->pool_index;
    const char* filename = dp->filename;
    struct sockaddr_in rem_addr = dp->rem_addr;

    int rc;

    printf("Client connected: %s\n", inet_ntoa(rem_addr.sin_addr));

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
    free(dp);

    //TODO SYNCRONIZE THREAD POOL
    thread_pool[pool_index].alive = false;
    return NULL;
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

