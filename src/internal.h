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

#endif //VOID_INTERNAL_H
