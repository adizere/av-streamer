/* 
 * File:   main.cpp
 * Author: megabyte
 *
 * Created on March 30, 2011, 10:02 PM
 */

#include "common.h"

/*
 * 
 */
int main(int argc, char** argv) {

    int socket_handle = socket(PF_INET, SOCK_DCCP, IPPROTO_DCCP);

    int on = 1;
    int result = setsockopt(socket_handle, SOL_DCCP, SO_REUSEADDR, (const char *) &on, sizeof(on));

    struct sockaddr_in conn_address;
    conn_address.sin_family = AF_INET;
    conn_address.sin_addr.s_addr = inet_addr(LOCAL_IP);
    conn_address.sin_port = htons(LOCAL_PORT);

    result = connect(socket_handle, (struct sockaddr *)&conn_address, sizeof(conn_address));

    
    
    

    /* TODO: error handling for all system calls */
    printf("Success\n");
    return (EXIT_SUCCESS);
}

