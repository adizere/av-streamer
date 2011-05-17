#include "audiovideo.h"


// Do any needed initialization here.
AVManager::AVManager (AVManagerMode mode) :
    filename (NULL), stream_info (NULL)
#if defined (__AvsServer__)
    , format_ctx (NULL), codec_ctx (NULL), codec (NULL)
#elif defined (__AvsClient__)
    , sws_ctx (NULL), screen(NULL), yuv_overlay (NULL)
#endif
{
    this->mode = mode;
    this->stream_info = (streaminfo *)malloc (sizeof (struct _streaminfo));
    // Register formats and codecs.
    av_register_all ();
}

#if defined (__AvsServer__)

bool AVManager::init_send (char *filename)
{
    int i = 0;
    int video_stream = 0;
    
    if (!filename || strlen (filename) < 3) {
        fprintf (stderr, "Error: Cannot open media file (invalid file name).\n");
        return false;
    }
    
    this->filename = (char *)malloc (strlen (filename) + 1);
    strcpy (this->filename, filename);
    
    if (av_open_input_file (&format_ctx, this->filename, NULL, 0, NULL) !=0) {
        fprintf (stderr, "Error: Cannot open media file.\n");
        free (this->filename);
        this->filename = NULL;
        return false;
    }
    
    // Retrieve stream information.
    if(av_find_stream_info (this->format_ctx) < 0) {
        fprintf (stderr, "Error: Could not find stream information.\n");
        return false;
    }
    
    // Dump information about file onto standard error.
    av_dump_format (format_ctx, 0, this->filename, 0);
    
    // Now format_ctx->streams is just an array of pointers,
    // of size format_ctx->nb_streams, so let's walk through it
    // until we find a video stream.
    video_stream = -1;
    for (i=0; i < format_ctx->nb_streams; i++)
        if (format_ctx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
        video_stream = i;
        break;
    }
    if (video_stream == -1) {
        fprintf (stderr, "Error: No video stream was found.\n");
        return false;
    }
    
    // Get a pointer to the codec context for the video stream.
    this->codec_ctx = format_ctx->streams[video_stream]->codec;
    

    // Find the decoder for the video stream
    this->codec = avcodec_find_decoder(codec_ctx->codec_id);
    if(this->codec == NULL) {
        fprintf (stderr, "Error: Unsupported codec.\n");
        return false;
    }
    
    // Open codec.
    if (avcodec_open (codec_ctx, codec) < 0) {
        fprintf (stderr, "Error: Could not open codec.\n");
        return false;
    }
}

/** The client needs this first, to setup a new codec context,
 *  so that it may render the frames received from the server. */
bool AVManager::get_stream_info (streaminfo *stream_info)
{
    if (this->codec_ctx == NULL || stream_info == NULL)
        return false;
    
    // Cache a local copy of this. Maybe not useful after all.
    this->stream_info->width   = this->codec_ctx->width;
    this->stream_info->height  = this->codec_ctx->height;
    this->stream_info->pix_fmt = this->codec_ctx->pix_fmt;
    
    stream_info->width   = this->codec_ctx->width;
    stream_info->height  = this->codec_ctx->height;
    stream_info->pix_fmt = this->codec_ctx->pix_fmt;
    return true;
}

/**
    \brief You must dealloc the packet after reading it from the file.
    @return 0 if OK, < 0 on error or end of file
*/
bool AVManager::read_packet_from_file (AVPacket *packet)
{
    int frame_finished = 0;
    
    if (!packet) {
        fprintf (stderr, "Error: AVPacket storage is NULL.\n");
        return false;
    }
    
    if (av_read_frame (this->format_ctx, packet) < 0) {
        // Error or end of file.
        return false;
    }

    return true;
}

void AVManager::end_send ()
{
    // Close the codec.
    avcodec_close(codec_ctx);

    // Close the video file
    av_close_input_file(format_ctx);
    
    // Freedom...
    free (this->filename);
    this->filename = NULL;
}

