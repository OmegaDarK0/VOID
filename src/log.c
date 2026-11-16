#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "void.h"
#include "internal.h"

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #define VOID_GETPID() ((uint32)GetCurrentProcessId())
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #define VOID_GETPID() ((uint32)getpid())
#endif

#if defined(_WIN32)
    #define VOID_PLATFORM_OS "Windows"
#elif defined(__linux__)
    #define VOID_PLATFORM_OS "Linux"
#elif defined(__APPLE__)
    #define VOID_PLATFORM_OS "macOS"
#else
    #define VOID_PLATFORM_OS "POSIX"
#endif

#if defined(__x86_64__) || defined(_M_X64)
    #define VOID_PLATFORM_ARCH "x86_64"
#elif defined(__i386__) || defined(_M_IX86)
    #define VOID_PLATFORM_ARCH "x86"
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define VOID_PLATFORM_ARCH "ARM64"
#else
    #define VOID_PLATFORM_ARCH "Unknown"
#endif

#if defined(__clang__)
    #define VOID_COMPILER_NAME "Clang " __clang_version__
#elif defined(__GNUC__)
    #define VOID_COMPILER_NAME "GCC " __VERSION__
#elif defined(_MSC_VER)
    #define VOID_COMPILER_NAME "MSVC"
#else
    #define VOID_COMPILER_NAME "Standard C Compiler"
#endif

#define MAX_LOG_ENTRIES      1024
#define MAX_LOG_LENGTH       256
#define MAX_LOG_PATH         512
#define MAX_FULL_PATH        1024
#define MAX_APP_NAME         64
#define MAX_LOG_FILE_BYTES   MiB

#define NC      "\x1b[0m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define SCARLET "\x1b[37;41;1m"

static VoidLogEntry s_log_buffer[MAX_LOG_ENTRIES];

static volatile uint32 s_log_head = 0;
static volatile uint32 s_log_tail = 0;

static FILE *s_log_file = NULL;
static uint64 s_current_file_bytes = 0;
static bool s_file_initialized = false;
static bool s_color_enabled = true;
#ifdef _WIN32
static HANDLE s_console_handle = INVALID_HANDLE_VALUE;
static WORD s_default_console_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
static bool s_use_win32_console_attr = false;
#endif

static char s_app_name[MAX_APP_NAME] = NAME;
static bool s_app_name_custom = false;
static char s_config_dir[MAX_LOG_PATH] = "logs";
static uint32 s_max_files = 10;
static bool s_hidden_dir = true;
static VoidLogLevel  s_min_level = VOID_LOG_DEBUG;
static VoidLogTarget s_targets = VOID_LOG_TARGET_ALL;

static void detect_app_name(char *out_name, const size_t out_size) {
    if (!out_name || out_size == 0) return;
#ifdef _WIN32
    char exe_path[MAX_PATH];
    const DWORD len = GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        snprintf(out_name, out_size, "%s", NAME);
        return;
    }
#else
    char exe_path[MAX_FULL_PATH];
    const ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len <= 0) {
        snprintf(out_name, out_size, "%s", NAME);
        return;
    }
    exe_path[len] = '\0';
#endif
    const char *base = exe_path;
    for (const char *p = exe_path; *p; ++p) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    size_t name_len = strlen(base);
    if (name_len >= out_size) name_len = out_size - 1;
    memcpy(out_name, base, name_len);
    out_name[name_len] = '\0';
    if (name_len > 4 && strcmp(out_name + name_len - 4, ".exe") == 0) {
        out_name[name_len - 4] = '\0';
    }
}

