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

    pthread_t rthread_id, sthread_id;
    /* initializing the recv and send threads */
    rc = pthread_create(&rthread_id, NULL, recv_thread, (void*)dp);
    if(rc < 0)
        error("Error creating Receive thread\n", 1);

    rc = pthread_create(&sthread_id, NULL, send_thread, (void*)dp);
    if(rc < 0)
        error("Error creating Send thread\n", 1);


    pthread_join(rthread_id, NULL);
    pthread_join(sthread_id, NULL);

    close(client_sock);
    free(dp);


    LOCK(&server_mutex);
    thread_pool[pool_index].alive = false;
    UNLOCK(&server_mutex);

    return NULL;
}


/* Thread used to send audio video streams */
void* send_thread(void* data) {
    ch_data* dp = (ch_data*)data;
    
    int client_sock = dp->sock;
    const char* filename = dp->filename;

    char buff[512];
    FILE* finp = fopen(filename, "r");

    if(NULL == finp)
        error("Unable to open input file!", 1);

    while(fgets(buff, 5, finp)){
        int rc = send(client_sock, buff, strlen(buff)+1, 0);
        usleep(dp->st_rate);
    }
}


/* Thread used to receive feedback from client */
void* recv_thread(void* data) {
    ch_data* dp = (ch_data*)data;

    int client_sock = dp->sock;

    char buff[512];

    // add a method to determine when we should stop recv()-ing
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