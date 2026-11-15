#ifndef VOID_INTERNAL_H
#define VOID_INTERNAL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "voiddef.h"

typedef enum {
    VOID_INIT_SDL = 1 << 0,
    VOID_INIT_IMG = 1 << 1,
    VOID_INIT_MIX = 1 << 2,
} VoidInitFlag;

typedef struct VoidThread {
    VoidThreadFunc func;
    void *data;
} VoidThread;

typedef struct VoidLogEntry {
    VoidLogLevel level;
    uint64_t timestamp;
    uint32_t thread_id;
    char text[256];
    volatile uint32_t is_ready;
} VoidLogEntry;

struct VoidWindow {
    SDL_Window* handle;
    SDL_Renderer* render;
    bool is_running, should_close, was_resized;
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
