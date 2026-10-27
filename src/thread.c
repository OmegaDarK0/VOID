#include <SDL2/SDL.h>
#include "void.h"

#define MAX_WORKER_THREADS 64

static VoidThread s_threads[MAX_WORKER_THREADS];
static volatile uint32 s_thread_count = 0;

static int void_thread_runner(void *ptr) {
    const VoidThread *thread = (VoidThread*)ptr;
    if (thread && thread->func) {
        thread->func(thread->data);
    }
    return true;
}

void void_thread_create(const VoidThreadFunc func, void *data) {
    const uint32 index = __sync_fetch_and_add(&s_thread_count, 1);
    if (index >= MAX_WORKER_THREADS) return;
    s_threads[index].func = func;
    s_threads[index].data = data;
    SDL_Thread *thread = SDL_CreateThread(void_thread_runner, "VoidWorker", &s_threads[index]);
    if (thread) {
        SDL_DetachThread(thread);
    }
}

void void_thread_sleep(const uint32 ms) {
    SDL_Delay(ms);
}

uint32 void_thread_get_id(void) {
    return SDL_GetThreadID(NULL);
}

uint32 void_atomic_increment(volatile uint32 *value) {
    return __sync_add_and_fetch(value, 1);
}

uint32 void_atomic_decrement(volatile uint32 *value) {
    return __sync_sub_and_fetch(value, 1);
}

bool void_atomic_compare(volatile uint32 *value, const uint32 expected, const uint32 desired) {
    if (__sync_bool_compare_and_swap(value, expected, desired)) {
        return true;
    }
    return false;
}

void void_processor_pause(void) {
    SDL_CPUPauseInstruction();
}
