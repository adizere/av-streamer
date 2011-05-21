#include "audiovideo.h"


#if defined (__AvsClient__)

int decode_interrupt_cb (void);
void audio_callback (void *userdata, Uint8 *stream, int len);
int audio_decode_frame (AVCodecContext *audio_codec_ctx, uint8_t *audio_buf, int buf_size);

static PacketQueue *local_audio_packet_queue = NULL;
static int quit = 0;

PacketQueue::PacketQueue ()
    : first_pkt (NULL), last_pkt (NULL), nb_packets (0),
    size (0), mutex (NULL), cond (NULL)
{
    local_audio_packet_queue = this;
}


bool PacketQueue::init ()
{
    this->mutex = SDL_CreateMutex ();
    if (this->mutex == NULL) {
        fprintf (stderr, "Error: Unable to create SDL mutex.\n");
        goto CLEAN_AND_FAIL;
    }

    this->cond = SDL_CreateCond ();
    if (this->cond == NULL) {
        fprintf (stderr, "Error: Unable to create SDL condition.\n");
        goto CLEAN_AND_FAIL;
    }

    url_set_interrupt_cb (&decode_interrupt_cb);

CLEAN_AND_FAIL:
    this->cleanup ();
    return false;
}


///
/// \brief Put/push AVPacket to the queue.
///
bool PacketQueue::put (AVPacket *packet)
{
    AVPacketList *pkt = NULL;

    if (av_dup_packet (packet) < 0) {
        fprintf (stderr, "Error : Unable to duplicate packet.\n");
        return false;
    }

    pkt = (AVPacketList *)av_malloc (sizeof (AVPacketList));
    if (!pkt) {
        fprintf (stderr, "Error : Unable to av_allocate a packet.\n");
        return false;
    }

    pkt->pkt = *packet;
    pkt->next = NULL;

    SDL_LockMutex (this->mutex);

    if (this->last_pkt == NULL)
        this->first_pkt = pkt;
    else
        this->last_pkt->next = pkt;
    
    this->last_pkt = pkt;
    this->nb_packets++;
    this->size += pkt->pkt.size;

    // Send a signal to our get function (if it is waiting)
    // through our condition variable to tell it that there
    // is data and it can proceed, then unlocks the mutex to let it go.

    SDL_CondSignal (this->cond);

    SDL_UnlockMutex (this->mutex);
    
    return true;
}


int PacketQueue::get (AVPacket *packet, int block)
{
    AVPacketList *pkt = NULL;
    int ret;

    SDL_LockMutex(this->mutex);

    for ( ; ; ) {
        if (quit) {
            ret = -1;
            break;
        }

        pkt = this->first_pkt;

        if (pkt) {
            this->first_pkt = pkt->next;
            
            if (!this->first_pkt)
                this->last_pkt = NULL;

            this->nb_packets--;
            this->size -= pkt->pkt.size;
            *packet = pkt->pkt;
            av_free (pkt);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(this->cond, this->mutex);
        }
    }

    SDL_UnlockMutex (this->mutex);
    return ret;
}


void PacketQueue::set_quit ()
{
    quit = 1;
}


void PacketQueue::close ()
{
    this->cleanup ();
}


PacketQueue::~PacketQueue ()
{
    this->cleanup ();
}

void PacketQueue::cleanup ()
{
    quit = 0;

    if (this->mutex != NULL)
        SDL_DestroyMutex (this->mutex);
    this->mutex = NULL;
    if (this->cond != NULL)
        SDL_DestroyCond (this->cond);
    this->cond = NULL;

    this->first_pkt  = NULL;
    this->last_pkt   = NULL;
    this->nb_packets = 0;
    this->mutex      = NULL;
    this->cond       = NULL;
    this->size       = 0;
}


int decode_interrupt_cb (void)
{
    return quit;
}


