/* 
 * File:   main.cpp
 * Author: megabyte
 *
 * Created on March 30, 2011, 10:02 PM
 */

#include "globals.h"
#include "../common/common.h"


AVManager avmanager (AVReceiverMode);
int socket_handle;      /* global socket */

fifo* av_packets_queue;
pthread_mutex_t p_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

/* This flag is marked when the recv_thread has finished receiving from the server
 * In this way, we know when we should stop trying to dequeue from av_packets_queue
 */
int flag_transmission_finished = 0;


void sigint_handler(int signal) {
    printf("Client shutting down..\n");

    close(socket_handle);
    
    // close the player
    
    
    destroy_fifo(av_packets_queue);
    
    exit(EXIT_SUCCESS);
}


/* Thread used to receive incoming data streams */
void* recv_thread(void*)
{
    /* use the same packet in every recv() */
    rtp_packet* packet = create_packet();
    
    /* some necessary variables */
    int last_PUC_seq = 0;
    int last_timestamp = TIMESTAMP_START - 1;
    fifo_elem PUC; /* Package Under Construction */
    int puc_total_payload_size  = 0;    // Actually the size of the entire current media_packet (all fragments put together).
    int puc_done_size           = 0;    // Bytes successfully copyied to the PUC from the total amount.
    int puc_fragment_size       = 0;    // Current PUC fragment size.
    AVMediaPacket *media_packet = NULL; // A fifo_elem is actually an AVMediaPacket *.
    
    fd_set fds;

    
    /* now receive data */
    while(1)
    {
        memset(packet, 0, sizeof(rtp_packet));
        
        /* select() syscall area */
        FD_ZERO(&fds);
        FD_SET(socket_handle, &fds);
        struct timeval *t, time = {CLIENT_RECV_TIMEOUT, 0};
        t = &time;
        
        int er = select(socket_handle + 1, &fds, NULL, NULL, t);
        if (er == 0)
            break;
        else if (er < 0) {                      /** er < 0 an error ocurred during select() **/
            error("Error @ select() call in reliableSend()\n", 1);
            break;
        }
        /* END select() syscall area */
        
        int rc = recv(socket_handle, packet, sizeof(rtp_packet), 0);
        if (rc == 0)  /* peer has shutdown the socket */
        {
            printf("Server was shutdown.. \n");
            break;
        }
        
        /* 
         * Now cometh the RTP protocol */
        if (packet->header.timestamp <= last_timestamp) {
            /* dropping the packet */
            continue;
        }
        
        if (packet->header.M == MARKER_FIRST) {
            /* this is the first fragment from a bigger packet */

            // At the beginning we will surely have the entire AVMediaPacket header + some data.
            // A little hack: we get how many bytes are we expecting to receive
            // in order to fully construct our PUC.
            
            media_packet = (AVMediaPacket *)packet->payload;
            puc_total_payload_size = media_packet->entire_media_packet_size;
            
            if (puc_total_payload_size >= sizeof(rtp_payload))
                puc_fragment_size = sizeof(rtp_payload);
            else
                puc_fragment_size = puc_total_payload_size;

            puc_done_size = puc_fragment_size;
            PUC = (fifo_elem)malloc (media_packet->entire_media_packet_size);
            if (PUC == NULL) {
                error ("Not enough memory to allocate that much of a buffer.\n");
                break;
            }
            memset ((void *)PUC, 0, media_packet->entire_media_packet_size);
            memcpy ((void *)PUC, (void *)(packet->payload), puc_fragment_size);
            
            last_PUC_seq = packet->header.seq;

            continue;
        }
        
        if (packet->header.M == MARKER_ALONE) {
            /* this is the whole packet, no fragmentation was necessary */
            // Copy bytes from network buffer to fifo_elem queue node.
            
            media_packet = (AVMediaPacket *)packet->payload;
            puc_total_payload_size = media_packet->entire_media_packet_size;
            puc_fragment_size = puc_total_payload_size;
            fifo_elem fe = (fifo_elem)malloc (media_packet->entire_media_packet_size);
            if (fe == NULL) {
                error ("Not enough memory to allocate that much of a buffer.\n");
                break;            
            }
            memcpy((void *)fe, (void *)media_packet, media_packet->entire_media_packet_size);
            
            LOCK(&p_queue_mutex);

            rc = enqueue(av_packets_queue, fe);
            if (rc < 0)
                error("The queue is full - adjust the FIFO_DEFAULT_CAPACITY to a bigger value. Quitting!", 1);

            UNLOCK(&p_queue_mutex);
            
            // Do not free the fifo_elem. It's not yours!
            // The enqueue is done by reference copy only.

            continue;
        }
        
        if (packet->header.seq - last_PUC_seq == 1) {
            /* this is a completing fragment for the packet in PUC */
            
            if (puc_done_size + sizeof (rtp_payload) > puc_total_payload_size)
                puc_fragment_size = puc_total_payload_size - puc_done_size;
            else
                puc_fragment_size = sizeof (rtp_payload);

            // put in PUC packet->payload
            uint8_t* dest_ptr = (uint8_t*)PUC;
            dest_ptr         += puc_done_size;
            memcpy( dest_ptr, (void*) (packet->payload), puc_fragment_size);
            puc_done_size    += puc_fragment_size;
            
            if (packet->header.M == MARKER_LAST) {
                /* this is the last fragment of the packet from PUC */
                
                // Development: Small assertion just to check for now
                // that we've got the entire packet on this side everytime right.
                if (puc_done_size != puc_total_payload_size)
                    error ("Incomplete fifo_elem.\n");

                LOCK(&p_queue_mutex);
                
                rc = enqueue(av_packets_queue, PUC);
                if (rc < 0)
                    error("The queue is full - adjust the FIFO_DEFAULT_CAPACITY to a bigger value. Quitting!", 1);
                
                UNLOCK(&p_queue_mutex);
                
                last_timestamp = packet->header.timestamp;
            } else {
                /* not the last fragment.. more needed */
                
                last_PUC_seq = packet->header.seq;
            }
        }
    }
    sleep(1);   /* some buffer sleep() between recv-ing and playing */
    printf("Transmission from server finished.\n");
    flag_transmission_finished = 1;
}