static void resolve_log_dir(const char *raw_dir, const bool hidden, char *out_dir, const size_t out_size) {
    const char *src = raw_dir && raw_dir[0] != '\0' ? raw_dir : "logs";
    char cleaned[MAX_LOG_PATH];
    snprintf(cleaned, sizeof(cleaned), "%s", src);
    size_t len = strlen(cleaned);
    while (len > 1 && (cleaned[len - 1] == '/' || cleaned[len - 1] == '\\')) {
        cleaned[--len] = '\0';
    }
#ifdef _WIN32
    (void)hidden;
    snprintf(out_dir, out_size, "%s", cleaned);
#else
    char *last_sep = NULL;
    for (char *p = cleaned; *p; ++p) {
        if (*p == '/' || *p == '\\') last_sep = p;
    }
    const char *leaf = last_sep ? (last_sep + 1) : cleaned;
    const bool starts_with_dot = (leaf[0] == '.');
    if (hidden && !starts_with_dot) {
        if (last_sep) {
            *last_sep = '\0';
            snprintf(out_dir, out_size, "%s/.%s", cleaned, leaf);
        } else {
            snprintf(out_dir, out_size, ".%s", leaf);
        }
    } else if (!hidden && starts_with_dot && leaf[1] != '\0') {
        if (last_sep) {
            *last_sep = '\0';
            snprintf(out_dir, out_size, "%s/%s", cleaned, leaf + 1);
        } else {
            snprintf(out_dir, out_size, "%s", leaf + 1);
        }
    } else {
        snprintf(out_dir, out_size, "%s", cleaned);
    }
#endif
}

static void create_log_dir_recursive(const char *dir_path, const bool hidden) {
    char tmp[MAX_LOG_PATH];
    snprintf(tmp, sizeof(tmp), "%s", dir_path);
    const size_t len = strlen(tmp);
    for (size_t i = 1; i < len; ++i) {
        if (tmp[i] == '/' || tmp[i] == '\\') {
            const char sep = tmp[i];
            tmp[i] = '\0';
#ifdef _WIN32
            CreateDirectoryA(tmp, NULL);
#else
            mkdir(tmp, 0755);
#endif
            tmp[i] = sep;
        }
    }
#ifdef _WIN32
    CreateDirectoryA(tmp, NULL);
    const DWORD attrs = GetFileAttributesA(tmp);
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        if (hidden) {
            SetFileAttributesA(tmp, attrs | FILE_ATTRIBUTE_HIDDEN);
        } else if (attrs & FILE_ATTRIBUTE_HIDDEN) {
            SetFileAttributesA(tmp, attrs & ~((DWORD)FILE_ATTRIBUTE_HIDDEN));
        }
    }
#else
    (void)hidden;
    mkdir(tmp, 0755);
#endif
}

static bool is_managed_log_file(const char *filename) {
    if (!filename) return false;
    const size_t prefix_len = strlen(s_app_name);
    const size_t name_len   = strlen(filename);
    if (name_len <= prefix_len + 5) return false;
    if (strncmp(filename, s_app_name, prefix_len) != 0) return false;
    if (filename[prefix_len] != '-') return false;
    return strcmp(filename + name_len - 4, ".log") == 0;
}

static void prune_oldest_logs(const char *resolved_dir, const uint32 max_to_keep) {
    while (true) {
        uint32 count = 0;
        char oldest_name[MAX_LOG_PATH] = {0};
#ifdef _WIN32
        char search_pattern[MAX_FULL_PATH];
        snprintf(search_pattern, sizeof(search_pattern), "%s\\%s-*.log", resolved_dir, s_app_name);
        WIN32_FIND_DATAA find_data;
        HANDLE hFind = FindFirstFileA(search_pattern, &find_data);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    is_managed_log_file(find_data.cFileName)) {
                    count++;
                    if (oldest_name[0] == '\0' || strcmp(find_data.cFileName, oldest_name) < 0) {
                        snprintf(oldest_name, sizeof(oldest_name), "%s", find_data.cFileName);
                    }
                }
            } while (FindNextFileA(hFind, &find_data));
            FindClose(hFind);
        }
#else
        DIR *dir = opendir(resolved_dir);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (is_managed_log_file(entry->d_name)) {
                    count++;
                    if (oldest_name[0] == '\0' || strcmp(entry->d_name, oldest_name) < 0) {
                        snprintf(oldest_name, sizeof(oldest_name), "%s", entry->d_name);
                    }
                }
            }
            closedir(dir);
        }
