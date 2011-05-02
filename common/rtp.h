/* 
 * File:   rtp.h
 * Author: adizere
 *
 * Created on March 30, 2011, 10:55 PM
 */

#ifndef _RTP_H
#define	_RTP_H


typedef struct _rtp_header rtp_header;
struct _rtp_header {
    /* version */
    u_int16_t version;

    /* flags */
    u_int8_t P;         /* padding */
    u_int8_t X;         /* extension header presence */
    u_int8_t CC;        /* Contributing sources count */
    u_int8_t M;         /* Marker - if packet has a special relevance */
    u_int8_t PT;        /* Payload type */

    /* sequence number - increasing for every individual packet */
    u_int16_t seq;

    /* timestamp */
    u_int32_t timestamp;

    /* SSRC identifier - source identifier */
    u_int32_t ssrc;

} __attribute__((packed));

typedef int rtp_payload;

typedef struct _rtp_packet rtp_packet;
struct _rtp_packet {
    rtp_header header;
    rtp_payload payload;
};


#define SSRC_DEFAULT_ID 25555       /* for ssrc field */
#define SEQUENCE_START 25555        /* sequence number shouldn't start at 0 */
#define RTP_VERSION 1
#define PAYLOAD_TYPE 1


#endif	/* _RTP_H */