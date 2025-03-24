#include <pthread.h>
#include <stdbool.h>
#include "video_utils.h"
#include "frame_queue.h"

AVFormatContext* fmt_ctx = NULL;
AVCodecContext* video_dec_ctx = NULL;

int video_stream_idx = -1;
AVPacket* pkt = NULL;

SDL_Renderer *renderer;

bool end_of_file_reached = false;

FrameQ frame_buff = {
    .front = 0,
    .cnt = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .buffer_is_not_full = PTHREAD_COND_INITIALIZER,
    .buffer_has_stuff = PTHREAD_COND_INITIALIZER,
};

void* rendering_thrd(void* arg) {
    AVFrame *f = av_frame_alloc();
    int frame_num = 0;
    while (!(end_of_file_reached && frame_buff.cnt)) {
        usleep(1000000/30); // 30 fps!
        pop_q(&frame_buff, f);
        Render(renderer, f, frame_num);
        frame_num++;
    }
}

void* decoder_thrd(void* arg) {
    while (!end_of_file_reached) {
        av_read_frame(fmt_ctx, pkt);
        if (pkt->stream_index == video_stream_idx) {
            int ret = decode_packet(video_dec_ctx, pkt);
            if (!ret) {
                push_q(&frame_buff, frame);
            } else if (ret == AVERROR_EOF) {
                end_of_file_reached = true;
            }
        }
        av_packet_unref(pkt);
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

    pthread_t reading, rendering;
    pthread_create(&reading, NULL, decoder_thrd, NULL);
    pthread_create(&rendering, NULL, rendering_thrd, NULL);

    // Event loop
    SDL_Event e;
    int quit = 0;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
                pthread_cancel(reading);
                pthread_cancel(rendering);
            }
        }
        if (end_of_file_reached) {
            break;
        }
    }

    pthread_join(reading, NULL);
    pthread_join(rendering, NULL);

    // Flush the decoder
    decode_packet(video_dec_ctx, NULL);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