#endif
        if (count <= max_to_keep || oldest_name[0] == '\0') {
            break;
        }
        char full_oldest_path[MAX_FULL_PATH];
        snprintf(full_oldest_path, sizeof(full_oldest_path), "%s/%s", resolved_dir, oldest_name);
        if (remove(full_oldest_path) != 0) {
            break;
        }
    }
}

static void write_session_header(FILE *file, const struct tm *tm_info, const uint32 pid) {
    if (!file) return;
    char readable_date[64];
    strftime(readable_date, sizeof(readable_date), "%Y-%m-%d %H:%M:%S", tm_info);
#if defined(__STDC_VERSION__)
    const long c_std = __STDC_VERSION__;
#else
    const long c_std = 0L;
#endif
    const int written = fprintf(
        file,
        "================================================================================\n"
        "                              %s - LOG SESSION\n"
        "================================================================================\n"
        "  Application  : %s\n"
        "  Framework    : %s v%s (commit: %s)\n"
        "  Build Mode   : %s (C%ld | %s %s)\n"
        "  Toolchain    : %s\n"
        "  Platform     : %s (%s)\n"
        "  CPU Cores    : %u logical core(s)\n"
        "  Process ID   : %u\n"
        "  Session Date : %s\n"
        "================================================================================\n",
        NAME,
        s_app_name,
        NAME, VERSION, GIT_COMMIT,
        MODE, c_std, __DATE__, __TIME__,
        VOID_COMPILER_NAME,
        VOID_PLATFORM_OS, VOID_PLATFORM_ARCH,
        void_get_core_count(),
        pid,
        readable_date
    );
    if (written > 0) {
        s_current_file_bytes += (uint64)written;
    }
    fflush(file);
}

static void open_dated_log_file(void) {
    if (s_log_file) {
        fclose(s_log_file);
        s_log_file = NULL;
    }
    s_current_file_bytes = 0;
    s_file_initialized = true;
    if (!(s_targets & VOID_LOG_TARGET_FILE) || s_max_files == 0) {
        return;
    }
    char resolved_dir[MAX_LOG_PATH];
    resolve_log_dir(s_config_dir, s_hidden_dir, resolved_dir, sizeof(resolved_dir));
    create_log_dir_recursive(resolved_dir, s_hidden_dir);
    prune_oldest_logs(resolved_dir, s_max_files - 1);
    const time_t now = time(NULL);
    struct tm tm_info;
#ifdef _WIN32
    localtime_s(&tm_info, &now);
#else
    localtime_r(&now, &tm_info);
#endif
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d_%H%M%S", &tm_info);
    const uint32 pid = VOID_GETPID();
    char file_path[MAX_FULL_PATH];
    snprintf(file_path, sizeof(file_path), "%s/%s-%s-pid-%u.log", resolved_dir, s_app_name, date_str, pid);
    uint32 part = 2;
    FILE *probe = fopen(file_path, "r");
    while (probe != NULL && part < 1000) {
        fclose(probe);
        snprintf(file_path, sizeof(file_path), "%s/%s-%s-pid-%u-%u.log", resolved_dir, s_app_name, date_str, pid, part++);
        probe = fopen(file_path, "r");
    }
    if (probe) fclose(probe);
    s_log_file = fopen(file_path, "w");
    if (s_log_file) {
        write_session_header(s_log_file, &tm_info, pid);
    }
}

static void reset_file_state_if_needed(void) {
    if (s_log_file) {
        fclose(s_log_file);
        s_log_file = NULL;
    }
    s_file_initialized = false;
}

void void_log_init(void) {
    if (!s_app_name_custom) {
        detect_app_name(s_app_name, sizeof(s_app_name));
    }
#ifdef _WIN32
    s_color_enabled = false;
    s_use_win32_console_attr = false;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == NULL || hOut == INVALID_HANDLE_VALUE || GetFileType(hOut) == FILE_TYPE_UNKNOWN) {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        }
    }
    if (hOut != NULL && hOut != INVALID_HANDLE_VALUE) {
        s_console_handle = hOut;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
            s_default_console_attr = csbi.wAttributes;
            s_use_win32_console_attr = true;
        } else {
            DWORD fileType = GetFileType(hOut);
            if (fileType == FILE_TYPE_PIPE || fileType == FILE_TYPE_CHAR) {
                s_color_enabled = true;
            }
        }
    }
    setvbuf(stdout, NULL, _IONBF, 0);
