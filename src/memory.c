#include <stdlib.h>
#include "void.h"

#define MEMORY_PRINT_FACTOR 25

struct VoidArena {
    uint8 *base;
    uint64 capacity, offset;
};

static VoidArena s_global_arena, s_frame_arena;

void void_memory_print(void) {
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
}

bool void_memory_init(const uint64 global_size, const uint64 frame_size) {
    s_global_arena.base = (uint8*)malloc(global_size + frame_size);
    if (s_global_arena.base == NULL) {
        VOID_LOG_FATAL("Memory is NULL!");
        return false;
    }
    s_global_arena.capacity = global_size;
    s_global_arena.offset = 0;
    s_frame_arena.base = s_global_arena.base + global_size;
    s_frame_arena.capacity = frame_size;
    s_frame_arena.offset = 0;
    VOID_LOG_OK("Memory allocated!");
    return true;
}

void void_memory_exit(void) {
    free(s_global_arena.base);
    s_global_arena.base = NULL;
    s_frame_arena.base = NULL;
    VOID_LOG_INFO("Memory free.");
}

void *void_arena_alloc(const uint64 size, const uint32 alignment) {
    const uintptr_t address = (uintptr_t)s_global_arena.base + s_global_arena.offset;
    const uintptr_t aligned_address = (address + (alignment - 1)) & ~(uintptr_t)(alignment - 1);
    const uint64 padding = aligned_address - address;
    if (s_global_arena.offset + padding + size > s_global_arena.capacity) return NULL;
    s_global_arena.offset += padding + size;
    return (void*)aligned_address;
}

VoidArenaMarker void_arena_get_marker(void) {
    return s_global_arena.offset;
}

void void_arena_rollback_to(const VoidArenaMarker marker) {
    if (marker <= s_global_arena.offset) {
        s_global_arena.offset = marker;
    }
}

void *void_frame_alloc(const uint64 size, const uint32 alignment) {
    const uintptr_t address = (uintptr_t)s_frame_arena.base + s_frame_arena.offset;
    const uintptr_t aligned_address = (address + (alignment - 1)) & ~((uintptr_t)alignment - 1);
    const uint64 padding = aligned_address - address;
    if (s_frame_arena.offset + padding + size > s_frame_arena.capacity) return NULL;
    s_frame_arena.offset += padding + size;
    return (void*)aligned_address;
}

VoidArenaMarker void_frame_get_marker(void) {
    return s_frame_arena.offset;
}

void void_frame_rollback_to(const VoidArenaMarker marker) {
    if (marker <= s_frame_arena.offset) {
        s_frame_arena.offset = marker;
    }
}

void void_frame_free(void) {
    s_frame_arena.offset = 0;
}
