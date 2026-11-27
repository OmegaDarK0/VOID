#include <SDL2/SDL.h>
#include "void.h"

#define MAX_DELTA_TIME 0.05f

static uint64 s_last_time = 0;
static double s_perf_freq = 0.0;

uint64 void_time_get_ticks(void) {
    return SDL_GetTicks64();
}

uint64 void_time_get_counter(void) {
    return SDL_GetPerformanceCounter();
}

uint64 void_time_get_frequency(void) {
    return SDL_GetPerformanceFrequency();
}

float void_time_get_delta(void) {
    const uint64 current_time = void_time_get_counter();
    if (s_perf_freq == 0.0) {
        s_perf_freq = (double)void_time_get_frequency();
    }
    if (s_last_time == 0) {
        s_last_time = current_time;
        return 0.0f;
    }
    float delta_time = (float)((double)(current_time - s_last_time) / s_perf_freq);
    if (delta_time > MAX_DELTA_TIME) {
        delta_time = MAX_DELTA_TIME;
    }
    s_last_time = current_time;
    return delta_time;
}