#include <SDL2/SDL_image.h>
#include <stdalign.h>
#include "internal.h"
#include "void.h"

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static VoidTexture* void_texture_alloc(void) {
    VoidTexture *texture = void_arena_alloc(NULL, sizeof(VoidTexture), alignof(VoidTexture));
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
    if (!window || !window->render || !filename) return NULL;
    const VoidScope scope = void_arena_push(NULL);
    VoidTexture *texture = void_texture_alloc();
    if (!texture) return NULL;
    texture->handle = IMG_LoadTexture(window->render, filename);
    if (!texture->handle) {
        void_arena_pop(NULL, scope);
        VOID_LOG_ERROR("IMG_LoadTexture failed: %s", IMG_GetError());
        return NULL;
    }
    if (!void_texture_query(texture)) {
        SDL_DestroyTexture(texture->handle);
        void_arena_pop(NULL, scope);
        return NULL;
    }
    return texture;
}

VoidTexture* void_texture_create_from_pixels(const VoidWindow *window, const int w, const int h, const void *pixels) {
    if (!window || !window->render) return NULL;
    const VoidScope scope = void_arena_push(NULL);
    VoidTexture *texture = void_texture_alloc();
    if (!texture) return NULL;
    texture->handle = SDL_CreateTexture(window->render, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!texture->handle) {
        void_arena_pop(NULL, scope);
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
    const VoidScope scope = void_arena_push(NULL);
    VoidTexture *texture = void_texture_alloc();
    if (!texture) return NULL;
    texture->handle = SDL_CreateTexture(window->render, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!texture->handle) {
        void_arena_pop(NULL, scope);
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

void void_texture_destroy(VoidTexture *texture) {
    if (texture && texture->handle) {
        SDL_DestroyTexture(texture->handle);
        texture->handle = NULL;
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
    if (!texture || !texture->handle) return;
    SDL_BlendMode sdl_mode = SDL_BLENDMODE_NONE;
    switch (mode) {
        case VOID_BLEND_NONE:  sdl_mode = SDL_BLENDMODE_NONE;  break;
        case VOID_BLEND_BLEND: sdl_mode = SDL_BLENDMODE_BLEND; break;
        case VOID_BLEND_ADD:   sdl_mode = SDL_BLENDMODE_ADD;   break;
        case VOID_BLEND_MOD:   sdl_mode = SDL_BLENDMODE_MOD;   break;
        case VOID_BLEND_MUL:   sdl_mode = SDL_BLENDMODE_MUL;   break;
        case VOID_BLEND_PREMULTIPLIED:
            sdl_mode = SDL_ComposeCustomBlendMode(
                SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
                SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
            break;
        case VOID_BLEND_ADD_PREMULTIPLIED:
            sdl_mode = SDL_ComposeCustomBlendMode(
                SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
                SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
            break;
        default:
            break;
    }
    SDL_SetTextureBlendMode(texture->handle, sdl_mode);
}

int void_texture_get_width(const VoidTexture *texture) { return texture ? texture->width : 0; }
int void_texture_get_height(const VoidTexture *texture) { return texture ? texture->height : 0; }

bool void_texture_draw(const VoidWindow *window, const VoidTexture *texture,
    const int src_x, const int src_y, const int src_w, const int src_h,
    const float dst_x, const float dst_y, const float dst_w, const float dst_h) {
    if (!window || !window->render || !texture || !texture->handle) return false;
    const SDL_Rect src = {.x = src_x, .y = src_y, .w = src_w, .h = src_h};
    const SDL_Rect *src_ptr = (src_w > 0 && src_h > 0) ? &src : NULL;
    const SDL_FRect dst = {.x = dst_x, .y = dst_y, .w = dst_w, .h = dst_h};
    if (SDL_RenderCopyF(window->render, texture->handle, src_ptr, &dst) < 0) {
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
    const SDL_Rect src = {.x = src_x, .y = src_y, .w = src_w, .h = src_h};
    const SDL_Rect *src_ptr = (src_w > 0 && src_h > 0) ? &src : NULL;
    const SDL_FRect dst = {.x = dst_x, .y = dst_y, .w = dst_w, .h = dst_h};
    SDL_FPoint sdl_center;
    if (center) {
        sdl_center.x = center->x;
        sdl_center.y = center->y;
    }
    if (SDL_RenderCopyExF(window->render, texture->handle, src_ptr, &dst, angle, center ? &sdl_center : NULL, (SDL_RendererFlip)flip) < 0) {
        VOID_LOG_ERROR("SDL_RenderCopyExF failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

VoidTexture *void_texture_load_from_memory(const VoidWindow *window, const void *data, const uint64 size) {
    if (!window || !window->render || !data || size == 0) return NULL;
    SDL_RWops *rw = SDL_RWFromConstMem(data, (int)size);
    if (!rw) {
        VOID_LOG_ERROR("SDL_RWFromConstMem failed: %s", SDL_GetError());
        return NULL;
    }
    const VoidScope scope = void_arena_push(NULL);
    VoidTexture *texture = void_texture_alloc();
    if (!texture) {
        SDL_RWclose(rw);
        return NULL;
    }
    texture->handle = IMG_LoadTexture_RW(window->render, rw, 1); // 1 = ferme rw automatiquement
    if (!texture->handle) {
        void_arena_pop(NULL, scope);
        VOID_LOG_ERROR("IMG_LoadTexture_RW failed: %s", IMG_GetError());
        return NULL;
    }
    if (!void_texture_query(texture)) {
        SDL_DestroyTexture(texture->handle);
        void_arena_pop(NULL, scope);
        return NULL;
    }
    return texture;
}

void void_texture_update_rect(const VoidTexture *texture, const int x, const int y, const int w, const int h, const void *pixels) {
    if (!texture || !texture->handle || !pixels || w <= 0 || h <= 0) return;
    const SDL_Rect rect = {.x = x, .y = y, .w = w, .h = h};
    SDL_UpdateTexture(texture->handle, &rect, pixels, w * 4);
}

void void_texture_set_filter(const VoidTexture *texture, const VoidFilterMode mode) {
    if (!texture || !texture->handle) return;
    SDL_ScaleMode sdl_mode = SDL_ScaleModeNearest;
    switch (mode) {
        case VOID_FILTER_NEAREST: sdl_mode = SDL_ScaleModeNearest; break;
        case VOID_FILTER_LINEAR:  sdl_mode = SDL_ScaleModeLinear;  break;
        case VOID_FILTER_BEST:    sdl_mode = SDL_ScaleModeBest;    break;
    }
    SDL_SetTextureScaleMode(texture->handle, sdl_mode);
}