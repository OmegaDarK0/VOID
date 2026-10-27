#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif

#include "void.h"

void* void_file_read(const char *filepath, uint64 *out_size) {
    if (!filepath) return NULL;
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
    const VoidScope scope = void_arena_push(NULL);
    void *buffer = void_arena_alloc(NULL, size + 1, 1);
    if (!buffer) {
        VOID_LOG_ERROR("Arena allocation failed for file: %s (%" PRIu64 " bytes)", filepath, size);
        CloseHandle(file);
        if (out_size) *out_size = 0;
        return NULL;
    }
    uint64 total_read = 0;
    char *ptr = (char*)buffer;
    while (total_read < size) {
        const uint64 remaining = size - total_read;
        const DWORD bytes_to_read = (remaining > 0xFFFFFFFF) ? 0xFFFFFFFF : (DWORD)remaining;
        DWORD bytes_read = 0;
        if (!ReadFile(file, ptr, bytes_to_read, &bytes_read, NULL) || bytes_read == 0) {
            VOID_LOG_ERROR("Failed to read entire file: %s", filepath);
            void_arena_pop(NULL, scope);
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
    const VoidScope scope = void_arena_push(NULL);
    void *buffer = void_arena_alloc(NULL, size + 1, 1);
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
            void_arena_pop(NULL, scope);
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

bool void_file_write(const char *filepath, const void *data, const uint64 size) {
    if (!filepath || (!data && size > 0)) return false;
#ifdef _WIN32
    HANDLE file = CreateFileA(filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        VOID_LOG_ERROR("Failed to open file for writing: %s", filepath);
        return false;
    }
    uint64 total_written = 0;
    const char *ptr = (const char*)data;
    while (total_written < size) {
        const uint64 remaining = size - total_written;
        const DWORD bytes_to_write = (remaining > 0xFFFFFFFF) ? 0xFFFFFFFF : (DWORD)remaining;
        DWORD bytes_written = 0;
        if (!WriteFile(file, ptr, bytes_to_write, &bytes_written, NULL) || bytes_written == 0) {
            VOID_LOG_ERROR("Failed to write file: %s", filepath);
            CloseHandle(file);
            return false;
        }
        total_written += bytes_written;
        ptr += bytes_written;
    }
    CloseHandle(file);
#else
    const int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        VOID_LOG_ERROR("Failed to open file for writing: %s", filepath);
        return false;
    }
    uint64 total_written = 0;
    const char *ptr = (const char*)data;
    while (total_written < size) {
        const ssize_t bytes_written = write(fd, ptr, size - total_written);
        if (bytes_written <= 0) {
            VOID_LOG_ERROR("Failed to write file: %s", filepath);
            close(fd);
            return false;
        }
        total_written += (uint64)bytes_written;
        ptr += bytes_written;
    }
    close(fd);
#endif
    return true;
}

bool void_file_exists(const char *filepath) {
    if (!filepath) return false;
#ifdef _WIN32
    const DWORD attrs = GetFileAttributesA(filepath);
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat statbuf;
    return (stat(filepath, &statbuf) == 0) && S_ISREG(statbuf.st_mode);
#endif
}

uint64 void_file_get_mtime(const char *filepath) {
    if (!filepath) return 0;
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(filepath, GetFileExInfoStandard, &data)) {
        return 0;
    }
    ULARGE_INTEGER ull;
    ull.LowPart  = data.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = data.ftLastWriteTime.dwHighDateTime;
    return (uint64)ull.QuadPart;
#else
    struct stat statbuf;
    if (stat(filepath, &statbuf) != 0) {
        return 0;
    }
    return (uint64)statbuf.st_mtime;
#endif
}

bool void_file_copy(const char *src_path, const char *dst_path) {
    if (!src_path || !dst_path) return false;
#ifdef _WIN32
    return CopyFileA(src_path, dst_path, FALSE) != 0;
#else
    const VoidScope scope = void_arena_push(NULL);
    uint64 size = 0;
    void *data = void_file_read(src_path, &size);
    if (!data) {
        void_arena_pop(NULL, scope);
        return false;
    }
    const bool ok = void_file_write(dst_path, data, size);
    void_arena_pop(NULL, scope);
    return ok;
#endif
}