void audio_callback (void *userdata, Uint8 *stream, int len)
{
    AVCodecContext *audio_codec_ctx = (AVCodecContext *)userdata;
    int len1 = 0;
    int audio_size = 0;

    //static uint8_t *audio_buf = (uint8_t*)malloc (AVCODEC_MAX_AUDIO_FRAME_SIZE);
    static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2] = {0};
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;

    while(len > 0)
    {
        if(audio_buf_index >= audio_buf_size) {
            /* We have already sent all our data; get more. */
            audio_size = audio_decode_frame (audio_codec_ctx,
                audio_buf, sizeof (audio_buf));//AVCODEC_MAX_AUDIO_FRAME_SIZE);
            if(audio_size < 0) {
                /* If error, output silence */
                audio_buf_size = 1024;
                memset (audio_buf, 0, audio_buf_size);
            }
            else
            {
                audio_buf_size = audio_size;
            }
        
            audio_buf_index = 0;
        }
        
        len1 = audio_buf_size - audio_buf_index;
        if(len1 > len)
            len1 = len;
        memcpy (stream, (uint8_t *)audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }

    //free (audio_buf);
}


int audio_decode_frame
    (AVCodecContext *audio_codec_ctx, uint8_t *audio_buf, int buf_size)
{
    static AVPacket pkt;
    static uint8_t *audio_pkt_data = NULL;
    static int audio_pkt_size = 0;

    int len1 = 0;
    int data_size = 0;

    for ( ; ; ) {
        while (audio_pkt_size > 0) {
            data_size = buf_size;
            //len1 = avcodec_decode_audio3 (audio_codec_ctx, &data_size, (int *)audio_buf, &pkt);
                //&data_size, audio_pkt_data, audio_pkt_size);
            len1 = avcodec_decode_audio2 (audio_codec_ctx, (int16_t *)audio_buf,
                &data_size, audio_pkt_data, audio_pkt_size);
            if (len1 < 0) {
                /* if error, skip frame */
                audio_pkt_size = 0;
                break;
            }
    
            audio_pkt_data += len1;
            audio_pkt_size -= len1;

            if (data_size <= 0) {
                /* No data yet, get more frames */
                continue;
            }
            
            /* We have data, return it and come back for more later */
            return data_size;
        }
        
        if (pkt.data) {
            av_free_packet(&pkt);
        }        

        if (quit)
            return -1;

        if (local_audio_packet_queue->get (&pkt, 1) < 0)
            return -1;
        
        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
    }
}

#endif /* defined (__AvsClient__) */


// Do any needed initialization here.
AVManager::AVManager (AVManagerMode mode) :
    filename (NULL), stream_info (NULL), codec (NULL), codec_ctx (NULL),
    video_stream_index (0), audio_stream_index (0)
#if defined (__AvsServer__)
    , format_ctx (NULL)
#endif /* defined (__AvsServer__) */
#if defined (__AvsClient__)
    , audio_codec (NULL), audio_codec_ctx (NULL),
    sws_ctx (NULL), screen(NULL), yuv_overlay (NULL)
#endif /* defined (__AvsClient__) */
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
    int video_stream = -1;
    int audio_stream = -1;
    
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
    //av_dump_format (format_ctx, 0, this->filename, 0);
    
    // Now format_ctx->streams is just an array of pointers,
    // of size format_ctx->nb_streams, so let's walk through it
    // until we find a video stream. The same for the video stream.
    for (i=0; i < format_ctx->nb_streams; i++)
    {
        if (format_ctx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO
            && video_stream < 0) {
            video_stream = i;
        }
        if (format_ctx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO
            && audio_stream < 0) {
            audio_stream = i;
        }
    }

    if (video_stream == -1) {
        fprintf (stderr, "Error: No video stream was found.\n");
        return false;
    }

    if (audio_stream == -1) {
        fprintf (stderr, "Error: No audio stream was found.\n");
        return false;
    }

    this->video_stream_index = video_stream;
    this->audio_stream_index = audio_stream;
    
    // Get a pointer to the codec context for the video stream.
    this->codec_ctx = format_ctx->streams[video_stream]->codec;

    // Find the decoder for the video stream.
    this->codec = avcodec_find_decoder(codec_ctx->codec_id);
    if(this->codec == NULL) {
        fprintf (stderr, "Error: Unsupported video codec.\n");
        return false;
    }
    
    // Open video codec.
    if (avcodec_open (codec_ctx, codec) < 0) {
        fprintf (stderr, "Error: Could not open video codec.\n");
        return false;
    }

    // Get a pointer to the codec context for the video stream.
    this->audio_codec_ctx = format_ctx->streams[audio_stream]->codec;

    // Find the decoder for the video stream.
    this->audio_codec = avcodec_find_decoder(audio_codec_ctx->codec_id);
    if(this->audio_codec == NULL) {
        fprintf (stderr, "Error: Unsupported audio codec.\n");
        return false;
    }
    
    // Open video codec.
    if (avcodec_open (audio_codec_ctx, audio_codec) < 0) {
        fprintf (stderr, "Error: Could not open audio codec.\n");
        return false;
    }

    return true;
}

