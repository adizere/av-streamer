/*
 * File:   main.cpp
 * Author: megabyte
 *
 * Created on March 30, 2011, 10:01 PM
 */

#include "../common/common.h"
#include "globals.h"
#include "clhandler.h"


/* Custom handle for SIGINT
 */
void shutdown_server(int sig)
{
    printf("\n\tServer shutting down..\n");
    
    // TODO add clean-up code here

    // TODO stop client handlers, destroy queues, destroy mutexes
    
    int i;
    for(i = 0; i < MAX_DCCP_CONNECTION_BACK_LOG; ++i)
        if(thread_pool[i].alive)
        {
            pthread_join(thread_pool[i].tid, NULL);
            thread_pool[i].alive = false;
        }
    close(rv_sock);
}


/* Handles the command line arguments:
 *  - name of the file to be sent: string
 *  - server transmission rate: integer
 */
void handle_cmd_args(int argc, char** argv, char** file, useconds_t* st_rate)
{
    if (argc < 3)
        error("Please specify the name of the file and the transmission rate as parameters", 1);
    
    if(strlen(argv[1]) <= 0)
        error("Empty file name", 1);
    
    *file = (char*)malloc(strlen(argv[1]) + 1);
    strncpy(*file, argv[1], strlen(argv[1]));

    *st_rate = atoi(argv[2]);
    
    /* warning.. could be annoying so comment it.. */
//    if (*st_rate < 5000 || *st_rate > 20000)
//        printf("Transmission rate set to: %d; recommended value: (5.000 [~200 packets/second] - 20.000 [~50 packets/second])\n", *st_rate);

}


///
/// \brief In order to test this, you'll have to set __AvsClient__ and __AvsServer__ defines.
///
int audiovideo_api_test()
{
    // Server side initialization.

    AVManager mgr_server (AVSenderMode);
    streaminfo si;

    if (mgr_server.init_send ("/home/hani/nature.avi") == false)
        return -1;
    
    if (mgr_server.get_stream_info (&si) == false)
        return -1;

    // Client side initialization with data received from server.

    AVManager mgr_client (AVReceiverMode);
    streaminfo si_client = {0};
    AVMediaPacket *media_packet = NULL;
    si_client = si;
    mgr_client.init_recv (&si);


    // On each packet send over the wire, we must add a flag
    // stating that this is a either an audio or a video packet.
    
    while (mgr_server.read_packet_from_file (&media_packet))
    {
        if (media_packet->packet_type == AVPacketVideoType)
            mgr_client.play_video_packet (media_packet);
        // experimental audio playing.
        if (media_packet->packet_type == AVPacketAudioType)
            mgr_client.play_audio_packet (media_packet);
        mgr_client.free_packet (media_packet);
        media_packet = NULL;
    }

    // Cleanup

    mgr_server.end_send ();
    mgr_client.end_recv ();

    return 0;
}


/*
 * Server entry point
 */
int main(int argc, char* argv[])
{
    //return audiovideo_api_test();

    char* file;
    useconds_t st_rate;
    handle_cmd_args(argc, argv, &file, &st_rate);

    struct sockaddr_in addr, rem_addr;
    socklen_t len, rem_len;

    signal(SIGINT, shutdown_server);

    printf("Server started on port: %d\n", LOCAL_PORT);

    /* DCCP Server inititialization
     * This is the rendez-vous socket
     */
    rv_sock = socket(PF_INET, SOCK_DCCP, IPPROTO_DCCP);
    if(rv_sock < 0)
        error("Socket init error!\n");


    /* Turn off bind address checking, and allow port numbers to be reused - otherwise
     * the TIME_WAIT phenomenon will prevent  binding to these address.port combinations for (2 * MSL) seconds.
     */
    int on = 1;
    int rc = setsockopt(rv_sock, SOL_DCCP, SO_REUSEADDR, (const char *) &on, sizeof(on));
    int i, index;
    if(rc < 0)
        error("Error setting socket option!\n");

    addr.sin_family = PF_INET;
    addr.sin_port = htons(LOCAL_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);


    /* Bind the socket to the local address and port. */
    rc = bind(rv_sock, (struct sockaddr *)&addr, sizeof(addr));
    if(rc < 0)
        error("Unable to bind!\n");

    /* Listen on that port for incoming connections */
    rc = listen(rv_sock, MAX_DCCP_CONNECTION_BACK_LOG);
    if(rc < 0)
        error("Unable to set queue size!\n");

    printf("Waiting for client to connect...\n");

    /* Wait to accept a client connecting. When a client joins, mRemoteName and mRemoteLength are filled out by accept() */
    int client_sock;
    int tid;

    while(true)
    {
        index = -1;
        
        /* Find a free slot for the next client */
        for(i = 0; i < MAX_DCCP_CONNECTION_BACK_LOG; ++i)
            if(thread_pool[i].alive == false)
            {
                index = i;
                break;
            }
        if(index < 0)
            error("Server busy: no free slots!\n", 1);

        rem_len = sizeof(rem_addr);
        client_sock = accept(rv_sock, (struct sockaddr *)&rem_addr, &rem_len);
        if(client_sock < 0)
            error("Socket accept() error!\n", 1);
        

        /* 
         * Initialize client handler data and client media transfer/codec information. */
        ch_data* dp = (ch_data*) malloc(sizeof(ch_data));
        
        dp->sock = client_sock;
        dp->pool_index = index;
        dp->filename = file;
        dp->st_rate = st_rate;
        dp->rem_addr = rem_addr;
        dp->private_fifo = create_fifo(FIFO_DEFAULT_CAPACITY);
        
        // Initialize audio-video stream information.
        
        dp->avmanager = new AVManager (AVSenderMode);
        memset ((void *)&dp->stream_info, 0, sizeof (streaminfo));
        
        pthread_mutex_t* pfifo_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
        
        rc = pthread_mutex_init(pfifo_mutex, NULL);
        if(rc < 0)
            error("Unable to create the mutex for client handler fifo!", 1);
        dp->p_fifo_mutex = pfifo_mutex;

        
        /* 
         * Now create the thread which will manage everythin related to this client */
        rc = pthread_create(&(thread_pool[index].tid), NULL, ch_thread, (void*)dp);
        if(rc < 0)
            error("Unable to create thread!\n", 1);

    }

    for(i = 0; i < MAX_DCCP_CONNECTION_BACK_LOG; ++i)
    {
        LOCK(&server_mutex);
        if(thread_pool[i].alive)
        {
            pthread_join(thread_pool[i].tid, NULL);
            thread_pool[i].alive = false;
        }
        UNLOCK(&server_mutex);
    }

    close(rv_sock);

    printf("Success\n");
    return (EXIT_SUCCESS);
}