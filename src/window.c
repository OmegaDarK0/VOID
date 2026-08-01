#include <SDL2/SDL.h>
#include <stdalign.h>
#include "void.h"

struct VoidWindow {
    SDL_Window* handle;
    SDL_Renderer* render;
    bool is_running, should_close;
};

struct VoidRender {
    SDL_Renderer* handle;
};

struct VoidPoint {
    float x, y;
};

struct VoidRect {
    float x, y, w, h;
};

VoidWindow *void_window_create(const char *title, const uint32 width, const uint32 height) {
    const VoidArenaMarker marker = void_arena_get_marker();
    VoidWindow* window = void_arena_alloc(sizeof(VoidWindow), alignof(VoidWindow));
    if (window == NULL) {
        VOID_LOG_ERROR("Window allocation failed!");
        return NULL;
    }
    window->handle = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        (int)width, (int)height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
        );
    if (window->handle == NULL) {
        void_arena_rollback_to(marker);
        VOID_LOG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
        return NULL;
    }
    window->render = SDL_CreateRenderer(window->handle, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );
    if (window->render == NULL) {
        SDL_DestroyWindow(window->handle);
        void_arena_rollback_to(marker);
        VOID_LOG_ERROR("SDL_CreateRenderer failed: %s", SDL_GetError());
        return NULL;
    }
    window->is_running = true;
    window->should_close = false;
    VOID_LOG_OK("Window created! (addr: %p)", window);
    return window;
}

void void_window_destroy(const VoidWindow *window) {
    if (window == NULL) {
        VOID_LOG_WARN("Window already destroyed!");
        return;
    }
    if (window->render) SDL_DestroyRenderer(window->render);
    if (window->handle) SDL_DestroyWindow(window->handle);
    VOID_LOG_INFO("Window destroyed.");
}

void void_window_close(VoidWindow *window) {
    window->is_running = false;
}

bool void_window_is_running(const VoidWindow *window) {
    return window->is_running;
}

bool void_window_should_close(const VoidWindow *window) {
    return window->should_close;
}

void void_window_poll_events(VoidWindow *window) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                window->should_close = true;
                break;
            default:
                break;
        }
    }
}

VoidRender *void_window_get_render(const VoidWindow *window) {
    return (VoidRender*) window->render;
}

void void_window_get_size(const VoidWindow *window, int *width, int *height) {
    if (window == NULL || window->handle == NULL) {
        if (width) *width = 0;
        if (height) *height = 0;
        return;
    }
    SDL_GetWindowSize(window->handle, width, height);
}

void void_window_set_logical_size(const VoidWindow *window, const int width, const int height) {
    if (window == NULL || window->render == NULL) return;
    SDL_RenderSetLogicalSize(window->render, width, height);
}

bool void_render_clear(const VoidWindow *window, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) {
        VOID_LOG_ERROR("SDL_SetRenderDrawColor failed: %s", SDL_GetError());
        return false;
    }
    if (SDL_RenderClear(window->render) < 0) {
        VOID_LOG_ERROR("SDL_RenderClear failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

void void_render_present(const VoidWindow *window) {
    SDL_RenderPresent(window->render);
}

bool void_render_point(const VoidWindow *window, const float x, const float y, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) {
        VOID_LOG_ERROR("SDL_SetRenderDrawColor failed: %s", SDL_GetError());
        return false;
    }
    if (SDL_RenderDrawPointF(window->render, x, y) < 0) {
        VOID_LOG_ERROR("SDL_RenderDrawPointF failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool void_render_line(const VoidWindow *window, const float x1, const float y1, const float x2, const float y2, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) {
        VOID_LOG_ERROR("SDL_SetRenderDrawColor failed: %s", SDL_GetError());
        return false;
    }
    if (SDL_RenderDrawLineF(window->render, x1, y1, x2, y2) < 0) {
        VOID_LOG_ERROR("SDL_RenderDrawLineF failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool void_render_rect(const VoidWindow *window, const float x, const float y, const float w, const float h, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) {
        VOID_LOG_ERROR("SDL_SetRenderDrawColor failed: %s", SDL_GetError());
        return false;
    }
    const SDL_FRect rect = {x, y, w, h};
    if (fill) {
        if (SDL_RenderFillRectF(window->render, &rect) < 0) {
            VOID_LOG_ERROR("SDL_RenderFillRectF failed: %s", SDL_GetError());
            return false;
        }
    } else {
        if (SDL_RenderDrawRectF(window->render, &rect) < 0) {
            VOID_LOG_ERROR("SDL_RenderDrawRectF failed: %s", SDL_GetError());
            return false;
        }
    }
    return true;
}

bool void_render_points(const VoidWindow *window, const VoidPoint *points, const int count, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) {
        VOID_LOG_ERROR("SDL_SetRenderDrawColor failed: %s", SDL_GetError());
        return false;
    }
    if (SDL_RenderDrawPointsF(window->render, (const SDL_FPoint*)points, count) < 0) {
        VOID_LOG_ERROR("SDL_RenderDrawPointsF failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool void_render_lines(const VoidWindow *window, const VoidPoint *points, const int count, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) {
        VOID_LOG_ERROR("SDL_SetRenderDrawColor failed: %s", SDL_GetError());
        return false;
    }
    if (SDL_RenderDrawLinesF(window->render, (const SDL_FPoint*)points, count) < 0) {
        VOID_LOG_ERROR("SDL_RenderDrawLinesF failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool void_render_rects(const VoidWindow *window, const VoidRect *rects, const int count, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) {
        VOID_LOG_ERROR("SDL_SetRenderDrawColor failed: %s", SDL_GetError());
        return false;
    }
    if (fill) {
        if (SDL_RenderFillRectsF(window->render, (const SDL_FRect*)rects, count) < 0) {
            VOID_LOG_ERROR("SDL_RenderFillRectsF failed: %s", SDL_GetError());
            return false;
        }
    } else {
        if (SDL_RenderDrawRectsF(window->render, (const SDL_FRect*)rects, count) < 0) {
            VOID_LOG_ERROR("SDL_RenderDrawRectsF failed: %s", SDL_GetError());
            return false;
        }
    }
    return true;
}