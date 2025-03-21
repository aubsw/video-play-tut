#include "video_utils.h"

AVFormatContext* fmt_ctx = NULL;
AVCodecContext* video_dec_ctx = NULL;

int video_stream_idx = -1;
AVPacket* pkt = NULL;

void Render(SDL_Renderer* renderer, int frameNum) {
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

        bool is_video_packet;
        if (is_video_packet = (pkt->stream_index == video_stream_idx)) {
            frame_cnt += 1;
            ret = decode_packet(video_dec_ctx, pkt);
        } else {
            continue;
        }

        Render(renderer, frame_cnt);
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

