#ifndef VOID_INTERNAL_H
#define VOID_INTERNAL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "types.h"

// ============================================================================
// INTERNAL OPAQUE STRUCTURES (Shared between VOID source files)
// ============================================================================

struct VoidWindow {
    SDL_Window* handle;
    SDL_Renderer* render;
    bool is_running, should_close;
};

struct VoidRender {
    SDL_Renderer* handle;
};

struct VoidTexture {
    SDL_Texture *handle;
    uint32 format;
    int access, width, height;
};

struct VoidSound {
    Mix_Chunk *handle;
};

struct VoidMusic {
    Mix_Music *handle;
};

// Internal input hooks called by void_window_poll_events (window.c)
void void_input_begin_frame(void);
void void_input_on_mouse_wheel(float x, float y);

#endif //VOID_INTERNAL_H
