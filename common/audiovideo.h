/*
 * Created on May 3, 2011, 10:02 AM
 * 
 * =============================================================================
 * SDL
 * 
 * In order to compile and use this module, you will need SDL multimedia library.
 * sudo apt-get install libsdl1.2-dev
 * 
 * After installation, your project include path should contain sources from:
 * /usr/include/SDL
 * 
 * =============================================================================
 * FFmpeg
 * 
 * 1. Download from http://www.ffmpeg.org/download.html
 * 2. Build: configure, make, make install
 * 3. Includes are taken from: /usr/local/include
 * 
 * =============================================================================
 * 
 * Libraries: -lavformat -lavcodec -lavutil -lswscale -lz -lm -lSDL
 * 
 * =============================================================================
 *
 * You may want to check http://www.earth-touch.com/ for some free audio-video
 * nature footages, excellent for testing this aplication.
 *
 * =============================================================================
 * 
 */

#ifndef _AUDIOVIDEO_H
#define	_AUDIOVIDEO_H


#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif


#include <stdio.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include <SDL/SDL.h>


#define SDL_AUDIO_BUFFER_SIZE 1024


///
/// \todo We could share this code between AvsServer and AvsClient by means
///       of some defines, to eliminate server code from AvsClient component.
///
enum AVManagerMode
{
    AVSenderMode,       ///< Server sender mode.
    AVReceiverMode      ///< Client receiver mode.
};


///
/// \brief Types of AVPackage's.
///
enum AVMediaPacketType
{
    AVPacketVideoType,
    AVPacketAudioType,
    AVPacketUnknownType
};


///
/// \brief Header for AVPackage with some additional media information.
///
typedef struct _AVMediaPacket
{
    int entire_media_packet_size;           ///< Size of entire media packet (header + data).
    AVMediaPacketType packet_type;
    AVPacket packet;
    int data_size;                          ///< Size of data from media packet (just data).
                                            ///< Actually duplicated from packet field.
    // Actuall media data follows right here...
} __attribute__((packed)) AVMediaPacket;


#if defined (__AvsClient__)

/// \brief Audio packets queue list. The mutex and condition are because SDL
///        is running the audio process as a separate thread.
///        If we don't lock the queue properly, data could get compromised.
class PacketQueue
{
private:
    AVPacketList *first_pkt, *last_pkt; ///< Linked list storing audio packets.
    int nb_packets;                     
    int size;                           ///< This we get from packet->size.
    SDL_mutex *mutex;
    SDL_cond *cond;

public:
    PacketQueue ();
    bool init ();
    bool put (AVPacket *packet);
    int get (AVPacket *packet, int block);
    void set_quit ();
    void close ();
    ~PacketQueue ();

private:
    void cleanup ();
    friend int decode_interrupt_cb (void);
};

#endif /* defined (__AvsClient__) */


/// Codec initialization information
/// send over the network to the client.
typedef struct _streaminfo
{
    int width;
    int height;
    PixelFormat pix_fmt;
    CodecID codec_id;           ///< Video codec ID.
    CodecID audio_codec_id;     ///< Audio codec ID.
    int freq;                   ///< Audio frequency in samples per second (sample rate).
    Uint8 channels;             ///< Number of channels: 1 mono, 2 stereo.
} __attribute__((packed)) streaminfo ;


///
/// \brief Per client audio-video manager on both sides of the transmition.
///
class AVManager
{
private:
    char *filename;                     ///< Name of the file that is currently processed from disk.
    AVManagerMode mode;                 ///< What are we doing here?
    streaminfo *stream_info;            ///< Information about the current stream;
    int video_stream_index;
    int audio_stream_index;
    AVCodecContext *codec_ctx;          ///< Video codec context.
    AVCodec *codec;                     ///< Video codec.
    AVCodecContext *audio_codec_ctx;    ///< Audio codec context.
    AVCodec *audio_codec;               ///< Audio codec.
#if defined (__AvsServer__)
    AVFormatContext *format_ctx;        ///< Media file handle and context.
#endif /*defined (__AvsServer__) */
#if defined (__AvsClient__)
    struct SwsContext *sws_ctx;
    SDL_Surface *screen;
    SDL_Overlay *yuv_overlay;
    SDL_AudioSpec wanted_audio_spec;    ///< Desired audio specification (what we wish to get).
    SDL_AudioSpec obtained_audio_spec;  ///< Actual obtained audio specification (what we actually get).
    PacketQueue audio_packet_queue;     ///< Audio packet queue.
#endif /* defined (__AvsClient__) */

public:
    AVManager (AVManagerMode mode);
#if defined (__AvsServer__)
    bool init_send (char *filename);
    bool get_stream_info (streaminfo *stream_info);
    bool read_packet_from_file (AVMediaPacket **media_packet);
    void end_send ();
#endif /*defined (__AvsServer__) */
#if defined (__AvsClient__)
    bool init_recv (streaminfo *stream_info);
    bool play_video_packet (AVMediaPacket *media_packet);
    bool play_audio_packet (AVMediaPacket *media_packet);
    void end_recv ();
#endif /* defined (__AvsClient__) */
#if !defined (__AvsServer__) && !defined (__AvsClient__)
#error "Why do you need AVManager after all?"
#endif
    static void free_packet (AVMediaPacket *media_packet);
    ~AVManager ();
};


#endif	/* _AUDIOVIDEO_H */