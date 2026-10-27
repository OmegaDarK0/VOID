#include "internal.h"
#include "void.h"

void void_render_set_logical_size(const VoidWindow *window, const int width, const int height) {
    if (!window || !window->render) return;
    SDL_RenderSetLogicalSize(window->render, width, height);
}

void void_render_set_driver(const char *driver_name) {
    if (driver_name) SDL_SetHint(SDL_HINT_RENDER_DRIVER, driver_name);
}

void void_render_set_filter(const VoidFilterMode mode) {
    switch (mode) {
        case VOID_FILTER_NEAREST: SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); break;
        case VOID_FILTER_LINEAR:  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); break;
        case VOID_FILTER_BEST:    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2"); break;
    }
}

bool void_render_clear(const VoidWindow *window, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (!window || !window->render) return false;
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) return false;
    return SDL_RenderClear(window->render) == 0;
}

void void_render_present(const VoidWindow *window) {
    if (window && window->render) {
        SDL_RenderPresent(window->render);
    }
}

bool void_render_point(const VoidWindow *window, const float x, const float y, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (!window || !window->render) return false;
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) return false;
    return SDL_RenderDrawPointF(window->render, x, y) == 0;
}

bool void_render_line(const VoidWindow *window, const float x1, const float y1, const float x2, const float y2, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (!window || !window->render) return false;
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) return false;
    return SDL_RenderDrawLineF(window->render, x1, y1, x2, y2) == 0;
}

bool void_render_rect(const VoidWindow *window, const float x, const float y, const float w, const float h, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (!window || !window->render) return false;
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) return false;
    const SDL_FRect rect = {.x = x, .y = y, .w = w, .h = h};
    return fill ? SDL_RenderFillRectF(window->render, &rect) == 0 : SDL_RenderDrawRectF(window->render, &rect) == 0;
}

bool void_render_points(const VoidWindow *window, const VoidPoint *points, const int count, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (!window || !window->render || !points || count <= 0) return false;
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) return false;
    return SDL_RenderDrawPointsF(window->render, (const SDL_FPoint*)points, count) == 0;
}

bool void_render_lines(const VoidWindow *window, const VoidPoint *points, const int count, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (!window || !window->render || !points || count <= 0) return false;
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) return false;
    return SDL_RenderDrawLinesF(window->render, (const SDL_FPoint*)points, count) == 0;
}

bool void_render_rects(const VoidWindow *window, const VoidRect *rects, const int count, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (!window || !window->render || !rects || count <= 0) return false;
    if (SDL_SetRenderDrawColor(window->render, r, g, b, a) < 0) return false;
    return fill ? SDL_RenderFillRectsF(window->render, (const SDL_FRect*)rects, count) == 0 : SDL_RenderDrawRectsF(window->render, (const SDL_FRect*)rects, count) == 0;
}