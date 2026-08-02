#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif

#include "void.h"

void* void_file_read(const char *filepath, uint64 *out_size) {
#ifdef _WIN32
    HANDLE file = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        VOID_LOG_ERROR("Failed to open file: %s", filepath);
        if (out_size) *out_size = 0;
        return NULL;
    }
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file, &file_size)) {
        VOID_LOG_ERROR("Failed to get file size: %s", filepath);
        CloseHandle(file);
        if (out_size) *out_size = 0;
        return NULL;
    }
    const uint64 size = (uint64)file_size.QuadPart;
    const VoidArenaMarker marker = void_arena_get_marker();
    void *buffer = void_arena_alloc(size + 1, 1);
    if (!buffer) {
        VOID_LOG_ERROR("Arena allocation failed for file: %s (%" PRIu64 " bytes)", filepath, size);
        CloseHandle(file);
        if (out_size) *out_size = 0;
        return NULL;
    }
    uint64 total_read = 0;
    char *ptr = (char*)buffer;
    while (total_read < size) {
        uint64 remaining = size - total_read;
        DWORD bytes_to_read = (remaining > 0xFFFFFFFF) ? 0xFFFFFFFF : (DWORD)remaining;
        DWORD bytes_read = 0;
        if (!ReadFile(file, ptr, bytes_to_read, &bytes_read, NULL) || bytes_read == 0) {
            VOID_LOG_ERROR("Failed to read entire file: %s", filepath);
            void_arena_rollback_to(marker);
            CloseHandle(file);
            if (out_size) *out_size = 0;
            return NULL;
        }
        total_read += bytes_read;
        ptr += bytes_read;
    }
    CloseHandle(file);
#else
    const int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        VOID_LOG_ERROR("Failed to open file: %s", filepath);
        if (out_size) *out_size = 0;
        return NULL;
    }

    struct stat statbuf;
    if (fstat(fd, &statbuf) < 0) {
        VOID_LOG_ERROR("Failed to get file size: %s", filepath);
        close(fd);
        if (out_size) *out_size = 0;
        return NULL;
    }
    const uint64 size = (uint64)statbuf.st_size;
    const VoidArenaMarker marker = void_arena_get_marker();
    void *buffer = void_arena_alloc(size + 1, 1);
    if (!buffer) {
        VOID_LOG_ERROR("Arena allocation failed for file: %s (%" PRIu64 " bytes)", filepath, size);
        close(fd);
        if (out_size) *out_size = 0;
        return NULL;
    }
    uint64 total_read = 0;
    char *ptr = buffer;
    while (total_read < size) {
        const ssize_t bytes_read = read(fd, ptr, size - total_read);
        if (bytes_read <= 0) {
            VOID_LOG_ERROR("Failed to read entire file: %s", filepath);
            void_arena_rollback_to(marker);
            close(fd);
            if (out_size) *out_size = 0;
            return NULL;
        }
        total_read += (uint64)bytes_read;
        ptr += bytes_read;
    }
    close(fd);
#endif
    ((char*)buffer)[size] = '\0';
    if (out_size) *out_size = size;
    VOID_LOG_INFO("File loaded: %s (%" PRIu64 " bytes)", filepath, size);
    return buffer;
}