#elif defined (__AvsClient__)

/** Initialize stream with initial setup information from the server. */
bool AVManager::init_recv (streaminfo *stream_info)
{
    if (stream_info == NULL ||
        stream_info->height <= 0 ||
        stream_info->width <= 0)
    {
        fprintf (stderr, "Error: Invalid stream information.\n");
        return false;
    }
    
    if (this->sws_ctx) {
        fprintf (stderr, "Error: A SWS Context already exists. Initialization failed.\n");
        return false;
    }
    
    this->stream_info->width   = stream_info->width;
    this->stream_info->height  = stream_info->height;
    this->stream_info->pix_fmt = stream_info->pix_fmt;
    
    // Prepare a context for frame conversion.
    this->sws_ctx = sws_getContext (
        this->stream_info->width,
        this->stream_info->height,
        this->stream_info->pix_fmt,
        this->stream_info->width,
        this->stream_info->height,
        PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    if (this->sws_ctx == NULL) {
        fprintf (stderr, "Error: Could not create sws scaling context for image convertion.\n");
        goto CLEAN_AND_FAIL;
    }
    
    // Initialize SDL library.
    if(SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf (stderr, "Error: Could not initialize SDL (%s).\n", SDL_GetError());
        goto CLEAN_AND_FAIL;
    }
    
    this->screen = SDL_SetVideoMode (this->stream_info->width,
        this->stream_info->height, 0, 0);
    if (this->screen == NULL) {
        fprintf (stderr, "Error: SDL could not set video mode.\n");
        goto CLEAN_AND_FAIL;
    }
    
    this->yuv_overlay =
        SDL_CreateYUVOverlay (this->stream_info->width,
            this->stream_info->height, SDL_YV12_OVERLAY, screen);
    if (!this->yuv_overlay) {
        fprintf(stderr, "Error: SDL could not create YUV overlay.\n");
        goto CLEAN_AND_FAIL;
    }
    
    return true;
    
CLEAN_AND_FAIL:
    if (this->sws_ctx)
        sws_freeContext (this->sws_ctx);
    return false;
}

/** Play a frame received from the server. */
bool AVManager::play_frame (AVFrame *frame)
{
    AVPicture picture;
    SDL_Rect  rectangle;
    int       scale = 0;
    
    if (frame == NULL) {
        fprintf (stderr, "Error: Invalid AV frame.\n");
        return false;
    }
    
    SDL_LockYUVOverlay (this->yuv_overlay);
    picture.data[0] = this->yuv_overlay->pixels[0];
    picture.data[1] = this->yuv_overlay->pixels[2];
    picture.data[2] = this->yuv_overlay->pixels[1];
    
    picture.linesize[0] = this->yuv_overlay->pitches[0];
    picture.linesize[1] = this->yuv_overlay->pitches[2];
    picture.linesize[2] = this->yuv_overlay->pitches[1];
    
    scale = sws_scale (sws_ctx, frame->data, frame->linesize, 0,
        this->stream_info->height, picture.data, picture.linesize);
    
    printf ("Result of scale: %d bytes.\n", scale);
    
    SDL_UnlockYUVOverlay (this->yuv_overlay);
    
    rectangle.x = 0;
    rectangle.y = 0;
    rectangle.w = this->stream_info->width;
    rectangle.h = this->stream_info->height;
    
    SDL_DisplayYUVOverlay(this->yuv_overlay, &rectangle);
}

void AVManager::end_recv ()
{
    if (this->stream_info)
        free (this->stream_info);
    if (this->sws_ctx)
        sws_freeContext (sws_ctx);
}

#endif

void AVManager::dealloc_packet (AVPacket *packet)
{
    if (packet)
        av_free_packet (packet);
}

AVManager::~AVManager ()
{
    if (this->filename)
        free (this->filename);
    if (this->stream_info)
        free (this->stream_info);
#if defined (__AvsClient__)
    if (this->sws_ctx)
        sws_freeContext (sws_ctx);
#endif
}
