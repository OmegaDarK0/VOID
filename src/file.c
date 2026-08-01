#include <stdio.h>
#include "void.h"

void* void_file_read(const char *filepath, uint64 *out_size) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        VOID_LOG_ERROR("Failed to open file: %s", filepath);
        if (out_size) *out_size = 0;
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    const uint64 size = ftell(file);
    fseek(file, 0, SEEK_SET);
    const VoidArenaMarker marker = void_arena_get_marker();
    void *buffer = void_arena_alloc(size + 1, 1);
    if (!buffer) {
        VOID_LOG_ERROR("Arena allocation failed for file: %s (%" PRIu64 " bytes)", filepath, size);
        fclose(file);
        if (out_size) *out_size = 0;
        return NULL;
    }
    const size_t read_bytes = fread(buffer, 1, size, file);
    fclose(file);
    if (read_bytes != size) {
        VOID_LOG_ERROR("Failed to read entire file: %s", filepath);
        void_arena_rollback_to(marker);
        if (out_size) *out_size = 0;
        return NULL;
    }
    ((char*)buffer)[size] = '\0';
    if (out_size) *out_size = size;
    VOID_LOG_INFO("File loaded: %s (%" PRIu64 " bytes)", filepath, size);
    return buffer;
}
