#ifndef VOID_H
#define VOID_H

#include "api.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// SYSTEM & TIME (core.c / time.c)
// ============================================================================

VOID_API bool   void_init(void);
VOID_API void   void_exit(void);

VOID_API uint32 void_get_core_count(void); // Useful for initializing workers (job.cpp)

VOID_API uint64 void_time_get_ticks(void);        // High-resolution time in milliseconds
VOID_API float  void_time_get_delta(void);        // Delta time calculated by the Back-end

// ============================================================================
// DYNAMIC LIBRARY LOADING (library.c)
// ============================================================================
// Allows the engine to load/unload gameplay code at runtime (Hot-Reloading)

VOID_API void* void_library_load(const char *filepath);
VOID_API void* void_library_get_symbol(void *handle, const char *symbol_name);
VOID_API void  void_library_free(void *handle);

// ============================================================================
// MEMORY (memory.c) - Hierarchical & Scoped Arenas
// ============================================================================

// Initializes the master OS virtual memory block (returns the Root Arena)
VOID_API VoidArena *void_memory_init(uint64 total_capacity);
VOID_API void       void_memory_exit(void);

// Retrieves the master Root Arena or sets/gets the active Default Arena (used when arena == NULL)
VOID_API void       void_memory_set_default_arena(VoidArena *arena);
VOID_API VoidArena *void_memory_get_default_arena(void);
VOID_API VoidArena *void_memory_get_root_arena(void);

// Carves a new named sub-arena out of a parent arena (NULL parent = Root Arena)
VOID_API VoidArena *void_arena_suballoc(VoidArena *parent, const char *name, uint64 capacity);

// Allocates or resizes memory from a specific arena (NULL arena = Default Arena)
VOID_API void      *void_arena_alloc(VoidArena *arena, uint64 size, uint32 alignment);
VOID_API void      *void_arena_realloc(VoidArena *arena, void *ptr, uint64 old_size, uint64 new_size, uint32 alignment);

// Cross-arena data promotion, zero-copy top-of-stack move, and string utilities
VOID_API void      *void_arena_memdup(VoidArena *dst_arena, const void *src_ptr, uint64 size, uint32 alignment);
VOID_API void      *void_arena_memmove(VoidArena *dst_arena, VoidArena *src_arena, void *src_ptr, uint64 size, uint32 alignment);
VOID_API char      *void_arena_strdup(VoidArena *dst_arena, const char *str);
VOID_API char      *void_arena_strndup(VoidArena *dst_arena, const char *str, uint64 max_len);
VOID_API char      *void_arena_sprintf(VoidArena *dst_arena, const char *fmt, ...);

// Resets an arena's offset to 0 in O(1)
VOID_API void       void_arena_reset(VoidArena *arena);

// Scoped Memory Management (O(1) hierarchical lifecycles)
VOID_API VoidScope  void_arena_push(const VoidArena *arena);
VOID_API void       void_arena_pop(VoidArena *arena, VoidScope scope);

// Telemetry & Real-time Monitoring
VOID_API void       void_arena_print(const VoidArena *arena);
VOID_API void       void_memory_print(void);
VOID_API void       void_memory_monitor(uint32 interval_ms);

// ============================================================================
// WINDOW & INPUT (window.c / input.c)
// ============================================================================

// --- 1. Creation & Lifecycle ---
VOID_API VoidWindow *void_window_create(const char *title, int x, int y, uint32 w, uint32 h, VoidWindowFlags w_flags, VoidRenderFlags r_flags);
VOID_API void        void_window_destroy(const VoidWindow *window);
VOID_API void        void_window_close(VoidWindow *window);
VOID_API bool        void_window_is_running(const VoidWindow *window);
VOID_API bool        void_window_should_close(const VoidWindow *window);
VOID_API void        void_window_poll_events(VoidWindow *window);

// --- 2. Window Setters ---
VOID_API void void_window_set_title(const VoidWindow *window, const char *title);
VOID_API void void_window_set_position(const VoidWindow *window, int x, int y);
VOID_API void void_window_set_size(const VoidWindow *window, int width, int height);
VOID_API void void_window_set_fullscreen(const VoidWindow *window, bool fullscreen);
VOID_API void void_window_set_bordered(const VoidWindow *window, bool bordered);
VOID_API void void_window_set_resizable(const VoidWindow *window, bool resizable);
VOID_API void void_window_set_grabbed(const VoidWindow *window, bool grabbed);
VOID_API void void_window_show(const VoidWindow *window);
VOID_API void void_window_hide(const VoidWindow *window);
VOID_API void void_window_maximize(const VoidWindow *window);
VOID_API void void_window_minimize(const VoidWindow *window);

// --- 3. Window Getters ---
VOID_API void void_window_get_position(const VoidWindow *window, int *x, int *y);
VOID_API void void_window_get_size(const VoidWindow *window, int *width, int *height);

// --- 4. Window Status (is?) ---
VOID_API bool void_window_is_fullscreen(const VoidWindow *window);
VOID_API bool void_window_is_hidden(const VoidWindow *window);
VOID_API bool void_window_is_maximized(const VoidWindow *window);
VOID_API bool void_window_is_minimized(const VoidWindow *window);
VOID_API bool void_window_is_grabbed(const VoidWindow *window);

