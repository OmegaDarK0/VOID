#include <stdalign.h>
#include "internal.h"
#include "void.h"

VoidSound* void_audio_load_sound(const char *filepath) {
    const VoidScope scope = void_arena_push(NULL);
    VoidSound *sound = void_arena_alloc(NULL, sizeof(VoidSound), alignof(VoidSound));
    if (!sound) return NULL;
    sound->handle = Mix_LoadWAV(filepath);
    if (!sound->handle) {
        VOID_LOG_ERROR("Mix_LoadWAV failed for %s: %s", filepath, Mix_GetError());
        void_arena_pop(NULL, scope);
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
    const VoidScope scope = void_arena_push(NULL);
    VoidMusic *music = void_arena_alloc(NULL, sizeof(VoidMusic), alignof(VoidMusic));
    if (!music) return NULL;
    music->handle = Mix_LoadMUS(filepath);
    if (!music->handle) {
        VOID_LOG_ERROR("Mix_LoadMUS failed for %s: %s", filepath, Mix_GetError());
        void_arena_pop(NULL, scope);
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
