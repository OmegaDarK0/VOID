#ifndef VOID_INTERNAL_H
#define VOID_INTERNAL_H

#include <SDL2/SDL.h>
#include <stdbool.h>

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

struct VoidPoint {
    float x, y;
};

struct VoidRect {
    float x, y, w, h;
};

#endif //VOID_INTERNAL_H
