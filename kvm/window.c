#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "window.h"

#define MAX_ROWS 25
#define MAX_COLS 80

struct vga_char {
    SDL_Texture* texture;

    SDL_Rect rect;
    char character;
    uint8_t fg_color;
    uint8_t bg_color;
};

struct vga_char vga_sdl[MAX_ROWS][MAX_COLS];

void vga_char_init(struct vga_char* vga_char)
{
    vga_char->texture = NULL;
    vga_char->character = ' ';
    vga_char->fg_color = 0xf;
    vga_char->bg_color = 0x0;
    vga_char->rect.x = 0;
    vga_char->rect.y = 0;
    // TODO: get h and w from actual render
    vga_char->rect.h = 32;
    vga_char->rect.w = 14;
}

void vga_char_free(struct vga_char* vga_char)
{
    if (vga_char->texture != NULL)
        SDL_DestroyTexture(vga_char->texture);
    vga_char_init(vga_char);
}

struct chara {
    int width;
    int height;
};

void get_font_dimensions(struct chara* chara, TTF_Font* font)
{
    SDL_Color tmpColor = { 0, 0, 0, 0 };
    // Create a single char temporarily to calculate our dimensions
    SDL_Surface* surface = TTF_RenderUTF8_Solid(font, "O", tmpColor);
    chara->width = surface->w;
    chara->height = surface->h;
    SDL_FreeSurface(surface);
}

void set_character(SDL_Renderer* renderer, struct vga_char* vgac, TTF_Font* font, int col, int row)
{
    char text[2] = {
        vgac->character,
        0,
    };

    SDL_Color fgColor = { 255, 255, 255, 0 };
    SDL_Color bgColor = { 255, 0, 0, 0 };

    if (vgac->texture != NULL) {
        SDL_DestroyTexture(vgac->texture);
        vgac->texture = NULL;
    }

    SDL_Surface* surface = TTF_RenderText_Shaded(font, text, fgColor, bgColor);
    vgac->texture = SDL_CreateTextureFromSurface(renderer, surface);
    vgac->rect.x = vgac->rect.w * col;
    vgac->rect.y = vgac->rect.h * row;
    SDL_FreeSurface(surface);
}

int kvm_window_init(struct kvm_window* window, struct vm* vm)
{
    window->vm = vm;
    /* Inint TTF. */
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
    TTF_Init();
    window->font = TTF_OpenFont("test_font.ttf", 24);
    if (window->font == NULL) {
        fprintf(stderr, "error: font not found\n");
        return 1;
    }

    struct chara chara;
    get_font_dimensions(&chara, window->font);
    SDL_CreateWindowAndRenderer(chara.width * (MAX_COLS + 1), chara.height * (MAX_ROWS + 1), 0, &window->window, &window->renderer);

    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            int val = (row * MAX_COLS) + col + row;
            char c = (val % 10) + '0';
            vga_char_init(&vga_sdl[row][col]);
            vga_sdl[row][col].character = c;
            set_character(window->renderer, &vga_sdl[row][col], window->font, col, row);
        }
    }
    return 0;
}

int kvm_window_free(struct kvm_window* window)
{
    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            vga_char_free(&vga_sdl[row][col]);
        }
    }

    TTF_Quit();

    SDL_DestroyRenderer(window->renderer);
    SDL_DestroyWindow(window->window);
    SDL_Quit();
    return 0;
}

void read_vga_memory(struct kvm_window* window)
{
    // TODO: color
    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            int offset = (row * MAX_COLS + col) * 2;
            char ch = *(char*)(window->vm->shared_memory + 0xb8000 + offset);
            if (ch != 0 && vga_sdl[row][col].character != ch) {
                vga_sdl[row][col].character = ch;
                set_character(window->renderer, &vga_sdl[row][col], window->font, col, row);
            }
        }
    }
}

int kvm_window_run(struct kvm_window* window)
{
    int quit = 0;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event) == 1) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }
        SDL_SetRenderDrawColor(window->renderer, 0, 0, 0, 0);
        SDL_RenderClear(window->renderer);
        read_vga_memory(window);

        for (int row = 0; row < MAX_ROWS; row++) {
            for (int col = 0; col < MAX_COLS; col++) {
                SDL_RenderCopy(window->renderer, vga_sdl[row][col].texture, NULL, &vga_sdl[row][col].rect);
            }
        }

        SDL_RenderPresent(window->renderer);
        SDL_Delay(33);
    }

    return 0;
}
