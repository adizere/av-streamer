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
 */

#ifndef _AUDIOVIDEO_H
#define	_AUDIOVIDEO_H


#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <SDL/SDL.h>


///
/// \brief Per client audio-video manager. This class grabs data from disk
///        and sends it on its way to the client.
///
class AVManager
{
private:
    char *filename;                     ///< Name of the file that is currently processed from disk.
    AVFormatContext *format_ctx;        ///< Media file handle and context.
    AVCodecContext *codec_ctx;          ///< Codec context.
    AVCodec *codec;
    AVFrame *frame;
    int video_stream;
public:
    AVManager ();
    bool open_file (char *filename);
    bool init_grabbing ();
    void close_file ();
    ~AVManager ();
};

#endif	/* _AUDIOVIDEO_H */