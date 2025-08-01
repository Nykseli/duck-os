#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "window.h"

#define MAX_ROWS 25
#define MAX_COLS 80

#define VGA_BLACK 0x0
#define VGA_BLUE 0x1
#define VGA_GREEN 0x2
#define VGA_CYAN 0x3
#define VGA_RED 0x4
#define VGA_MAGENTA 0x5
#define VGA_BROWN 0x6
#define VGA_LIGHT_GRAY 0x7
#define VGA_DARK_GRAY 0x8
#define VGA_LIGHT_BLUE 0x9
#define VGA_LIGHT_GREEN 0xa
#define VGA_LIGHT_CYAN 0xb
#define VGA_LIGHT_RED 0xc
#define VGA_LIGHT_MAGENTA 0xd
#define VGA_YELLOW 0xe
#define VGA_WHITE 0xf

SDL_Color vga_color_to_sdl(uint8_t color)
{
    // colors are from: https://en.wikipedia.org/wiki/BIOS_color_attributes
    switch (color) {
    case VGA_BLACK:
        return (SDL_Color) { 0, 0, 0, 0 };
    case VGA_BLUE:
        return (SDL_Color) { 0, 0, 0xaa, 0 };
    case VGA_GREEN:
        return (SDL_Color) { 0, 0xaa, 0, 0 };
    case VGA_CYAN:
        return (SDL_Color) { 0, 0xaa, 0xaa, 0 };
    case VGA_RED:
        return (SDL_Color) { 0xaa, 0, 0, 0 };
    case VGA_MAGENTA:
        return (SDL_Color) { 0xaa, 0, 0xaa, 0 };
    case VGA_BROWN:
        return (SDL_Color) { 0xaa, 0x55, 0, 0 };
    case VGA_LIGHT_GRAY:
        return (SDL_Color) { 0xaa, 0xaa, 0xaa, 0 };
    case VGA_DARK_GRAY:
        return (SDL_Color) { 0x55, 0x55, 0x55, 0 };
    case VGA_LIGHT_BLUE:
        return (SDL_Color) { 0x55, 0x55, 0xff, 0 };
    case VGA_LIGHT_GREEN:
        return (SDL_Color) { 0x55, 0xff, 0x55, 0 };
    case VGA_LIGHT_CYAN:
        return (SDL_Color) { 0x55, 0xff, 0xff, 0 };
    case VGA_LIGHT_RED:
        return (SDL_Color) { 0xff, 0x55, 0x55, 0 };
    case VGA_LIGHT_MAGENTA:
        return (SDL_Color) { 0xff, 0x55, 0xff, 0 };
    case VGA_YELLOW:
        return (SDL_Color) { 0xff, 0xff, 0x55, 0 };
    case VGA_WHITE:
        return (SDL_Color) { 0xff, 0xff, 0xff, 0 };
    }

    return (SDL_Color) { 0, 0, 0, 0 };
}

struct vga_char {
    SDL_Texture* texture;

    SDL_Rect rect;
    char character;
    uint8_t fg_color;
    uint8_t bg_color;
};

struct vga_char vga_sdl[MAX_ROWS][MAX_COLS];

int font_width = 0;
int font_height = 0;

void vga_char_init(struct vga_char* vga_char)
{
    vga_char->texture = NULL;
    vga_char->character = ' ';
    vga_char->fg_color = 0xf;
    vga_char->bg_color = 0x0;
    vga_char->rect.x = 0;
    vga_char->rect.y = 0;
    // TODO: get h and w from actual render
    vga_char->rect.h = font_height;
    vga_char->rect.w = font_width;
}

void vga_char_free(struct vga_char* vga_char)
{
    if (vga_char->texture != NULL)
        SDL_DestroyTexture(vga_char->texture);
    vga_char_init(vga_char);
}

void get_font_dimensions(TTF_Font* font)
{
    SDL_Color tmpColor = { 0, 0, 0, 0 };
    // Create a single char temporarily to calculate our dimensions
    SDL_Surface* surface = TTF_RenderUTF8_Solid(font, "O", tmpColor);
    font_width = surface->w;
    font_height = surface->h;

    SDL_FreeSurface(surface);
}

