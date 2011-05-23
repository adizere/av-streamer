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

typedef struct _rtp_header rtp_header;
struct _rtp_header {
    /* version */
    uint16_t version;

    /* flags */
    uint8_t P;         /* padding */
    uint8_t X;         /* extension header presence */
    uint8_t CC;        /* Contributing sources count */
    uint8_t M;         /* Marker - if packet has a special relevance */
    uint8_t PT;        /* Payload type */

    /* sequence number - increasing for every individual packet */
    uint16_t seq;

    /* timestamp */
    uint32_t timestamp;

    /* SSRC identifier - source identifier */
    uint32_t ssrc;

} __attribute__((packed));

//TODO: redefined rtp_payload to the actual payload
typedef int rtp_payload;

typedef struct _rtp_packet rtp_packet;
struct _rtp_packet {
    rtp_header header;
    rtp_payload payload;
};


#define SSRC_DEFAULT_ID 25555       /* for ssrc field */
#define SEQUENCE_START 25555        /* sequence number shouldn't start at 0 */
#define RTP_VERSION 1               /* our first version */
#define MARKER_LAST_PACKET 2
#define MARKER_DEFAULT 1
#define PAYLOAD_DEFAULT 1


/* Used to instantiate a new rtp_packet with it's default values */
rtp_packet* create_packet();

#endif	/* _RTP_H */