// --- 5. Render API ---
VOID_API VoidRender *void_window_get_render(const VoidWindow *window);
VOID_API void        void_render_set_logical_size(const VoidWindow *window, int width, int height);

// --- System Render Hints ---
// Call BEFORE window creation to force a graphics API
VOID_API void void_render_set_driver(const char *driver_name);
// Call before loading/creating a texture to set its sampling quality
VOID_API void void_render_set_filter(VoidFilterMode mode);

// ============================================================================
// BASIC 2D RENDERING (Provided by the SDL_Renderer back-end)
// ============================================================================

// Clears the screen with a color (R, G, B, A)
VOID_API bool void_render_clear(const VoidWindow *window, uint8 r, uint8 g, uint8 b, uint8 a);

// Presents the final image to the screen (Swap Buffers)
VOID_API void void_render_present(const VoidWindow *window);

// Drawing primitives
VOID_API bool void_render_point(const VoidWindow *window, float x, float y, uint8 r, uint8 g, uint8 b, uint8 a);
VOID_API bool void_render_line(const VoidWindow *window, float x1, float y1, float x2, float y2, uint8 r, uint8 g, uint8 b, uint8 a);
VOID_API bool void_render_rect(const VoidWindow *window, float x, float y, float w, float h, uint8 r, uint8 g, uint8 b, uint8 a, bool fill);

VOID_API bool void_render_points(const VoidWindow *window, const VoidPoint *points, int count, uint8 r, uint8 g, uint8 b, uint8 a);
VOID_API bool void_render_lines(const VoidWindow *window, const VoidPoint *points, int count, uint8 r, uint8 g, uint8 b, uint8 a);
VOID_API bool void_render_rects(const VoidWindow *window, const VoidRect *rects, int count, uint8 r, uint8 g, uint8 b, uint8 a, bool fill);

// Texture management
VOID_API VoidTexture *void_texture_load(const VoidWindow *window, const char *filename);
VOID_API void         void_texture_destroy(const VoidTexture *texture);
VOID_API bool         void_texture_draw(const VoidWindow *window, const VoidTexture *texture, int src_x, int src_y, int src_w, int src_h, float dst_x, float dst_y, float dst_w, float dst_h);

// Texture getters (Cached internally)
VOID_API int void_texture_get_width(const VoidTexture *texture);
VOID_API int void_texture_get_height(const VoidTexture *texture);

// --- VRAM & FBO (Render Targets) ---
VOID_API VoidTexture* void_texture_create_from_pixels(const VoidWindow *window, int w, int h, const void *pixels);
VOID_API void         void_texture_update(const VoidTexture *texture, const void *pixels);
VOID_API VoidTexture* void_texture_create_target(const VoidWindow *window, int w, int h);
VOID_API bool         void_window_set_render_target(VoidWindow *window, VoidTexture *target);

// --- Blending ---
VOID_API void void_texture_set_color(const VoidTexture *texture, uint8 r, uint8 g, uint8 b);
VOID_API void void_texture_set_alpha(const VoidTexture *texture, uint8 a);
VOID_API void void_texture_set_blend_mode(const VoidTexture *texture, VoidBlendMode mode);

// --- ---
VOID_API bool void_texture_draw_ex(const VoidWindow *window, const VoidTexture *texture,
                                   int src_x, int src_y, int src_w, int src_h,
                                   float dst_x, float dst_y, float dst_w, float dst_h,
                                   double angle, const VoidPoint *center, VoidFlipMode flip);

// ============================================================================
// ADVANCED GEOMETRY (gfx.c) - Zero Allocation
// ============================================================================

VOID_API bool void_gfx_draw_triangle(const VoidWindow *window, float x1, float y1, float x2, float y2, float x3, float y3, uint8 r, uint8 g, uint8 b, uint8 a, bool fill);
VOID_API bool void_gfx_draw_quad(const VoidWindow *window, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, uint8 r, uint8 g, uint8 b, uint8 a, bool fill);
VOID_API bool void_gfx_draw_circle(const VoidWindow *window, float x, float y, float radius, uint8 r, uint8 g, uint8 b, uint8 a, bool fill);

// Concave Polygon Triangulation (Ear Clipping)
VOID_API bool void_gfx_draw_regular_polygon(const VoidWindow *window, float x, float y, float radius, int sides, float rotation, uint8 r, uint8 g, uint8 b, uint8 a, bool fill);
VOID_API bool void_gfx_draw_polygon(const VoidWindow *window, const VoidPoint *points, int count, uint8 r, uint8 g, uint8 b, uint8 a, bool fill);

// --- Advanced Polished Shapes ---
VOID_API bool void_gfx_draw_ellipse(const VoidWindow *window, float x, float y, float radius_x, float radius_y, uint8 r, uint8 g, uint8 b, uint8 a, bool fill);
VOID_API bool void_gfx_draw_thick_line(const VoidWindow *window, float x1, float y1, float x2, float y2, float thickness, uint8 r, uint8 g, uint8 b, uint8 a);
VOID_API bool void_gfx_draw_rounded_rect(const VoidWindow *window, float x, float y, float w, float h, float corner_radius, uint8 r, uint8 g, uint8 b, uint8 a, bool fill);