void set_character(SDL_Renderer* renderer, struct vga_char* vgac, TTF_Font* font, int col, int row)
{
    char text[2] = {
        vgac->character,
        0,
    };

    SDL_Color fgColor = vga_color_to_sdl(vgac->fg_color);
    SDL_Color bgColor = vga_color_to_sdl(vgac->bg_color);

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
    window->font = TTF_OpenFont("font/DroidSansMono.ttf", 24);
    if (window->font == NULL) {
        fprintf(stderr, "error: font not found\n");
        return 1;
    }

    get_font_dimensions(window->font);
    SDL_CreateWindowAndRenderer(font_width * (MAX_COLS + 1), font_height * (MAX_ROWS + 1), 0, &window->window, &window->renderer);

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
    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            int offset = (row * MAX_COLS + col) * 2;
            char ch = *(char*)(window->vm->shared_memory + 0xb8000 + offset);
            uint8_t color = *(uint8_t*)(window->vm->shared_memory + 0xb8000 + offset + 1);
            uint8_t bg = color >> 4;
            uint8_t fg = color & 0x0f;

            struct vga_char* vchar = &vga_sdl[row][col];
            if (ch != 0 && (vchar->character != ch || fg != vchar->fg_color || bg != vchar->bg_color)) {
                vchar->bg_color = bg;
                vchar->fg_color = fg;
                vchar->character = ch;
                set_character(window->renderer, vchar, window->font, col, row);
            }
        }
    }
}

enum ps2_scan_code sdlkey_to_ps2(SDL_Keycode code)
{
    switch (code) {
    case SDLK_a:
        return PS2_A;
    case SDLK_b:
        return PS2_B;
    case SDLK_c:
        return PS2_C;
    case SDLK_d:
        return PS2_D;
    case SDLK_e:
        return PS2_E;
    case SDLK_f:
        return PS2_F;
    case SDLK_g:
        return PS2_G;
    case SDLK_h:
        return PS2_H;
    case SDLK_i:
        return PS2_I;
    case SDLK_j:
        return PS2_J;
    case SDLK_k:
        return PS2_L;
    case SDLK_l:
        return PS2_L;
    case SDLK_m:
        return PS2_M;
    case SDLK_n:
        return PS2_N;
    case SDLK_o:
        return PS2_O;
    case SDLK_p:
        return PS2_P;
    case SDLK_q:
        return PS2_Q;
    case SDLK_r:
        return PS2_R;
    case SDLK_s:
        return PS2_S;
    case SDLK_t:
        return PS2_T;
    case SDLK_u:
        return PS2_U;
    case SDLK_v:
        return PS2_V;
    case SDLK_w:
        return PS2_W;
    case SDLK_x:
        return PS2_X;
    case SDLK_y:
        return PS2_Y;
    case SDLK_z:
        return PS2_Z;
    case SDLK_0:
        return PS2_Num_0;
    case SDLK_1:
        return PS2_Num_1;
    case SDLK_2:
        return PS2_Num_2;
    case SDLK_3:
        return PS2_Num_3;
    case SDLK_4:
        return PS2_Num_4;
    case SDLK_5:
        return PS2_Num_5;
    case SDLK_6:
        return PS2_Num_6;
    case SDLK_7:
        return PS2_Num_7;
    case SDLK_8:
        return PS2_Num_8;
    case SDLK_9:
        return PS2_Num_9;
    case SDLK_BACKSPACE:
        return PS2_Backspace;
    case SDLK_RETURN:
        return PS2_ENTER;
    case SDLK_SPACE:
        return PS2_Space;
    default:
        printf("Unmapped SDL key: '0x%02x'\n", code);
        return PS2_ERROR;
    }
};

int kvm_window_run(struct kvm_window* window)
{
    int quit = 0;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event) == 1) {
            switch (event.type) {
            case SDL_QUIT:
                quit = 1;
                break;
            case SDL_KEYUP:
                kvm_vm_send_key(window->vm, sdlkey_to_ps2(event.key.keysym.sym), true);
                break;
            case SDL_KEYDOWN:
                kvm_vm_send_key(window->vm, sdlkey_to_ps2(event.key.keysym.sym), false);
                break;
            default:
                break;
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
