#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "void.h"

#ifdef _WIN32
    #include <windows.h>
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
#endif

#define MAX_LOG_ENTRIES 1024
#define MAX_LOG_LENGTH 256

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
static uint32 s_log_tail = 0;

static FILE *s_log_file = NULL;
static bool s_color_enabled = true;

void void_log_init(void) {
#ifdef _WIN32
    s_color_enabled = false;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (SetConsoleMode(hOut, dwMode)) {
                s_color_enabled = true;
            }
        }
    }
#endif
    if (s_log_file == NULL) {
        s_log_file = fopen("void.log", "w");
    }
}

void void_log_push(const VoidLogLevel level, const char *file, const int line, const char *fmt, ...) {
    if (s_log_head - s_log_tail >= MAX_LOG_ENTRIES) return;
    const uint32 current_index = void_atomic_increment(&s_log_head) - 1;
    const uint32 ring_index = current_index % MAX_LOG_ENTRIES;
    VoidLogEntry *entry = &s_log_buffer[ring_index];
    entry->is_ready = 0;
    entry->level = level;
    entry->timestamp = void_time_get_ticks();
    entry->thread_id = void_thread_get_id();
    va_list args;
    va_start(args, fmt);
    snprintf(entry->text, MAX_LOG_LENGTH, "[%s:%d] ", file, line);
    const size_t prefix_len = strlen(entry->text);
    vsnprintf(entry->text + prefix_len, MAX_LOG_LENGTH - prefix_len, fmt, args);
    va_end(args);
    entry->is_ready = 1;
}

void void_log_flush(void) {
    while (s_log_tail < s_log_head) {
        const uint32 ring_index = s_log_tail % MAX_LOG_ENTRIES;
        VoidLogEntry *entry = &s_log_buffer[ring_index];
        if (!entry->is_ready) break;
        char *tag = " VOID ", *color = NC, *reset = NC;
        switch (entry->level) {
            case VOID_LOG_DEBUG:
                tag = " DEET ";
                color = MAGENTA;
                break;
            case VOID_LOG_INFO:
                tag = " INFO ";
                color = CYAN;
                break;
            case VOID_LOG_OK:
                tag = "  OK  ";
                color = GREEN;
                break;
            case VOID_LOG_WARN:
                tag = " WARN ";
                color = YELLOW;
                break;
            case VOID_LOG_ERROR:
                tag = "ERROR!";
                color = RED;
                break;
            case VOID_LOG_FATAL:
                tag = "FATAL!";
                color = SCARLET;
                break;
            default:
                break;
        }
        if (s_color_enabled == false) {
            color = "";
            reset = "";
        }
        printf("%s[%s] [%06" PRIu64 "] [T:%010u] %s%s\n", color, tag, entry->timestamp, entry->thread_id, entry->text, reset);
        if (s_log_file) {
            fprintf(s_log_file, "[%s] [%06" PRIu64 "] [T:%010u] %s\n", tag, entry->timestamp, entry->thread_id, entry->text);
            fflush(s_log_file);
        }
        s_log_tail++;
    }
}

void void_log_exit(void) {
    void_log_flush();
    if (s_log_file) {
        fclose(s_log_file);
        s_log_file = NULL;
    }
}
