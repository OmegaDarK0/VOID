#include <SDL2/SDL_image.h>
#include <stdalign.h>
#include "internal.h"
#include "void.h"

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static VoidTexture* void_texture_alloc(void) {
    VoidTexture *texture = void_arena_alloc(sizeof(VoidTexture), alignof(VoidTexture));
    if (!texture) {
        VOID_LOG_ERROR("Texture allocation failed in Arena!");
    }
    return texture;
}

static bool void_texture_query(VoidTexture *texture) {
    if (SDL_QueryTexture(texture->handle, &texture->format, &texture->access, &texture->width, &texture->height) < 0) {
        VOID_LOG_ERROR("SDL_QueryTexture failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

VoidTexture *void_texture_load(const VoidWindow *window, const char *filename) {
    if (!window || !window->render) return NULL;
    const VoidArenaMarker marker = void_arena_get_marker();
    VoidTexture *texture = void_texture_alloc();
    if (!texture) return NULL;
    texture->handle = IMG_LoadTexture(window->render, filename);
    if (!texture->handle) {
        void_arena_rollback_to(marker);
        VOID_LOG_ERROR("IMG_LoadTexture failed: %s", IMG_GetError());
        return NULL;
    }
    if (!void_texture_query(texture)) {
        SDL_DestroyTexture(texture->handle);
        void_arena_rollback_to(marker);
        return NULL;
    }
    return texture;
}

VoidTexture* void_texture_create_from_pixels(const VoidWindow *window, const int w, const int h, const void *pixels) {
    if (!window || !window->render) return NULL;
    const VoidArenaMarker marker = void_arena_get_marker();
    VoidTexture *texture = void_texture_alloc();
    if (!texture) return NULL;
    texture->handle = SDL_CreateTexture(window->render, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!texture->handle) {
        void_arena_rollback_to(marker);
        VOID_LOG_ERROR("SDL_CreateTexture (Streaming) failed: %s", SDL_GetError());
        return NULL;
    }
    void_texture_query(texture);
    if (pixels) {
        void_texture_update(texture, pixels);
    }
    return texture;
}

void void_texture_update(const VoidTexture *texture, const void *pixels) {
    if (texture && texture->handle && pixels) {
        SDL_UpdateTexture(texture->handle, NULL, pixels, texture->width * 4);
    }
}

VoidTexture* void_texture_create_target(const VoidWindow *window, const int w, const int h) {
    if (!window || !window->render) return NULL;
    const VoidArenaMarker marker = void_arena_get_marker();
    VoidTexture *texture = void_texture_alloc();
    if (!texture) return NULL;
    texture->handle = SDL_CreateTexture(window->render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!texture->handle) {
        void_arena_rollback_to(marker);
        VOID_LOG_ERROR("SDL_CreateTexture (Target) failed: %s", SDL_GetError());
        return NULL;
    }
    void_texture_query(texture);
    return texture;
}

bool void_window_set_render_target(VoidWindow *window, VoidTexture *target) {
    if (!window || !window->render) return false;
    SDL_Texture *sdl_tex = target ? target->handle : NULL;
    if (SDL_SetRenderTarget(window->render, sdl_tex) < 0) {
        VOID_LOG_ERROR("SDL_SetRenderTarget failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

void void_texture_destroy(const VoidTexture *texture) {
    if (texture && texture->handle) {
        SDL_DestroyTexture(texture->handle);
    }
}

// ============================================================================
// BLENDING
// ============================================================================

void void_texture_set_color(const VoidTexture *texture, const uint8 r, const uint8 g, const uint8 b) {
    if (texture && texture->handle) SDL_SetTextureColorMod(texture->handle, r, g, b);
}

void void_texture_set_alpha(const VoidTexture *texture, const uint8 a) {
    if (texture && texture->handle) SDL_SetTextureAlphaMod(texture->handle, a);
}

void void_texture_set_blend_mode(const VoidTexture *texture, const VoidBlendMode mode) {
    if (texture && texture->handle) {
        SDL_SetTextureBlendMode(texture->handle, (SDL_BlendMode)mode);
    }
}

// ============================================================================
// DESSIN & TRANSFORMATIONS SPATIALES
// ============================================================================

int void_texture_get_width(const VoidTexture *texture) { return texture ? texture->width : 0; }
int void_texture_get_height(const VoidTexture *texture) { return texture ? texture->height : 0; }

bool void_texture_draw(const VoidWindow *window, const VoidTexture *texture,
    const int src_x, const int src_y, const int src_w, const int src_h,
    const float dst_x, const float dst_y, const float dst_w, const float dst_h) {
    if (!window || !window->render || !texture || !texture->handle) return false;
    const SDL_Rect src = {src_x, src_y, src_w, src_h};
    const SDL_FRect dst = {dst_x, dst_y, dst_w, dst_h};
    if (SDL_RenderCopyF(window->render, texture->handle, &src, &dst) < 0) {
        VOID_LOG_ERROR("SDL_RenderCopyF failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool void_texture_draw_ex(const VoidWindow *window, const VoidTexture *texture,
                          const int src_x, const int src_y, const int src_w, const int src_h,
                          const float dst_x, const float dst_y, const float dst_w, const float dst_h,
                          const double angle, const VoidPoint *center, const VoidFlipMode flip) {
    if (!window || !window->render || !texture || !texture->handle) return false;
    const SDL_Rect src = {src_x, src_y, src_w, src_h};
    const SDL_FRect dst = {dst_x, dst_y, dst_w, dst_h};
    SDL_FPoint sdl_center;
    if (center) {
        sdl_center.x = center->x;
        sdl_center.y = center->y;
    }
    if (SDL_RenderCopyExF(window->render, texture->handle, &src, &dst, angle, center ? &sdl_center : NULL, (SDL_RendererFlip)flip) < 0) {
        VOID_LOG_ERROR("SDL_RenderCopyExF failed: %s", SDL_GetError());
        return false;
    }
    return true;
}