#else
    s_color_enabled = isatty(STDOUT_FILENO) != 0;
#endif
}

void void_log_set_app_name(const char *app_name) {
    if (app_name && app_name[0] != '\0') {
        snprintf(s_app_name, sizeof(s_app_name), "%s", app_name);
        s_app_name_custom = true;
    } else {
        s_app_name_custom = false;
        detect_app_name(s_app_name, sizeof(s_app_name));
    }
    reset_file_state_if_needed();
}

void void_log_set_min_level(const VoidLogLevel min_level) {
    s_min_level = min_level;
}

void void_log_set_targets(const VoidLogTarget targets) {
    s_targets = targets;
    if (!(s_targets & VOID_LOG_TARGET_FILE) && s_log_file) {
        fclose(s_log_file);
        s_log_file = NULL;
    }
}

void void_log_set_dir_path(const char *dir_path) {
    if (dir_path && dir_path[0] != '\0') {
        snprintf(s_config_dir, sizeof(s_config_dir), "%s", dir_path);
    } else {
        snprintf(s_config_dir, sizeof(s_config_dir), "logs");
    }
    reset_file_state_if_needed();
}

void void_log_set_max_files(const uint32 max_files) {
    s_max_files = max_files;
    if (s_max_files == 0 && s_log_file) {
        fclose(s_log_file);
        s_log_file = NULL;
    } else if (s_file_initialized && (s_targets & VOID_LOG_TARGET_FILE) && s_max_files > 0) {
        char resolved_dir[MAX_LOG_PATH];
        resolve_log_dir(s_config_dir, s_hidden_dir, resolved_dir, sizeof(resolved_dir));
        prune_oldest_logs(resolved_dir, s_max_files);
    }
}

void void_log_set_hidden(const bool hidden) {
    if (s_hidden_dir == hidden) return;
#ifndef _WIN32
    char old_dir[MAX_LOG_PATH];
    char new_dir[MAX_LOG_PATH];
    resolve_log_dir(s_config_dir, s_hidden_dir, old_dir, sizeof(old_dir));
    resolve_log_dir(s_config_dir, hidden, new_dir, sizeof(new_dir));
    if (s_log_file) {
        fclose(s_log_file);
        s_log_file = NULL;
        s_file_initialized = false;
    }
    rename(old_dir, new_dir);
#endif
    s_hidden_dir = hidden;
    if (s_file_initialized) {
        char resolved_dir[MAX_LOG_PATH];
        resolve_log_dir(s_config_dir, s_hidden_dir, resolved_dir, sizeof(resolved_dir));
        create_log_dir_recursive(resolved_dir, s_hidden_dir);
    }
}

void void_log_push(const VoidLogLevel level, const char *file, const int line, const char *fmt, ...) {
    if (s_targets == VOID_LOG_TARGET_NONE || level < s_min_level) return;
    uint32 current_index;
    while (true) {
        const uint32 head = void_atomic_load(&s_log_head);
        const uint32 tail = void_atomic_load(&s_log_tail);
        if (head - tail >= MAX_LOG_ENTRIES) return;
        if (void_atomic_compare(&s_log_head, head, head + 1)) {
            current_index = head;
            break;
        }
        void_processor_pause();
    }
    const uint32 ring_index = current_index % MAX_LOG_ENTRIES;
    VoidLogEntry *entry = &s_log_buffer[ring_index];
    entry->level = level;
    entry->timestamp = void_time_get_ticks();
    entry->thread_id = void_thread_get_id();
    const char *short_file = file ? file : "unknown";
    for (const char *p = short_file; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            short_file = p + 1;
        }
    }
    int prefix_len = snprintf(entry->text, MAX_LOG_LENGTH, "[%s:%d] ", short_file, line);
    if (prefix_len < 0) prefix_len = 0;
    if (prefix_len < MAX_LOG_LENGTH) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(entry->text + prefix_len, (size_t)(MAX_LOG_LENGTH - prefix_len), fmt, args);
        va_end(args);
    }
    void_atomic_store(&entry->is_ready, 1);
}