VOID_API bool void_gfx_draw_pie(const VoidWindow *window, float x, float y, float radius, float start_angle, float end_angle, uint8 r, uint8 g, uint8 b, uint8 a, bool fill);
VOID_API bool void_gfx_draw_bezier(const VoidWindow *window, float x1, float y1, float cx, float cy, float x2, float y2, int segments, uint8 r, uint8 g, uint8 b, uint8 a);
VOID_API bool void_gfx_draw_ring(const VoidWindow *window, float x, float y, float inner_radius, float outer_radius, uint8 r, uint8 g, uint8 b, uint8 a, bool fill);
VOID_API bool void_gfx_draw_capsule(const VoidWindow *window, float x, float y, float w, float h, uint8 r, uint8 g, uint8 b, uint8 a, bool fill);

// --- The Ultimate Feature: Textured Geometry ---
VOID_API bool void_gfx_draw_textured_quad(const VoidWindow *window, const VoidTexture *texture, const VoidPoint *pts, const VoidPoint *uvs, uint8 r, uint8 g, uint8 b, uint8 a);

// ============================================================================
// HUMAN INTERFACE DEVICES / INPUT (input.c)
// ============================================================================

// --- Keyboard ---
VOID_API bool  void_input_is_key_pressed(VoidKeyCode key);

// --- Mouse (Logical resolution aware) ---
VOID_API void  void_input_get_mouse_pos(const VoidWindow *window, float *x, float *y);
VOID_API bool  void_input_is_mouse_pressed(VoidMouseButton button);

// --- Gamepads (Up to 4 controllers, automatic hot-plugging & deadzone) ---
VOID_API bool  void_input_is_gamepad_connected(int gamepad_index);
VOID_API float void_input_get_gamepad_axis(int gamepad_index, VoidGamepadAxis axis);
VOID_API bool  void_input_is_gamepad_pressed(int gamepad_index, VoidGamepadButton button);
VOID_API void  void_input_set_gamepad_deadzone(float deadzone);

// ============================================================================
// LOW-LEVEL THREADING (thread.c)
// ============================================================================
// Provides just enough primitives for job.cpp (C++) to build its Fibers

VOID_API void   void_thread_create(VoidThreadFunc func, void *data);
VOID_API void   void_thread_sleep(uint32 ms);
VOID_API uint32 void_thread_get_id(void);

// Atomic primitives to avoid blocking mutexes
VOID_API uint32 void_atomic_increment(volatile uint32 *value);
VOID_API uint32 void_atomic_decrement(volatile uint32 *value);
VOID_API bool   void_atomic_compare_exchange(volatile uint32 *value, uint32 expected, uint32 desired);

// ============================================================================
// LOGGING SYSTEM (log.c)
// ============================================================================

VOID_API void void_log_init(void);
VOID_API void void_log_push(VoidLogLevel level, const char *file, int line, const char *fmt, ...);
VOID_API void void_log_flush(void);
VOID_API void void_log_exit(void);

// ============================================================================
// FILE I/O (file.c)
// ============================================================================

VOID_API void* void_file_read(const char *filepath, uint64 *out_size);

// ============================================================================
// AUDIO MIXER & SPATIALIZATION (audio.c)
// ============================================================================

VOID_API VoidSound* void_audio_load_sound(const char *filepath);
VOID_API int32      void_audio_play_sound(const VoidSound *sound);
VOID_API void       void_audio_stop_channel(int32 channel);
VOID_API void       void_audio_set_channel_position(int32 channel, int16 angle, uint8 distance);
VOID_API void       void_audio_destroy_sound(const VoidSound *sound);

VOID_API VoidMusic* void_audio_load_music(const char *filepath);
VOID_API void       void_audio_play_music(const VoidMusic *music, bool loop);
VOID_API void       void_audio_stop_music(void);
VOID_API void       void_audio_destroy_music(const VoidMusic *music);

VOID_API void       void_audio_set_master_volume(float volume);

// Helper macros to automatically inject file and line numbers
#define VOID_LOG_DEBUG(...) void_log_push(VOID_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define VOID_LOG_INFO(...)  void_log_push(VOID_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define VOID_LOG_OK(...)    void_log_push(VOID_LOG_OK, __FILE__, __LINE__, __VA_ARGS__)
#define VOID_LOG_WARN(...)  void_log_push(VOID_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define VOID_LOG_ERROR(...) void_log_push(VOID_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define VOID_LOG_FATAL(...) void_log_push(VOID_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#ifdef NDEBUG
    #define VOID_ASSERT(condition) ((void)0)
#else
    #define VOID_ASSERT(condition) \
        do { \
            if (!(condition)) { \
                VOID_LOG_FATAL("Assertion failed: %s", #condition); \
                abort(); \
            } \
        } while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif // VOID_H