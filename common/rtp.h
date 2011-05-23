/* 
 * File:   rtp.h
 * Author: adizere
 *
 * Created on March 30, 2011, 10:55 PM
 */

#ifndef _RTP_H
#define	_RTP_H

#include <stdint.h>
#include <stdlib.h>

#define PAYLOAD_MAX_SIZE 1024

typedef struct _rtp_header rtp_header;
struct _rtp_header {
    /* version */
    uint16_t version;

    /* flags */
    uint8_t P;          /* Padding */
    uint8_t X;          /* Extension header presence */
    uint8_t CC;         /* Contributing sources count */
    uint8_t M;          /* Marker - if packet has a special relevance */
    uint8_t PT;         /* Payload type */

    /* sequence number - increasing for every RTP Packet */
    uint16_t seq;

    /* timestamp - increasing for each AVMediaPacket */
    uint32_t timestamp;

    /* SSRC identifier - source identifier */
    uint32_t ssrc;

} __attribute__((packed));


typedef char rtp_payload[PAYLOAD_MAX_SIZE];

typedef struct _rtp_packet rtp_packet;
struct _rtp_packet {
    rtp_header header;
    rtp_payload payload;
};


#define SSRC_DEFAULT_ID 25555       /* for ssrc field */
#define SEQUENCE_START 25555        /* sequence number shouldn't start at 0 */
#define RTP_VERSION 1               /* our first version */
#define MARKER_LAST_PACKET 2

/* 
 * Definitions for the Marker flag types follows */
#define MARKER_ALONE 0          /* The packet is not fragmented*/
#define MARKER_FIRST 1          /* The first fragment of a fragmented packet */
#define MARKER_LAST 2           /* The last fragment of a fragmented packet */
#define MARKER_FRAGMENT 3       /* The contained fragment of a fragmented packet */

#define PAYLOAD_TYPE_DEFAULT 1


/* Used to instantiate a new rtp_packet with it's default values */
rtp_packet* create_packet();

#endif	/* _RTP_H */
