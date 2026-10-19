#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdalign.h>
#include "void.h"

#define MEMORY_PRINT_FACTOR 32
#define MAX_REGISTERED_ARENAS 16

static VoidArena s_root_arena;
static VoidArena *s_default_arena = NULL;
static VoidArena *s_registered_arenas[MAX_REGISTERED_ARENAS];
static uint32 s_arena_count = 0;
static uint64 s_last_monitor_tick = 0;

static void arena_lock(VoidArena *arena) {
    while (!void_atomic_compare_exchange(&arena->lock, 0, 1));
}

static void arena_unlock(VoidArena *arena) {
    arena->lock = 0;
}

static void format_memory_size(const uint64 bytes, char *out_buf, const size_t buf_size) {
    if (bytes >= GiB) {
        snprintf(out_buf, buf_size, "%.2f GiB", (double)bytes / (double)GiB);
    } else if (bytes >= MiB) {
        snprintf(out_buf, buf_size, "%.2f MiB", (double)bytes / (double)MiB);
    } else if (bytes >= KiB) {
        snprintf(out_buf, buf_size, "%.2f KiB", (double)bytes / (double)KiB);
    } else {
        snprintf(out_buf, buf_size, "%" PRIu64 " B", bytes);
    }
}

// ============================================================================
// LIFECYCLE & TARGET MANAGEMENT
// ============================================================================