/** The client needs this first, to setup a new codec context,
 *  so that it may render the frames received from the server. */
bool AVManager::get_stream_info (streaminfo *stream_info)
{
    if (this->codec_ctx == NULL || stream_info == NULL)
        return false;
    
    // Cache a local copy of this. Maybe not useful after all.
    this->stream_info->width    = this->codec_ctx->width;
    this->stream_info->height   = this->codec_ctx->height;
    this->stream_info->pix_fmt  = this->codec_ctx->pix_fmt;
    this->stream_info->codec_id = this->codec_ctx->codec_id;
    this->stream_info->audio_codec_id = this->audio_codec_ctx->codec_id;
    this->stream_info->freq     = this->audio_codec_ctx->sample_rate;
    this->stream_info->channels = this->audio_codec_ctx->channels;
    
    stream_info->width    = this->codec_ctx->width;
    stream_info->height   = this->codec_ctx->height;
    stream_info->pix_fmt  = this->codec_ctx->pix_fmt;
    stream_info->codec_id = this->codec_ctx->codec_id;
    stream_info->audio_codec_id = this->audio_codec_ctx->codec_id;
    stream_info->freq     = this->audio_codec_ctx->sample_rate;
    stream_info->channels = this->audio_codec_ctx->channels;
    return true;
}

/**
    \brief You must dealloc the packet after reading it from the file.
    @return 0 if OK, < 0 on error or end of file
*/
bool AVManager::read_packet_from_file (AVMediaPacket *media_packet)
{
    int frame_finished = 0;
    
    if (media_packet == NULL) {
        fprintf (stderr, "Error: media packet storage is NULL.\n");
        return false;
    }
    
    // Read media packet (audio/video).

    if (av_read_frame (this->format_ctx, &media_packet->packet) < 0) {
        // Error or end of file.
        return false;
    }

    // Fill in the packet attributes with some basic information.

    if (media_packet->packet.stream_index == this->video_stream_index)
        media_packet->packet_type = AVPacketVideoType;
    else if (media_packet->packet.stream_index == this->audio_stream_index)
        media_packet->packet_type = AVPacketAudioType;
    else media_packet->packet_type = AVPacketUnknownType;

    return true;
}

void AVManager::end_send ()
{
    // Close the codec.
    avcodec_close(codec_ctx);
    this->codec_ctx = NULL;

    // Close the video file
    av_close_input_file(format_ctx);
    this->format_ctx = NULL;
    
    // Freedom...
    free (this->filename);
    this->filename = NULL;
}

#endif /* defined (__AvsServer__) */

#if defined (__AvsClient__)

