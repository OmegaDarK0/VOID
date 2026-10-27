#include <SDL2/SDL.h>
#include "void.h"

#define MAX_DELTA_TIME 0.05f

static uint64 s_last_time = 0;

uint64 void_time_get_ticks(void) {
    return SDL_GetTicks64();
}

float void_time_get_delta(void) {
    const uint64 current_time = SDL_GetPerformanceCounter();
    if (s_last_time == 0) {
        s_last_time = current_time;
        return 0.0f;
    }
    float delta_time = (float)(current_time - s_last_time) / (float)SDL_GetPerformanceFrequency();
    if (delta_time > MAX_DELTA_TIME) {
        delta_time = MAX_DELTA_TIME;
    }
    s_last_time = current_time;
    return delta_time;
}