void void_log_flush(void) {
    uint32 tail = void_atomic_load(&s_log_tail);
    if (tail == void_atomic_load(&s_log_head)) return;
    const bool log_to_console = (s_targets & VOID_LOG_TARGET_CONSOLE) != 0;
    const bool log_to_file = (s_targets & VOID_LOG_TARGET_FILE) != 0 && s_max_files > 0;
    if (log_to_file && !s_file_initialized) {
        open_dated_log_file();
    }
    bool wrote_to_file = false;
    while (tail != void_atomic_load(&s_log_head)) {
        const uint32 ring_index = tail % MAX_LOG_ENTRIES;
        VoidLogEntry *entry = &s_log_buffer[ring_index];
        if (!void_atomic_load(&entry->is_ready)) break;
        if (log_to_console || (log_to_file && s_log_file)) {
            const char *tag = " VOID ";
            const char *color = NC;
            const char *reset = NC;
#ifdef _WIN32
            WORD win_attr = s_default_console_attr;
#endif
            switch (entry->level) {
                case VOID_LOG_DEBUG:
                    tag = " DEET ";
                    color = MAGENTA;
#ifdef _WIN32
                    win_attr = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
#endif
                    break;
                case VOID_LOG_INFO:
                    tag = " INFO ";
                    color = CYAN;
#ifdef _WIN32
                    win_attr = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
#endif
                    break;
                case VOID_LOG_OK:
                    tag = "  OK  ";
                    color = GREEN;
#ifdef _WIN32
                    win_attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
#endif
                    break;
                case VOID_LOG_WARN:
                    tag = " WARN ";
                    color = YELLOW;
#ifdef _WIN32
                    win_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
#endif
                    break;
                case VOID_LOG_ERROR:
                    tag = "ERROR!";
                    color = RED;
#ifdef _WIN32
                    win_attr = FOREGROUND_RED | FOREGROUND_INTENSITY;
#endif
                    break;
                case VOID_LOG_FATAL:
                    tag = "FATAL!";
                    color = SCARLET;
#ifdef _WIN32
                    win_attr = BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
#endif
                    break;
                default:
                    break;
            }
            if (!s_color_enabled) {
                color = "";
                reset = "";
            }
            if (log_to_console) {
#ifdef _WIN32
                if (s_use_win32_console_attr && s_console_handle != INVALID_HANDLE_VALUE) {
                    fflush(stdout);
                    SetConsoleTextAttribute(s_console_handle, win_attr);
                    printf("[%s] [%06" PRIu64 "] [T:%010u] %s",
                           tag, entry->timestamp, entry->thread_id, entry->text);
                    fflush(stdout);
                    SetConsoleTextAttribute(s_console_handle, s_default_console_attr);
                    printf("\n");
                } else
#endif
                {
                    printf("%s[%s] [%06" PRIu64 "] [T:%010u] %s%s\n",
                           color, tag, entry->timestamp, entry->thread_id, entry->text, reset);
                }
            }
            if (log_to_file && s_log_file) {
                const int written = fprintf(s_log_file, "[%s] [%06" PRIu64 "] [T:%010u] %s\n",
                                            tag, entry->timestamp, entry->thread_id, entry->text);
                if (written > 0) {
                    s_current_file_bytes += (uint64)written;
                    wrote_to_file = true;
                }
                if (s_current_file_bytes >= MAX_LOG_FILE_BYTES) {
                    fflush(s_log_file);
                    open_dated_log_file();
                    wrote_to_file = false;
                }
            }
        }
        void_atomic_store(&entry->is_ready, 0);
        tail++;
        void_atomic_store(&s_log_tail, tail);
    }
    if (log_to_console) {
        fflush(stdout);
    }
    if (wrote_to_file && s_log_file) {
        fflush(s_log_file);
    }
}

void void_log_exit(void) {
    void_log_flush();
    if (s_log_file) {
        fclose(s_log_file);
        s_log_file = NULL;
    }
    s_file_initialized = false;
}