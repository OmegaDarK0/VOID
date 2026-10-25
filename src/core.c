#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <bcrypt.h>
#else
    #include <sys/random.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "void.h"

#define SDL_INIT_FLAGS (SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)
#define IMG_INIT_FLAGS (IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP)
#define MIX_INIT_FLAGS (MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG)

static uint8 s_init_state = 0;

bool void_init(void) {
    void_log_init();
    if (s_init_state) {
        VOID_LOG_WARN("VOID already initialised! (state: %d)", s_init_state);
        void_log_flush();
        return false;
    }
    if (SDL_Init(SDL_INIT_FLAGS) < 0) {
        VOID_LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        void_log_flush();
        return false;
    }
    s_init_state |= VOID_INIT_SDL;
    if ((IMG_Init(IMG_INIT_FLAGS) & IMG_INIT_FLAGS) != IMG_INIT_FLAGS) {
        VOID_LOG_ERROR("IMG_Init failed: %s", IMG_GetError());
        void_log_flush();
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

uint64 void_get_random_seed(void) {
    uint64 seed = 0;
#ifdef _WIN32
    if (BCryptGenRandom(NULL, (PUCHAR)&seed, sizeof(seed), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0 && seed != 0) {
        return seed;
    }
#else
    if (getrandom(&seed, sizeof(seed), 0) == (ssize_t)sizeof(seed) && seed != 0) {
        return seed;
    }
#endif
    uint64 z = SDL_GetPerformanceCounter() ^ ((uint64)void_time_get_ticks() << 32);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}