/* Thread used to send feedback to the server */
void* send_thread(void*)
{

    int count = 0;
    char buff[512];
    while(flag_transmission_finished == 0)
    {
        sprintf(buff, "dummy feedback %d", count++);
        int rc = send(socket_handle, buff, strlen(buff)+1, 0);
        if (rc < 0)
            break;
        sleep(5);
        printf("Client feedback sent: %d bytes\n", rc);
    }

}


/* Take packets from the audio and video queues */
void* play_thread(void*)
{
    int rc;
    fifo_elem fe;
    AVMediaPacket *media_packet = NULL;
    streaminfo si = {0};
    
    printf("Play thread: \n");

    printf("Retrieving stream information from server...\n");

    do {
        LOCK (&p_queue_mutex);
        rc = dequeue (av_packets_queue, &fe);    
        UNLOCK (&p_queue_mutex);
        usleep (1000);
    } while (rc < 0); 

    if (rc < 0) {
        DBGPRINT ("Error: Unable to retrieve stream information from server.");
        return NULL;
    }

    media_packet = (AVMediaPacket *)fe;
    if (media_packet == NULL || media_packet->packet_type != AVPacketStreamInfoType ||
        AVManager::packet_to_stream_info (media_packet, &si) == false)
    {
        DBGPRINT ("Retrieved invalid stream information from packet queue.");
        AVManager::free_packet (media_packet);
        return NULL;
    }

    if (avmanager.init_recv (&si) == false) {
        DBGPRINT ("Unable to initialize audio-video manager.");
        return NULL;
    }

    AVManager::free_packet (media_packet);
    media_packet = NULL;
    


    while(1)
    {
        LOCK (&p_queue_mutex);
        rc = dequeue (av_packets_queue, &fe);
        UNLOCK (&p_queue_mutex);
        
        if (rc < 0) {
            if (flag_transmission_finished == 1)
                break;
            usleep(CLIENT_FIFO_POLL_TIMEOUT);
            continue;
        }

        media_packet = (AVMediaPacket *)fe;
        if (media_packet == NULL) {
            DBGPRINT ("[play_thread] invalid media packet retrieved from media stream.");
            AVManager::free_packet (media_packet);
            continue;
        }
        
//        printf ("Playing media packet: {pts=%ld}\n", media_packet->packet.pts);
        
        if (media_packet->packet_type == AVPacketVideoType)
            avmanager.play_video_packet (media_packet);
        // experimental audio playing.
        if (media_packet->packet_type == AVPacketAudioType)
            avmanager.play_audio_packet (media_packet);
        AVManager::free_packet (media_packet);
        
/*
        Dummy :p code. That's why we don't use it.
        
        char msg[5];
        memcpy(msg, &fe, sizeof(fe));
        msg[4] = 0;
        
        printf("%s", msg);
*/
    }

    avmanager.end_recv();
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

    /* general declares and initializations */
    pthread_t rthread_id, sthread_id, pthread_id;
    av_packets_queue = create_fifo(FIFO_DEFAULT_CAPACITY);
    
    
    /* initializing the recv, send and play threads */
    int rc = pthread_create(&rthread_id, NULL, recv_thread, NULL);
    if(rc < 0)
        error("Error creating Receive thread\n", 1);

    rc = pthread_create(&sthread_id, NULL, send_thread, NULL);
    if(rc < 0)
        error("Error creating Send thread\n", 1);
    
    rc = pthread_create(&pthread_id, NULL, play_thread, NULL);
    if(rc < 0)
        error("Error creating Play thread\n", 1);


    pthread_join(rthread_id, NULL);
    pthread_join(sthread_id, NULL);
    
    printf("Success\n");
    
    /* cleanup */
    sigint_handler(0);
    
    return (EXIT_SUCCESS);
}