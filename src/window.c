#include <stdalign.h>
#include "internal.h"
#include "void.h"

// ============================================================================
// 1. CREATION & LIFECYCLE
// ============================================================================
VoidWindow *void_window_create(const char *title, const int x, const int y, const uint32 w, const uint32 h, const VoidWindowFlags w_flags, const VoidRenderFlags r_flags) {
    const VoidArenaMarker marker = void_arena_get_marker();
    VoidWindow* window = void_arena_alloc(sizeof(VoidWindow), alignof(VoidWindow));
    if (window == NULL) {
        VOID_LOG_ERROR("Window allocation failed!");
        return NULL;
    }
    uint32 sdl_w_flags = VOID_WINDOW_DEFAULT | SDL_WINDOW_ALLOW_HIGHDPI;
    if (w_flags & VOID_WINDOW_FULLSCREEN) sdl_w_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (w_flags & VOID_WINDOW_OPENGL)     sdl_w_flags |= SDL_WINDOW_OPENGL;
    if (w_flags & VOID_WINDOW_SHOWN)      sdl_w_flags |= SDL_WINDOW_SHOWN;
    if (w_flags & VOID_WINDOW_HIDDEN)     sdl_w_flags |= SDL_WINDOW_HIDDEN;
    if (w_flags & VOID_WINDOW_BORDERLESS) sdl_w_flags |= SDL_WINDOW_BORDERLESS;
    if (w_flags & VOID_WINDOW_RESIZABLE)  sdl_w_flags |= SDL_WINDOW_RESIZABLE;
    if (w_flags & VOID_WINDOW_MINIMIZED)  sdl_w_flags |= SDL_WINDOW_MINIMIZED;
    if (w_flags & VOID_WINDOW_MAXIMIZED)  sdl_w_flags |= SDL_WINDOW_MAXIMIZED;
    if (w_flags & VOID_WINDOW_GRABBED)    sdl_w_flags |= SDL_WINDOW_MOUSE_GRABBED;
    window->handle = SDL_CreateWindow(title, x, y, (int)w, (int)h, sdl_w_flags);
    if (window->handle == NULL) {
        void_arena_rollback_to(marker);
        VOID_LOG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
        return NULL;
    }
    uint32 sdl_r_flags = VOID_RENDER_DEFAULT;
    if (r_flags & VOID_RENDER_SOFTWARE)      sdl_r_flags |= SDL_RENDERER_SOFTWARE;
    if (r_flags & VOID_RENDER_ACCELERATED)   sdl_r_flags |= SDL_RENDERER_ACCELERATED;
    if (r_flags & VOID_RENDER_PRESENTVSYNC)  sdl_r_flags |= SDL_RENDERER_PRESENTVSYNC;
    if (r_flags & VOID_RENDER_TARGETTEXTURE) sdl_r_flags |= SDL_RENDERER_TARGETTEXTURE;

    window->render = SDL_CreateRenderer(window->handle, -1, sdl_r_flags);
    if (window->render == NULL) {
        SDL_DestroyWindow(window->handle);
        void_arena_rollback_to(marker);
        VOID_LOG_ERROR("SDL_CreateRenderer failed: %s", SDL_GetError());
        return NULL;
    }
    window->is_running = true;
    window->should_close = false;
    VOID_LOG_OK("Window and Renderer created! (addr: %p)", window);
    return window;
}

void void_window_destroy(const VoidWindow *window) {
    if (window == NULL) return;
    if (window->render) SDL_DestroyRenderer(window->render);
    if (window->handle) SDL_DestroyWindow(window->handle);
    VOID_LOG_INFO("Window destroyed.");
}

void void_window_close(VoidWindow *window) { window->is_running = false; }
bool void_window_is_running(const VoidWindow *window) { return window->is_running; }
bool void_window_should_close(const VoidWindow *window) { return window->should_close; }

void void_window_poll_events(VoidWindow *window) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            window->should_close = true;
        }
    }
}

// ============================================================================
// 2. WINDOW SETTERS
// ============================================================================
void void_window_set_title(const VoidWindow *window, const char *title) {
    if (window && window->handle && title) SDL_SetWindowTitle(window->handle, title);
}

void void_window_set_position(const VoidWindow *window, const int x, const int y) {
    if (window && window->handle) SDL_SetWindowPosition(window->handle, x, y);
}

void void_window_set_size(const VoidWindow *window, const int width, const int height) {
    if (window && window->handle) SDL_SetWindowSize(window->handle, width, height);
}

void void_window_set_fullscreen(const VoidWindow *window, const bool fullscreen) {
    if (window && window->handle) SDL_SetWindowFullscreen(window->handle, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void void_window_set_bordered(const VoidWindow *window, const bool bordered) {
    if (window && window->handle) SDL_SetWindowBordered(window->handle, bordered ? SDL_TRUE : SDL_FALSE);
}

void void_window_set_resizable(const VoidWindow *window, const bool resizable) {
    if (window && window->handle) SDL_SetWindowResizable(window->handle, resizable ? SDL_TRUE : SDL_FALSE);
}

void void_window_set_grabbed(const VoidWindow *window, const bool grabbed) {
    if (window && window->handle) SDL_SetWindowMouseGrab(window->handle, grabbed ? SDL_TRUE : SDL_FALSE);
}

void void_window_show(const VoidWindow *window) {
    if (window && window->handle) SDL_ShowWindow(window->handle);
}

void void_window_hide(const VoidWindow *window) {
    if (window && window->handle) SDL_HideWindow(window->handle);
}

void void_window_maximize(const VoidWindow *window) {
    if (window && window->handle) SDL_MaximizeWindow(window->handle);
}

void void_window_minimize(const VoidWindow *window) {
    if (window && window->handle) SDL_MinimizeWindow(window->handle);
}

// ============================================================================
// 3. WINDOW GETTERS
// ============================================================================
void void_window_get_position(const VoidWindow *window, int *x, int *y) {
    if (window && window->handle) SDL_GetWindowPosition(window->handle, x, y);
}

void void_window_get_size(const VoidWindow *window, int *width, int *height) {
    if (window && window->handle) SDL_GetWindowSize(window->handle, width, height);
}

// ============================================================================
// 4. WINDOW STATUS (IS?)
// ============================================================================
bool void_window_is_fullscreen(const VoidWindow *window) {
    if (!window || !window->handle) return false;
    return (SDL_GetWindowFlags(window->handle) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
}

bool void_window_is_hidden(const VoidWindow *window) {
    if (!window || !window->handle) return true;
    return (SDL_GetWindowFlags(window->handle) & SDL_WINDOW_HIDDEN) != 0;
}

bool void_window_is_maximized(const VoidWindow *window) {
    if (!window || !window->handle) return false;
    return (SDL_GetWindowFlags(window->handle) & SDL_WINDOW_MAXIMIZED) != 0;
}

bool void_window_is_minimized(const VoidWindow *window) {
    if (!window || !window->handle) return false;
    return (SDL_GetWindowFlags(window->handle) & SDL_WINDOW_MINIMIZED) != 0;
}

bool void_window_is_grabbed(const VoidWindow *window) {
    if (!window || !window->handle) return false;
    return (SDL_GetWindowFlags(window->handle) & SDL_WINDOW_MOUSE_GRABBED) != 0;
}

// ============================================================================
// 5. RENDER API
// ============================================================================
VoidRender *void_window_get_render(const VoidWindow *window) {
    return (VoidRender*) window->render;
}
