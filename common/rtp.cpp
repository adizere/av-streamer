#include "rtp.h"


/* Used to instantiate a new rtp_packet with it's default values */
rtp_packet* create_packet()
{
    rtp_packet* ret = (rtp_packet*)malloc(sizeof(rtp_packet));
    
    rtp_header* ret_header = (rtp_header*)malloc(sizeof(rtp_header));
    
    ret->header = *ret_header;
    
    /* default values initialization */
    ret->header.ssrc = SSRC_DEFAULT_ID;
    ret->header.version = RTP_VERSION;
    ret->header.M = MARKER_ALONE;
    ret->header.PT = PAYLOAD_TYPE_DEFAULT;
    
    return ret;
}
