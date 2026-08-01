#include <SDL2/SDL_mixer.h>
#include <stdalign.h>
#include "void.h"

struct VoidSound {
    Mix_Chunk *handle;
};

struct VoidMusic {
    Mix_Music *handle;
};

VoidSound* void_audio_load_sound(const char *filepath) {
    const VoidArenaMarker marker = void_arena_get_marker();
    VoidSound *sound = void_arena_alloc(sizeof(VoidSound), alignof(VoidSound));
    if (!sound) return NULL;
    sound->handle = Mix_LoadWAV(filepath);
    if (!sound->handle) {
        VOID_LOG_ERROR("Mix_LoadWAV failed for %s: %s", filepath, Mix_GetError());
        void_arena_rollback_to(marker);
        return NULL;
    }
    return sound;
}

void void_audio_play_sound(const VoidSound *sound) {
    if (sound && sound->handle) {
        Mix_PlayChannel(-1, sound->handle, 0); 
    }
}

void void_audio_destroy_sound(const VoidSound *sound) {
    if (sound && sound->handle) {
        Mix_FreeChunk(sound->handle);
    }
}

VoidMusic* void_audio_load_music(const char *filepath) {
    const VoidArenaMarker marker = void_arena_get_marker();
    VoidMusic *music = void_arena_alloc(sizeof(VoidMusic), alignof(VoidMusic));
    if (!music) return NULL;
    music->handle = Mix_LoadMUS(filepath);
    if (!music->handle) {
        VOID_LOG_ERROR("Mix_LoadMUS failed for %s: %s", filepath, Mix_GetError());
        void_arena_rollback_to(marker);
        return NULL;
    }
    return music;
}

void void_audio_play_music(const VoidMusic *music, const bool loop) {
    if (music && music->handle) {
        Mix_PlayMusic(music->handle, loop ? -1 : 1);
    }
}

void void_audio_stop_music(void) {
    Mix_HaltMusic();
}

void void_audio_destroy_music(const VoidMusic *music) {
    if (music && music->handle) {
        Mix_FreeMusic(music->handle);
    }
}

void void_audio_set_master_volume(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    Mix_Volume(-1, (int)(volume * MIX_MAX_VOLUME));
    Mix_VolumeMusic((int)(volume * MIX_MAX_VOLUME));
}
