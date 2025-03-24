#pragma once
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>

#define COOL_FONT "/System/Library/Fonts/Supplemental/Comic Sans MS Bold.ttf"
#define SDL_COLOR_YELLOW (SDL_Color){255, 255, 0, 255}

// Globals
extern AVFormatContext* fmt_ctx;
extern AVCodecContext* video_dec_ctx;

extern int video_stream_idx;
extern AVPacket* pkt;

static AVStream* video_stream = NULL;

static uint8_t* video_dst_data[4] = {NULL};
static int      video_dst_linesize[4];
static int video_dst_bufsize;
static AVFrame* frame = NULL;
static TTF_Font* cool_font = NULL;

static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file\n",
                av_get_media_type_string(type));
        return ret;
    }
    int stream_index = ret;
    AVStream *st = fmt_ctx->streams[stream_index];

    // Find decoder for the stream
    const AVCodec *dec = NULL;
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
        // These two return values are special and mean there is no output
        // frame available, but there were no errors during decoding
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
            return 0;
        fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
        return ret;
    }
    return ret;
}

void InitVideoStuff(char* filename) {
    int ret = 0;
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

    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        exit(AVERROR(ENOMEM));
    }

    cool_font = TTF_OpenFont(COOL_FONT, 24);
    if (cool_font == NULL) {
        fprintf(stderr, "Oops, couldn't load font %s!\n", COOL_FONT);
        exit(AVERROR(ENOMEM));
    }
}

void Render(SDL_Renderer* renderer, AVFrame *f, int frameNum) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red color
    SDL_RenderClear(renderer);  // Clear screen with red

    // Convert FFmpeg frame to SDL texture
    SDL_Texture* texture_vid = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_YV12,  // Adjust format to match your frame
        SDL_TEXTUREACCESS_STREAMING,
        f->width,
        f->height
    );

    // Update texture with frame data
    SDL_UpdateYUVTexture(texture_vid,
        NULL,
        f->data[0], f->linesize[0],  // Y plane
        f->data[1], f->linesize[1],  // U plane
        f->data[2], f->linesize[2]   // V plane
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
