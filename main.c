#include <pthread.h>
#include "video_utils.h"
#include "frame_queue.h"

AVFormatContext* fmt_ctx = NULL;
AVCodecContext* video_dec_ctx = NULL;

int video_stream_idx = -1;
AVPacket* pkt = NULL;

SDL_Renderer *renderer;

FrameQ frame_buff = {
    .front = 0,
    .cnt = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .buffer_is_not_full = PTHREAD_COND_INITIALIZER,
    .buffer_has_stuff = PTHREAD_COND_INITIALIZER,
};

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

void _render(int frame_num) {
    AVFrame *f = av_frame_alloc();
    usleep(1000000/30); // 30 fps!
    pop_q(&frame_buff, f);
    Render(renderer, f, frame_num);
}
void* rendering_thrd(void* arg) {
    // TODO: Make this run until we're out of video frames
    AVFrame *f = av_frame_alloc();
    for (int i = 0; i < 500; i++) {
        usleep(1000000/30); // 30 fps!
        pop_q(&frame_buff, f);
        Render(renderer, f, i);
    }
}

int _decode() {
    int got_frames = 1;
    for (int i = 0; i < 20 && frame_buff.cnt >= 0 && frame_buff.cnt < FRAME_BUFFER_SIZE; i++) {
        av_read_frame(fmt_ctx, pkt);
        // Early return if not a video packet (since this is a toy audio-less video player).
        if (pkt->stream_index != video_stream_idx) {
            av_packet_unref(pkt);
            continue;
        }
        const int ret = decode_packet(video_dec_ctx, pkt);
        if (!ret) {
            push_q(&frame_buff, frame);
            got_frames=0;
        }
        av_packet_unref(pkt);
    }
    return got_frames;
}

void* decoder_thrd(void* arg) {
    // TODO: Make this run until we're out of video frames
    for (int i = 0; i < 500; i++) {
        av_read_frame(fmt_ctx, pkt);
        // Early return if not a video packet (since this is a toy audio-less video player).
        if (pkt->stream_index != video_stream_idx) {
            av_packet_unref(pkt);
            continue;
        }
        const int ret = decode_packet(video_dec_ctx, pkt);
        if (!ret) {
            push_q(&frame_buff, frame);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("argc %d\n", argc);
        fprintf(stderr, "Usage: %s <video file> \n", argv[0]);
        exit(0);
    }

    InitVideoStuff(argv[1]);

    // Init frame buff
    // TODO: Make this good.
    for (int i = 0; i < FRAME_BUFFER_SIZE; i++) {
        frame_buff.frames[i] = av_frame_alloc();
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("AubPlayer", 100, 100, 640, 480, SDL_WINDOW_RESIZABLE);
    if (!win) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    // pthread_t reading, rendering;
    // pthread_create(&reading, NULL, decoder_thrd, NULL);
    // pthread_create(&rendering, NULL, rendering_thrd, NULL);

    // Event loop
    SDL_Event e;
    int quit = 0;
    int frame_cnt = 0;
    // for (;;) {
    for (int i = 0; i < 500; ++i) {

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }
        if (!_decode()) {
            _render(i);
        }
        if (quit) {
            break;
        }
    }

    // pthread_cancel(reading);
    // pthread_cancel(rendering);

    // Flush the decoder
    decode_packet(video_dec_ctx, NULL);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

