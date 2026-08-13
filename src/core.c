#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "void.h"

#define SDL_INIT_FLAGS (SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO)
#define IMG_INIT_FLAGS (IMG_INIT_JPG | IMG_INIT_PNG)
#define MIX_INIT_FLAGS (MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG)

uint8 s_init_state = 0;

bool void_init(void) {
    void_log_init();
    if (s_init_state) {
        VOID_LOG_WARN("VOID already initialised! (state: %d)", s_init_state);
        return false;
    }
    if (SDL_Init(SDL_INIT_FLAGS) < 0) {
        VOID_LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    s_init_state |= VOID_INIT_SDL;
    if ((IMG_Init(IMG_INIT_FLAGS) & IMG_INIT_FLAGS) != IMG_INIT_FLAGS) {
        VOID_LOG_ERROR("IMG_Init failed: %s", IMG_GetError());
        return false;
    }
    s_init_state |= VOID_INIT_IMG;
    if ((Mix_Init(MIX_INIT_FLAGS) & MIX_INIT_FLAGS) != MIX_INIT_FLAGS) {
        VOID_LOG_WARN("Mix_Init failed: %s", Mix_GetError());
    } else {
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            VOID_LOG_WARN("Mix_OpenAudio failed: %s", Mix_GetError());
        } else if (Mix_AllocateChannels(32) < 0) {
            VOID_LOG_WARN("Mix_AllocateChannels failed: %s", Mix_GetError());
        } else {
            s_init_state |= VOID_INIT_MIX;
        }
    }
    VOID_LOG_OK("VOID initialised! (state: %d)", s_init_state);
    return true;
}

void void_exit(void) {
    if (s_init_state & VOID_INIT_MIX) {
        s_init_state &= ~VOID_INIT_MIX;
        Mix_CloseAudio();
        Mix_Quit();
    }
    if (s_init_state & VOID_INIT_IMG) {
        s_init_state &= ~VOID_INIT_IMG;
        IMG_Quit();
    }
    if (s_init_state & VOID_INIT_SDL) {
        s_init_state &= ~VOID_INIT_SDL;
        SDL_Quit();
    }
    void_memory_exit();
    VOID_LOG_INFO("VOID exited.");
    void_log_exit();
}

uint32 void_get_core_count(void) {
    return SDL_GetCPUCount();
}
