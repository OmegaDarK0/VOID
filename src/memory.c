#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <stddef.h>
#endif

#include "void.h"

#define MEMORY_PRINT_FACTOR 25

struct VoidArena {
    uint8 *base;
    uint64 capacity, offset;
};

static VoidArena s_global_arena, s_frame_arena;
static volatile uint32 s_arena_lock = 0;

static void void_memory_lock(void) {
    while (!void_atomic_compare_exchange(&s_arena_lock, 0, 1));
}

static void void_memory_unlock(void) {
    s_arena_lock = 0;
}

void void_memory_print(void) {
    void_memory_lock();
    char buffer[MEMORY_PRINT_FACTOR + 1];
    buffer[MEMORY_PRINT_FACTOR] = '\0';
    const uint8 global_alloc = s_global_arena.capacity > 0 ? MEMORY_PRINT_FACTOR * s_global_arena.offset / s_global_arena.capacity : 0;
    const uint8 global_free = MEMORY_PRINT_FACTOR - global_alloc;
    for (uint8 i = 0; i < global_alloc; i++) buffer[i] = '#';
    for (uint8 i = 0; i < global_free; i++) buffer[global_alloc + i] = '.';
    VOID_LOG_DEBUG("[GLOBAL] %s (%" PRIu64 " / %" PRIu64 ")", buffer, s_global_arena.offset, s_global_arena.capacity);
    const uint8 frame_alloc = s_frame_arena.capacity > 0 ? MEMORY_PRINT_FACTOR * s_frame_arena.offset / s_frame_arena.capacity : 0;
    const uint8 frame_free = MEMORY_PRINT_FACTOR - frame_alloc;
    for (uint8 i = 0; i < frame_alloc; i++) buffer[i] = '#';
    for (uint8 i = 0; i < frame_free; i++) buffer[frame_alloc + i] = '.';
    VOID_LOG_DEBUG("[FRAME]  %s (%" PRIu64 " / %" PRIu64 ")", buffer, s_frame_arena.offset, s_frame_arena.capacity);
    void_memory_unlock();
}

bool void_memory_init(const uint64 global_size, const uint64 frame_size) {
    const uint64 total_size = global_size + frame_size;
#ifdef _WIN32
    uint8 *memory_block = VirtualAlloc(NULL, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (memory_block == NULL) {
#else
    uint8 *memory_block = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory_block == MAP_FAILED) {
#endif
        VOID_LOG_FATAL("Failed to allocate virtual memory!");
        return false;
    }
    s_global_arena.base = memory_block;
    s_global_arena.capacity = global_size;
    s_global_arena.offset = 0;
    s_frame_arena.base = memory_block + global_size;
    s_frame_arena.capacity = frame_size;
    s_frame_arena.offset = 0;
    VOID_LOG_OK("Virtual memory allocated! (%" PRIu64 " bytes)", total_size);
    return true;
}

void void_memory_exit(void) {
    if (s_global_arena.base) {
#ifdef _WIN32
        VirtualFree(s_global_arena.base, 0, MEM_RELEASE);
#else
        munmap(s_global_arena.base,  s_global_arena.capacity + s_frame_arena.capacity);
#endif
        s_global_arena.base = NULL;
        s_frame_arena.base = NULL;
        VOID_LOG_INFO("Virtual memory freed.");
    }
}

void *void_arena_alloc(const uint64 size, const uint32 alignment) {
    void_memory_lock();
    const uintptr_t address = (uintptr_t)s_global_arena.base + s_global_arena.offset;
    const uintptr_t aligned_address = (address + (alignment - 1)) & ~(uintptr_t)(alignment - 1);
    const uint64 padding = aligned_address - address;
    if (s_global_arena.offset + padding + size > s_global_arena.capacity) {
        void_memory_unlock();
        return NULL;
    }
    s_global_arena.offset += padding + size;
    void_memory_unlock();
    return (void*)aligned_address;
}

VoidArenaMarker void_arena_get_marker(void) {
    void_memory_lock();
    const VoidArenaMarker marker = s_global_arena.offset;
    void_memory_unlock();
    return marker;
}

void void_arena_rollback_to(const VoidArenaMarker marker) {
    void_memory_lock();
    if (marker <= s_global_arena.offset) {
        s_global_arena.offset = marker;
    }
    void_memory_unlock();
}

void *void_frame_alloc(const uint64 size, const uint32 alignment) {
    void_memory_lock();
    const uintptr_t address = (uintptr_t)s_frame_arena.base + s_frame_arena.offset;
    const uintptr_t aligned_address = (address + (alignment - 1)) & ~((uintptr_t)alignment - 1);
    const uint64 padding = aligned_address - address;
    if (s_frame_arena.offset + padding + size > s_frame_arena.capacity) {
        void_memory_unlock();
        return NULL;
    }
    s_frame_arena.offset += padding + size;
    void_memory_unlock();
    return (void*)aligned_address;
}

VoidArenaMarker void_frame_get_marker(void) {
    void_memory_lock();
    const VoidArenaMarker marker = s_frame_arena.offset;
    void_memory_unlock();
    return marker;
}

void void_frame_rollback_to(const VoidArenaMarker marker) {
    void_memory_lock();
    if (marker <= s_frame_arena.offset) {
        s_frame_arena.offset = marker;
    }
    void_memory_unlock();
}

void void_frame_free(void) {
    void_memory_lock();
    s_frame_arena.offset = 0;
    void_memory_unlock();
}