VoidArena *void_memory_init(const uint64 total_capacity) {
#ifdef _WIN32
    uint8 *memory_block = VirtualAlloc(NULL, total_capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (memory_block == NULL) {
#else
    uint8 *memory_block = mmap(NULL, total_capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory_block == MAP_FAILED) {
#endif
        VOID_LOG_FATAL("Failed to allocate OS virtual memory!");
        return NULL;
    }
    s_root_arena.name = "ROOT_MASTER";
    s_root_arena.base = memory_block;
    s_root_arena.capacity = total_capacity;
    s_root_arena.offset = 0;
    s_root_arena.lock = 0;
    s_default_arena = &s_root_arena;
    s_arena_count = 0;
    s_registered_arenas[s_arena_count++] = &s_root_arena;
    char size_str[32];
    format_memory_size(total_capacity, size_str, sizeof(size_str));
    VOID_LOG_OK("Virtual memory initialized! [%s]", size_str);
    return &s_root_arena;
}

void void_memory_exit(void) {
    if (s_root_arena.base) {
#ifdef _WIN32
        VirtualFree(s_root_arena.base, 0, MEM_RELEASE);
#else
        munmap(s_root_arena.base, s_root_arena.capacity);
#endif
        s_root_arena.base = NULL;
        s_default_arena   = NULL;
        s_arena_count     = 0;
        VOID_LOG_INFO("Virtual memory freed.");
    }
}

void void_memory_set_default_arena(VoidArena *arena) {
    s_default_arena = arena ? arena : &s_root_arena;
}

VoidArena *void_memory_get_default_arena(void) {
    return s_default_arena;
}

VoidArena *void_memory_get_root_arena(void) {
    return &s_root_arena;
}

// ============================================================================
// ALLOCATION, SUBALLOCATION & REALLOC
// ============================================================================

void *void_arena_alloc(VoidArena *arena, const uint64 size, const uint32 alignment) {
    if (size == 0) return NULL;
    VoidArena *target = arena ? arena : s_default_arena;
    const uint32 align = alignment > 0 ? alignment : 1;
    arena_lock(target);
    const uintptr_t address = (uintptr_t)target->base + target->offset;
    const uintptr_t aligned_address = (address + (align - 1)) & ~(uintptr_t)(align - 1);
    const uint64 padding = aligned_address - address;
    if (target->offset + padding + size > target->capacity) {
        arena_unlock(target);
        VOID_LOG_ERROR("Arena '%s' Out of Memory! (Requested: %" PRIu64 " bytes)", target->name, size);
        return NULL;
    }
    target->offset += padding + size;
    arena_unlock(target);
    return (void*)aligned_address;
}

VoidArena *void_arena_suballoc(VoidArena *parent, const char *name, const uint64 capacity) {
    VoidArena *target_parent = parent ? parent : &s_root_arena;
    const VoidScope scope = void_arena_push(target_parent);
    VoidArena *sub = void_arena_alloc(target_parent, sizeof(VoidArena), alignof(VoidArena));
    uint8 *block = void_arena_alloc(target_parent, capacity, 16);
    if (!sub || !block) {
        void_arena_pop(target_parent, scope);
        VOID_LOG_ERROR("Failed to suballocate arena '%s'!", name ? name : "UNNAMED");
        return NULL;
    }
    sub->name = name ? name : "UNNAMED";
    sub->base = block;
    sub->capacity = capacity;
    sub->offset = 0;
    sub->lock = 0;
    if (s_arena_count < MAX_REGISTERED_ARENAS) {
        s_registered_arenas[s_arena_count++] = sub;
    }
    return sub;
}

void *void_arena_realloc(VoidArena *arena, void *ptr, const uint64 old_size, const uint64 new_size, const uint32 alignment) {
    VoidArena *target = arena ? arena : s_default_arena;
    if (!ptr) return void_arena_alloc(target, new_size, alignment);
    if (new_size == 0) return NULL;
    arena_lock(target);
    const uintptr_t addr = (uintptr_t)ptr;
    const uintptr_t base = (uintptr_t)target->base;
    const uintptr_t top  = base + target->offset;
    // Fast-path: In-place resize if the block is at the very top of the arena
    if (addr >= base && addr + old_size == top) {
        const uint64 base_offset = (uint64)(addr - base);
        if (base_offset + new_size <= target->capacity) {
            target->offset = base_offset + new_size;
            arena_unlock(target);
            return ptr;
        }
    }
    arena_unlock(target);
    // Fallback: Allocate a new block and copy existing payload
    void *new_ptr = void_arena_alloc(target, new_size, alignment);
    if (!new_ptr) return NULL;
    const uint64 copy_size = (old_size < new_size) ? old_size : new_size;
    if (copy_size > 0) {
        memcpy(new_ptr, ptr, copy_size);
    }
    return new_ptr;
}

// ============================================================================
// DATA PROMOTION, MOVE & STRING UTILITIES
// ============================================================================

void *void_arena_memdup(VoidArena *dst_arena, const void *src_ptr, const uint64 size, const uint32 alignment) {
    if (!src_ptr || size == 0) return NULL;
    void *dst = void_arena_alloc(dst_arena, size, alignment);
    if (!dst) return NULL;
    memcpy(dst, src_ptr, size);
    return dst;
}

void *void_arena_memmove(VoidArena *dst_arena, VoidArena *src_arena, void *src_ptr, const uint64 size, const uint32 alignment) {
    if (!src_ptr || size == 0) return NULL;
    VoidArena *target_dst = dst_arena ? dst_arena : s_default_arena;
    VoidArena *target_src = src_arena ? src_arena : s_default_arena;
    if (target_dst == target_src) return src_ptr;
    void *dst_ptr = void_arena_memdup(target_dst, src_ptr, size, alignment);
    if (!dst_ptr) return NULL;
    // Reclaim memory immediately if src_ptr was at the top of target_src
    arena_lock(target_src);
    const uintptr_t src_addr = (uintptr_t)src_ptr;
    const uintptr_t base     = (uintptr_t)target_src->base;
    const uintptr_t top_addr = base + target_src->offset;
    if (src_addr >= base && src_addr + size == top_addr) {
        target_src->offset = src_addr - base;
    }
    arena_unlock(target_src);
    return dst_ptr;
}

char *void_arena_strdup(VoidArena *dst_arena, const char *str) {
    if (!str) return NULL;
    const uint64 len = strlen(str) + 1;
    return void_arena_memdup(dst_arena, str, len, 1);
}

char *void_arena_strndup(VoidArena *dst_arena, const char *str, const uint64 max_len) {
    if (!str) return NULL;
    uint64 len = 0;
    while (len < max_len && str[len] != '\0') {
        len++;
    }
    char *dst = void_arena_alloc(dst_arena, len + 1, 1);
    if (!dst) return NULL;
    if (len > 0) {
        memcpy(dst, str, len);
    }
    dst[len] = '\0';
    return dst;
}

char *void_arena_sprintf(VoidArena *dst_arena, const char *fmt, ...) {
    if (!fmt) return NULL;
    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    const int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len < 0) {
        va_end(args_copy);
        return NULL;
    }
    char *buf = void_arena_alloc(dst_arena, (uint64)len + 1, 1);
    if (buf) {
        vsnprintf(buf, (size_t)len + 1, fmt, args_copy);
    }
    va_end(args_copy);
    return buf;
}

// ============================================================================
// SCOPES & RESET
// ============================================================================

void void_arena_reset(VoidArena *arena) {
    VoidArena *target = arena ? arena : s_default_arena;
    arena_lock(target);
    target->offset = 0;
    arena_unlock(target);
}

VoidScope void_arena_push(const VoidArena *arena) {
    VoidArena *target = (VoidArena*)(arena ? arena : s_default_arena);
    arena_lock(target);
    const VoidScope scope = { .offset_backup = target->offset };
    arena_unlock(target);
    return scope;
}

void void_arena_pop(VoidArena *arena, const VoidScope scope) {
    VoidArena *target = arena ? arena : s_default_arena;
    arena_lock(target);
    if (scope.offset_backup <= target->offset) {
        target->offset = scope.offset_backup;
    }
    arena_unlock(target);
}

// ============================================================================
// TELEMETRY & REAL-TIME MONITORING
// ============================================================================

void void_arena_print(const VoidArena *arena) {
    VoidArena *target = (VoidArena*)(arena ? arena : s_default_arena);
    arena_lock(target);
    const uint64 offset = target->offset;
    const uint64 capacity = target->capacity;
    const char *name = target->name;
    arena_unlock(target);
    char bar[MEMORY_PRINT_FACTOR + 1];
    bar[MEMORY_PRINT_FACTOR] = '\0';
    const uint8 filled = capacity > 0 ? (uint8)(MEMORY_PRINT_FACTOR * offset / capacity) : 0;
    for (uint8 i = 0; i < MEMORY_PRINT_FACTOR; i++) {
        bar[i] = i < filled ? '#' : '.';
    }
    const double pct = capacity > 0 ? (double)offset * 100.0 / (double)capacity : 0.0;
    char used_str[32], cap_str[32];
    format_memory_size(offset, used_str, sizeof(used_str));
    format_memory_size(capacity, cap_str, sizeof(cap_str));
    VOID_LOG_DEBUG("[%-11s] [%s] %6.2f%% (%s / %s)", name, bar, pct, used_str, cap_str);
}

void void_memory_print(void) {
    for (uint32 i = 0; i < s_arena_count; i++) {
        void_arena_print(s_registered_arenas[i]);
    }
}

void void_memory_monitor(const uint32 interval_ms) {
    const uint64 now = void_time_get_ticks();
    if (now - s_last_monitor_tick >= interval_ms) {
        s_last_monitor_tick = now;
        void_memory_print();
    }
}