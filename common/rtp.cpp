#include "rtp.h"


/* Used to instantiate a new rtp_packet with it's default values */
rtp_packet* create_packet()
{
    rtp_packet* ret;
    
    rtp_header* ret_header = (rtp_header*)malloc(sizeof(rtp_header));
    rtp_payload* ret_payload = (rtp_payload*)malloc(sizeof(rtp_payload));
    
    ret->header = *ret_header;
    ret->payload = *ret_payload;
    
    /* default values initialization */
    ret->header.ssrc = SSRC_DEFAULT_ID;
    ret->header.version = RTP_VERSION;
    ret->header.M = MARKER_DEFAULT;
    ret->header.PT = PAYLOAD_DEFAULT;
    
    return ret;
}