///
/// \brief Initialize stream with initial setup information from the server.
///
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
    
    this->stream_info->width    = stream_info->width;
    this->stream_info->height   = stream_info->height;
    this->stream_info->pix_fmt  = stream_info->pix_fmt;
    this->stream_info->codec_id = stream_info->codec_id;
    this->stream_info->audio_codec_id = stream_info->audio_codec_id;
    this->stream_info->freq     = stream_info->freq;
    this->stream_info->channels = stream_info->channels;    

    // Find the decoder for the video stream.
    this->codec = avcodec_find_decoder (this->stream_info->codec_id);
    if (this->codec == NULL) {
        fprintf (stderr, "Error: Unable to find propper video decoder.\n");
        goto CLEAN_AND_FAIL;    
    }
    
    // Create new video context.
    this->codec_ctx = avcodec_alloc_context ();
    if (this->codec_ctx == NULL) {
        fprintf (stderr, "Error: Unable to get a new video codec context.\n");
        goto CLEAN_AND_FAIL;
    }

    if (avcodec_open (this->codec_ctx, this->codec) < 0) {
        fprintf (stderr, "Error: Could not open video codec.");
        goto CLEAN_AND_FAIL;
    }

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

    // Create and open new audio context.
    
    this->audio_codec_ctx = avcodec_alloc_context ();
    if (this->audio_codec_ctx == NULL) {
        fprintf (stderr, "Error: Unable to get a new audio codec context.\n");
        goto CLEAN_AND_FAIL;
    }


    // Open SDL audio.

    this->wanted_audio_spec.freq     = this->audio_codec_ctx->sample_rate;
    this->wanted_audio_spec.format   = AUDIO_S16SYS;
    this->wanted_audio_spec.channels = this->audio_codec_ctx->channels;
    this->wanted_audio_spec.silence  = 0;
    this->wanted_audio_spec.samples  = SDL_AUDIO_BUFFER_SIZE;
    this->wanted_audio_spec.callback = audio_callback;
    this->wanted_audio_spec.userdata = this->audio_codec_ctx;
    
    if(SDL_OpenAudio (&this->wanted_audio_spec, &this->obtained_audio_spec) < 0) {
        fprintf (stderr, "Error: SDL_OpenAudio: %s\n", SDL_GetError());
        goto CLEAN_AND_FAIL;
    }

    // Find the decoder for the audio stream.
    this->audio_codec = avcodec_find_decoder (this->stream_info->audio_codec_id);
    if (this->audio_codec == NULL) {
        fprintf (stderr, "Error: Unable to find propper audio decoder.\n");
        goto CLEAN_AND_FAIL;
    }

    if (avcodec_open (this->audio_codec_ctx, this->audio_codec) < 0) {
        fprintf (stderr, "Error: Could not open audio codec.\n");
        goto CLEAN_AND_FAIL;
    }

    // Initialize audio packet queue.

    if (audio_packet_queue.init ()) {
        fprintf (stderr, "Error: Could not initialize audio packet queue.\n");
        goto CLEAN_AND_FAIL;    
    }

    SDL_PauseAudio (0);

    return true;
    
CLEAN_AND_FAIL:
    if (this->codec_ctx) {
        avcodec_close (this->codec_ctx);
        av_free (this->codec_ctx);
    }
    this->codec_ctx = NULL;
    if (this->audio_codec_ctx) {
        avcodec_close (this->audio_codec_ctx);
        av_free (this->audio_codec_ctx);
    }
    this->audio_codec_ctx = NULL;    
    if (this->sws_ctx)
        sws_freeContext (this->sws_ctx);
    this->sws_ctx = NULL;
    this->audio_packet_queue.close ();
    return false;
}


///
/// \brief Play a frame received from the server.
///
bool AVManager::play_video_packet (AVMediaPacket *media_packet)
{
    AVPicture picture;
    SDL_Rect  rectangle;
    int       scale = 0;
    AVFrame   *frame = NULL;
    int       frame_finished = false;
    AVPacket  *packet = NULL;

    if (media_packet == NULL) {
        fprintf (stderr, "Error: Invalid media packet.\n");
        return false;
    }

    packet = &media_packet->packet;

    frame = avcodec_alloc_frame ();
    if (frame == NULL) {
        fprintf (stderr, "Unable to allocate new AVFrame (frame).\n");
        return false;
    }
    
    avcodec_decode_video (this->codec_ctx, frame,
        &frame_finished, packet->data, packet->size);

    if (frame_finished)
    {
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
        
        SDL_DisplayYUVOverlay (this->yuv_overlay, &rectangle);
    }   
    
    return true;
}


///
/// \brief Just puts the packet in order for the AVManager
///        internal audio queue to play the audio packet.
///
bool AVManager::play_audio_packet (AVMediaPacket *media_packet)
{
    this->audio_packet_queue.put (&media_packet->packet);
}


void AVManager::end_recv ()
{
    if (this->codec_ctx) {
        avcodec_close (this->codec_ctx);
        av_free (this->codec_ctx);
    }
    this->codec_ctx = NULL;
    if (this->audio_codec_ctx) {
        avcodec_close (this->audio_codec_ctx);
        av_free (this->audio_codec_ctx);
    }
    this->audio_codec_ctx = NULL;    
    if (this->sws_ctx)
        sws_freeContext (this->sws_ctx);
    this->sws_ctx = NULL;
}

#endif /* defined (__AvsClient) */

void AVManager::free_packet (AVMediaPacket *media_packet)
{
    if (media_packet)
        av_free_packet (&media_packet->packet);
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
    this->audio_packet_queue.close ();
#endif /* (__AvsClient__) */
    this->video_stream_index = 0;
    this->audio_stream_index = 0;
    
}
