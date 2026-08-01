#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdalign.h>
#include "void.h"

struct VoidTexture {
    SDL_Texture *handle;
    uint32 format;
    int access, width, height;
};

VoidTexture *void_texture_load(const VoidWindow *window, const char *filename) {
    SDL_Renderer *renderer = (SDL_Renderer*)void_window_get_render(window);
    const VoidArenaMarker marker = void_arena_get_marker();
    VoidTexture *texture = void_arena_alloc(sizeof(VoidTexture), alignof(VoidTexture));
    if (texture == NULL) {
        VOID_LOG_ERROR("Texture allocation failed!");
        return NULL;
    }
    texture->handle = IMG_LoadTexture(renderer, filename);
    if (texture->handle == NULL) {
        void_arena_rollback_to(marker);
        VOID_LOG_ERROR("IMG_LoadTexture failed: %s", IMG_GetError());
        return NULL;
    }
    if (SDL_QueryTexture(texture->handle, &texture->format, &texture->access, &texture->width, &texture->height) < 0) {
        SDL_DestroyTexture(texture->handle);
        void_arena_rollback_to(marker);
        VOID_LOG_ERROR("SDL_QueryTexture failed: %s", SDL_GetError());
        return NULL;
    }
    return texture;
}

int void_texture_get_width(const VoidTexture *texture) {
    return texture ? texture->width : 0;
}

int void_texture_get_height(const VoidTexture *texture) {
    return texture ? texture->height : 0;
}

void void_texture_destroy(const VoidTexture *texture) {
    if (texture && texture->handle) {
        SDL_DestroyTexture(texture->handle);
    }
}

bool void_texture_draw(const VoidWindow *window, const VoidTexture *texture,
    const int src_x, const int src_y, const int src_w, const int src_h,
    const float dst_x, const float dst_y, const float dst_w, const float dst_h) {
    const SDL_Rect src = {src_x, src_y, src_w, src_h};
    const SDL_FRect dst = {dst_x, dst_y, dst_w, dst_h};
    SDL_Renderer *renderer = (SDL_Renderer*)void_window_get_render(window);
    if (SDL_RenderCopyF(renderer, texture->handle, &src, &dst) < 0) {
        VOID_LOG_ERROR("SDL_RenderCopyF failed: %s", SDL_GetError());
        return false;
    }
    return true;
}
