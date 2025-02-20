#include <stdio.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>

#define SDL_COLOR_YELLOW (SDL_Color){255, 255, 0, 255}
#define COOL_FONT "/System/Library/Fonts/Supplemental/Comic Sans MS Bold.ttf"

TTF_Font* cool_font = NULL;

static AVFormatContext* fmt_ctx = NULL;
static AVCodecContext* video_dec_ctx = NULL;
static AVStream* video_stream = NULL;
 
static uint8_t* video_dst_data[4] = {NULL};
static int      video_dst_linesize[4];
static int video_dst_bufsize;
 
static int video_stream_idx = -1;
static AVFrame* frame = NULL;
static AVPacket* pkt = NULL;
static int video_frame_count = 0;


static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    const AVCodec *dec = NULL;
 
    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file\n",
                av_get_media_type_string(type));
        return ret;
    }
    stream_index = ret;
    st = fmt_ctx->streams[stream_index];

    // Find decoder for the stream
    dec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!dec) {
        fprintf(stderr, "Failed to find %s codec\n",
                av_get_media_type_string(type));
        return AVERROR(EINVAL);
    }

    /* Allocate a codec context for the decoder */
    *dec_ctx = avcodec_alloc_context3(dec);
    if (!*dec_ctx) {
        fprintf(stderr, "Failed to allocate the %s codec context\n",
                av_get_media_type_string(type));
        return AVERROR(ENOMEM);
    }

    /* Copy codec parameters from input stream to output codec context */
    if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
        fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                av_get_media_type_string(type));
        return ret;
    }

    /* Init the decoders */
    if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
        fprintf(stderr, "Failed to open %s codec\n",
                av_get_media_type_string(type));
        return ret;
    }
    *stream_idx = stream_index;
    return 0;
}


static int decode_packet(AVCodecContext *dec, const AVPacket *pkt)
{
    int ret = 0;
 
    // Submit the packet to the decoder
    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
        return ret;
    }


    ret = avcodec_receive_frame(dec, frame);
    if (ret < 0) {
        // those two return values are special and mean there is no output
        // frame available, but there were no errors during decoding
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
            return 0;

        fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
        return ret;
    }
 
    return ret;
}


void InitVideoStuff(char* filename) {
    int ret;
    // Open input file, and allocate format context
    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", filename);
        exit(1);
    }
 
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        video_stream = fmt_ctx->streams[video_stream_idx];

        // Allocate image where the decoded image will be put
        ret = av_image_alloc(video_dst_data, video_dst_linesize,
                             video_dec_ctx->width, video_dec_ctx->height, video_dec_ctx->pix_fmt, 1);

        if (ret < 0) {
            fprintf(stderr, "Could not allocate raw video buffer\n");
            exit(1);
        }
        video_dst_bufsize = ret;
    }

    // Print info about the video.
    av_dump_format(fmt_ctx, 0, filename, 0);
 
    if (!video_stream) {
        fprintf(stderr, "Could not findr video stream in the input, aborting\n");
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        exit(AVERROR(ENOMEM));
    }
 
    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Could not allocate packet\n");
        exit(AVERROR(ENOMEM));
    }
 
}

void RenderWindow(SDL_Renderer* renderer, int frameNum) {

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red color
    SDL_RenderClear(renderer);  // Clear screen with red


    // Convert FFmpeg frame to SDL texture
    SDL_Texture* texture_vid = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_YV12,  // Adjust format to match your frame
        SDL_TEXTUREACCESS_STREAMING,
        frame->width,
        frame->height
    );

    // Update texture with frame data
    SDL_UpdateYUVTexture(texture_vid,
        NULL,
        frame->data[0], frame->linesize[0],  // Y plane
        frame->data[1], frame->linesize[1],  // U plane
        frame->data[2], frame->linesize[2]   // V plane
    );

    SDL_RenderCopy(renderer, texture_vid, NULL, NULL);

    char txt[200];
    sprintf(txt, "Hello! %d", frameNum);
    SDL_Surface* textSurface = TTF_RenderText_Solid(cool_font, txt, SDL_COLOR_YELLOW);
    SDL_Texture*texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect rect = {
        .x = 0, .y = 0, .w = textSurface->w, .h = textSurface->h
    };


    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_RenderPresent(renderer);  // Update the window
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(texture);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("argc %d\n", argc);
        fprintf(stderr, "Usage: %s <video file> \n", argv[0]);
        exit(0);
    }

    InitVideoStuff(argv[1]);

    if (TTF_Init() == -1) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        return 1;
    }

    cool_font = TTF_OpenFont(COOL_FONT, 24);
    if (cool_font == NULL) {
        printf("Oops, couldn't load font %s!\n", COOL_FONT);
        return 1;
    }


    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }


    SDL_Window *win = SDL_CreateWindow("Hello SDL", 100, 100, 640, 480, SDL_WINDOW_RESIZABLE);
    if (!win) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }


    // Event loop
    SDL_Event e;
    int ret = 0;
    int quit = 0;
    for (int i = 0; i < 1500; ++i) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }

        while (av_read_frame(fmt_ctx, pkt) >= 0) {
            // Check if the packet belongs to a stream we are interested in, otherwise
            // skip it
            if (pkt->stream_index == video_stream_idx)
                ret = decode_packet(video_dec_ctx, pkt);
            RenderWindow(renderer, i);
            av_packet_unref(pkt);
            if (ret < 0)
                break;
            break;
        }
    }

    // Flush the decoder
    decode_packet(video_dec_ctx, NULL);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

