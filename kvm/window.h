#ifndef _KVM_WINDOW_H_
#define _KVM_WINDOW_H_

#include <stdint.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

struct kvm_window {
    SDL_Renderer* renderer;
    SDL_Window* window;
    TTF_Font* font;
};

int kvm_window_init(struct kvm_window* window);
int kvm_window_free(struct kvm_window* window);
int kvm_window_run(struct kvm_window* window);

#endif
