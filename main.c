#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

#define SDL_COLOR_YELLOW (SDL_Color){255, 255, 0, 255}

int main(int argc, char *argv[]) {
    printf("hi!!!!\n");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Hello SDL", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
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

    // This is not portable.
    TTF_Font* sans = TTF_OpenFont("/System/Library/Fonts/NewYorkItalic.ttf", 24);
    if (sans == NULL) {
        printf("Oops, not text!\n");
        return 1;
    }

    SDL_Surface* textSurface = TTF_RenderText_Solid(sans, "Hello!", SDL_COLOR_YELLOW);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect rect = {
        .x = 0, .y = 0, .w = textSurface->w, .h = textSurface->h
    };

    // Event loop
    SDL_Event e;
    int quit = 0;
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }

        // Render a red rectangle
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red color
        SDL_RenderClear(renderer);  // Clear screen with red
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_RenderPresent(renderer);  // Update the window
    }

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

