#include "video_utils.h"

AVFormatContext* fmt_ctx = NULL;
AVCodecContext* video_dec_ctx = NULL;

int video_stream_idx = -1;
AVPacket* pkt = NULL;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("argc %d\n", argc);
        fprintf(stderr, "Usage: %s <video file> \n", argv[0]);
        exit(0);
    }

    InitVideoStuff(argv[1]);


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
    int frame_cnt = 0;
    for (int i = 0; i < 500; ++i) {

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }

        av_read_frame(fmt_ctx, pkt);
        // Check if the packet belongs to a stream we are interested in, otherwise
        // skip it

        bool is_video_packet;
        if (is_video_packet = (pkt->stream_index == video_stream_idx)) {
            frame_cnt += 1; ret = decode_packet(video_dec_ctx, pkt);
        } else {
            continue;
        }

        RenderWindow(renderer, frame_cnt);
        av_packet_unref(pkt);
        if (ret < 0)
            continue;
        if (quit) {
            break;
        }
        if (is_video_packet)
            usleep(1000000/30); // 30 fps!
    }

    // Flush the decoder
    decode_packet(video_dec_ctx, NULL);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

