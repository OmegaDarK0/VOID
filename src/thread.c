#include <SDL2/SDL.h>
#include "internal.h"
#include "void.h"

#define MAX_WORKER_THREADS 64

typedef struct {
    VoidThread thread;
    volatile uint32 in_use;
} VoidThreadSlot;

static VoidThreadSlot s_slots[MAX_WORKER_THREADS];

static int void_thread_runner(void *ptr) {
    VoidThreadSlot *slot = ptr;
    const VoidThreadFunc func = slot->thread.func;
    void *data = slot->thread.data;
    void_atomic_store(&slot->in_use, 0);
    if (func) {
        func(data);
    }
    return 0;
}

void void_thread_create(const VoidThreadFunc func, void *data) {
    if (!func) return;
    for (uint32 i = 0; i < MAX_WORKER_THREADS; ++i) {
        if (void_atomic_compare(&s_slots[i].in_use, 0, 1)) {
            s_slots[i].thread.func = func;
            s_slots[i].thread.data = data;
            SDL_Thread *thread = SDL_CreateThread(void_thread_runner, "VoidWorker", &s_slots[i]);
            if (thread) {
                SDL_DetachThread(thread);
            } else {
                void_atomic_store(&s_slots[i].in_use, 0);
                VOID_LOG_ERROR("SDL_CreateThread failed: %s", SDL_GetError());
            }
            return;
        }
    }
    VOID_LOG_ERROR("Failed to create thread: all %d spawn slots are busy.", MAX_WORKER_THREADS);
}

void void_thread_sleep(const uint32 ms) {
    SDL_Delay(ms);
}

uint32 void_thread_get_id(void) {
    return (uint32)SDL_GetThreadID(NULL);
}

uint32 void_atomic_load(const volatile uint32 *value) {
    return __atomic_load_n(value, __ATOMIC_ACQUIRE);
}

void void_atomic_store(volatile uint32 *value, const uint32 desired) {
    __atomic_store_n(value, desired, __ATOMIC_RELEASE);
}

uint32 void_atomic_increment(volatile uint32 *value) {
    return __atomic_add_fetch(value, 1, __ATOMIC_ACQ_REL);
}

uint32 void_atomic_decrement(volatile uint32 *value) {
    return __atomic_sub_fetch(value, 1, __ATOMIC_ACQ_REL);
}

bool void_atomic_compare(volatile uint32 *value, uint32 expected, const uint32 desired) {
    return __atomic_compare_exchange_n(value, &expected, desired, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

void void_processor_pause(void) {
    SDL_CPUPauseInstruction();
}