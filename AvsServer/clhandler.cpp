#include "clhandler.h"

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
