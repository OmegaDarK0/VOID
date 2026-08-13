#ifndef VOID_TYPES_H
#define VOID_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

// IEC Standards
#define KiB (1024ULL)
#define MiB (1024ULL * KiB)
#define GiB (1024ULL * MiB)

// SI Standards (Base 10)
#define KB (1000ULL)
#define MB (1000ULL * KB)
#define GB (1000ULL * MB)

#define BYTES_TO_BITS(bytes) ((bytes) * 8ULL)
#define BITS_TO_BYTES(bits)  (((bits) + 7ULL) / 8ULL)

// ============================================================================
// FUNDAMENTAL TYPES
// ============================================================================
// Guarantees data size, vital for Data-Oriented Design (SoA)

#define VOID_WINDOWPOS_UNDEFINED 0x1FFF0000
#define VOID_WINDOWPOS_CENTERED  0x2FFF0000

typedef enum {
    VOID_WINDOW_DEFAULT     = 0,
    VOID_WINDOW_FULLSCREEN  = 1 << 0,
    VOID_WINDOW_OPENGL      = 1 << 1,
    VOID_WINDOW_SHOWN       = 1 << 2,
    VOID_WINDOW_HIDDEN      = 1 << 3,
    VOID_WINDOW_BORDERLESS  = 1 << 4,
    VOID_WINDOW_RESIZABLE   = 1 << 5,
    VOID_WINDOW_MINIMIZED   = 1 << 6,
    VOID_WINDOW_MAXIMIZED   = 1 << 7,
    VOID_WINDOW_GRABBED     = 1 << 8,
} VoidWindowFlags;

typedef enum {
    VOID_RENDER_DEFAULT       = 0,
    VOID_RENDER_SOFTWARE      = 1 << 0,
    VOID_RENDER_ACCELERATED   = 1 << 1,
    VOID_RENDER_PRESENTVSYNC  = 1 << 2,
    VOID_RENDER_TARGETTEXTURE = 1 << 3
} VoidRenderFlags;

typedef enum {
    VOID_INIT_SDL = 1 << 0,
    VOID_INIT_IMG = 1 << 1,
    VOID_INIT_MIX = 1 << 2,
} VoidInitFlag;

typedef enum {
    VOID_LOG_DEBUG,
    VOID_LOG_OK,
    VOID_LOG_INFO,
    VOID_LOG_WARN,
    VOID_LOG_ERROR,
    VOID_LOG_FATAL
} VoidLogLevel;

typedef enum {
    // --- ALPHABET ---
    VOID_KEY_A, VOID_KEY_B, VOID_KEY_C, VOID_KEY_D, VOID_KEY_E, VOID_KEY_F,
    VOID_KEY_G, VOID_KEY_H, VOID_KEY_I, VOID_KEY_J, VOID_KEY_K, VOID_KEY_L,
    VOID_KEY_M, VOID_KEY_N, VOID_KEY_O, VOID_KEY_P, VOID_KEY_Q, VOID_KEY_R,
    VOID_KEY_S, VOID_KEY_T, VOID_KEY_U, VOID_KEY_V, VOID_KEY_W, VOID_KEY_X,
    VOID_KEY_Y, VOID_KEY_Z,
    // --- ---
    VOID_KEY_0, VOID_KEY_1, VOID_KEY_2, VOID_KEY_3, VOID_KEY_4,
    VOID_KEY_5, VOID_KEY_6, VOID_KEY_7, VOID_KEY_8, VOID_KEY_9,
    // ---  ---
    VOID_KEY_UP, VOID_KEY_DOWN, VOID_KEY_LEFT, VOID_KEY_RIGHT,
    // ---  ---
    VOID_KEY_SPACE, VOID_KEY_ENTER, VOID_KEY_ESCAPE, VOID_KEY_TAB, VOID_KEY_BACKSPACE,
    VOID_KEY_LSHIFT, VOID_KEY_RSHIFT, VOID_KEY_LCTRL, VOID_KEY_RCTRL, VOID_KEY_LALT, VOID_KEY_RALT,
    // ---  ---
    VOID_KEY_F1, VOID_KEY_F2, VOID_KEY_F3, VOID_KEY_F4, VOID_KEY_F5, VOID_KEY_F6,
    VOID_KEY_F7, VOID_KEY_F8, VOID_KEY_F9, VOID_KEY_F10, VOID_KEY_F11, VOID_KEY_F12
} VoidKeyCode;

typedef struct VoidArena VoidArena;
typedef uint64_t         VoidArenaMarker;

typedef void (*VoidThreadFunc)(void *data);

typedef struct VoidThread   VoidThread;
typedef struct VoidLogEntry VoidLogEntry;

typedef struct VoidWindow  VoidWindow;
typedef struct VoidRender  VoidRender;
typedef struct VoidTexture VoidTexture;

typedef struct VoidPoint VoidPoint;
typedef struct VoidRect  VoidRect;

typedef struct VoidSound VoidSound;
typedef struct VoidMusic VoidMusic;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#endif //VOID_TYPES_H
