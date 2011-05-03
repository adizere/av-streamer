/* 
 * File:   audiovideo.h
 * Author: hani
 *
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
 */

#ifndef _AUDIOVIDEO_H
#define	_AUDIOVIDEO_H


#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif


#include <stdio.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include <SDL/SDL.h>


///
/// \todo We could share this code between AvsServer and AvsClient by means
///       of some defines, to eliminate server code from AvsClient component.
///
enum AVManagerMode
{
    AVSenderMode,       ///< Server sender mode.
    AVReceiverMode      ///< Client receiver mode.
};


/** Codec initialization information
 *  send over the network to the client. */
typedef struct _streaminfo
{
    int width;
    int height;
    PixelFormat pix_fmt;
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
#if defined (__AvsServer__)
    AVFormatContext *format_ctx;        ///< Media file handle and context.
    AVCodecContext *codec_ctx;          ///< Codec context.
    AVCodec *codec;
    AVPacket packet;
    int video_stream;
#elif defined (__AvsClient__)
    struct SwsContext *sws_ctx;
    SDL_Surface *screen;
    SDL_Overlay *yuv_overlay;
#endif
    
public:
    AVManager (AVManagerMode mode);
#if defined (__AvsServer__)
    bool init_send (char *filename);
    bool get_stream_info (streaminfo *stream_info);
    int read_frame_from_file (AVFrame *frame);
    void end_send ();
#elif defined (__AvsClient__)
    bool init_recv (streaminfo *stream_info);
    bool play_frame (AVFrame *frame);
    void end_recv ();
#else
#warning "Why do you need AVManager after all?"
#endif
    static AVFrame * alloc_frame ();
    static void free_frame (AVFrame *frame);
    ~AVManager ();
};


#endif	/* _AUDIOVIDEO_H */