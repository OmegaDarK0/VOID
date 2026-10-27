#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "void.h"

#ifndef M_PIf
    #define M_PIf 3.14159265358979323846f
#endif

// --- CONSTANTS ---
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720

static VoidArena *s_level_arena = NULL;
static VoidArena *s_frame_arena = NULL;

// ============================================================================
// TEST 1: SYSTEM, ENTROPY & LOGGING
// ============================================================================
static void test_system_and_logging(void) {
    VOID_LOG_INFO("=== [TEST 1: SYSTEM, ENTROPY & LOGGING] ===");
    const uint32 cores = void_get_core_count();
    const uint64 seed1 = void_get_random_seed();
    const uint64 seed2 = void_get_random_seed();
    VOID_LOG_INFO("[SYSTEM] Logical CPU cores detected: %u", cores);
    VOID_LOG_OK("[ENTROPY] Hardware Seed #1: 0x%016" PRIx64, seed1);
    VOID_LOG_OK("[ENTROPY] Hardware Seed #2: 0x%016" PRIx64, seed2);
    VOID_ASSERT(seed1 != seed2);
    VOID_LOG_OK("[LOG] Standard OK validation message.");
    VOID_LOG_WARN("[LOG] Standard Warning validation message.");
    VOID_LOG_DEBUG("[LOG] Magenta DEET telemetry validation message.");
    void_log_flush();
}

// ============================================================================
// TEST 2: HIERARCHICAL & SCOPED MEMORY ARENAS
// ============================================================================
static void test_memory_arenas(void) {
    VOID_LOG_INFO("=== [TEST 2: HIERARCHICAL & SCOPED MEMORY ARENAS] ===");
    const VoidScope level_scope = void_arena_push(s_level_arena);
    const VoidScope frame_scope = void_arena_push(s_frame_arena);
    // 1. Dynamic formatting in FRAME arena
    char *temp_msg = void_arena_sprintf(s_frame_arena, "Entity #%d: Boss_Dragon (HP: %d)", 42, 9999);
    VOID_LOG_DEBUG("[MEMORY] Formatted in FRAME: '%s'", temp_msg);
    // 2. In-place top-of-stack reallocation
    const uint64 old_len = (uint64)strlen(temp_msg) + 1;
    temp_msg = void_arena_realloc(s_frame_arena, temp_msg, old_len, 128, 1);
    strcat(temp_msg, " [ENRAGED]");
    VOID_LOG_DEBUG("[MEMORY] Reallocated in-place in FRAME: '%s'", temp_msg);
    // 3. Cross-arena Cut & Paste promotion (FRAME -> LEVEL)
    char *promoted_msg = void_arena_memmove(s_level_arena, s_frame_arena, temp_msg, 128, 1);
    char *tag_copy     = void_arena_strdup(s_level_arena, "BOSS_ACTIVE");
    VOID_LOG_OK("[MEMORY] Promoted to LEVEL: '%s' [Tag: %s] (FRAME offset reclaimed!)", promoted_msg, tag_copy);
    void_memory_print();
    // 4. Simulate 3 ECS frames with 10,000 allocations each
    VOID_LOG_INFO("[MEMORY] Simulating 3 ECS frames (10,000 allocs/frame)...");
    for (int frame = 0; frame < 3; frame++) {
        for (int i = 0; i < 10000; i++) {
            void_arena_alloc(s_frame_arena, 64, 8);
        }
        void_arena_reset(s_frame_arena);
    }
    void_arena_pop(s_frame_arena, frame_scope);
    void_arena_pop(s_level_arena, level_scope);
    void_memory_print();
    VOID_LOG_OK("[MEMORY] All scopes popped and restored to 0 B.");
    void_log_flush();
}

// ============================================================================
// TEST 3: NATIVE OS FILE I/O & HOT-RELOAD PIPELINE
// ============================================================================
static void test_file_and_library(void) {
    VOID_LOG_INFO("=== [TEST 3: NATIVE FILE I/O & HOT-RELOAD PIPELINE] ===");
    const VoidScope scope = void_arena_push(s_frame_arena);
    void_memory_set_default_arena(s_frame_arena);
    const char *src_file = "void_test_artifact.tmp";
    const char *dst_file = "void_test_artifact_copy.tmp";
    const char *payload  = "VOID_PAL_HOT_RELOAD_VALIDATION_PAYLOAD_v0.1.0-beta";
    const uint64 payload_len = (uint64)strlen(payload);
    // 1. Write & Existence check
    if (void_file_write(src_file, payload, payload_len) && void_file_exists(src_file)) {
        const uint64 mtime = void_file_get_mtime(src_file);
        VOID_LOG_OK("[FILE] Written '%s' (%" PRIu64 " B, mtime: %" PRIu64 ").", src_file, payload_len, mtime);
    }
    // 2. Native copy & Read back
    if (void_file_copy(src_file, dst_file) && void_file_exists(dst_file)) {
        uint64 read_size = 0;
        const char *read_data = void_file_read(dst_file, &read_size);
        if (read_data && read_size == payload_len && strcmp(read_data, payload) == 0) {
            VOID_LOG_OK("[FILE] Copy & Read verified: '%s'", read_data);
        }
    }
    // Cleanup temporary test files
    remove(src_file);
    remove(dst_file);
    // 3. Read README.md snippet
    uint64 readme_size = 0;
    const char *readme_data = void_file_read("README.md", &readme_size);
    if (readme_data) {
        char *snippet = void_arena_strndup(s_frame_arena, readme_data, 28);
        VOID_LOG_OK("[FILE] README.md loaded (%" PRIu64 " B) -> '%s...'", readme_size, snippet);
    }
    // 4. Dynamic Library loader resilience check
    void *dll = void_library_load("non_existent_gameplay_module.dll");
    if (!dll) {
        VOID_LOG_OK("[LIBRARY] Gracefully handled missing dynamic library.");
    } else {
        void_library_free(dll);
    }
    void_memory_set_default_arena(NULL);
    void_arena_pop(s_frame_arena, scope);
    void_log_flush();
}

// ============================================================================
// TEST 4: MULTITHREADING, ATOMICS & SPINLOCK BACKOFF
// ============================================================================
static volatile uint32 s_shared_counter = 0;
static volatile uint32 s_test_spinlock  = 0;
static uint32 s_protected_sum  = 0;

static void thread_stress_task(void *data) {
    const uint32 iterations = (uint32)(uintptr_t)data;
    for (uint32 i = 0; i < iterations; i++) {
        void_atomic_increment(&s_shared_counter);
        if ((i & 1023u) == 0) {
            while (!void_atomic_compare(&s_test_spinlock, 0, 1)) {
                void_processor_pause();
            }
            s_protected_sum++;
            s_test_spinlock = 0;
        }
    }
}

static void test_threading(void) {
    VOID_LOG_INFO("=== [TEST 4: LOCK-FREE ATOMICS & MULTITHREADING] ===");
    const uint32 cores = void_get_core_count();
    const uint32 increments_per_thread = 1000000;
    const uint32 target = cores * increments_per_thread;
    s_shared_counter = 0;
    s_protected_sum  = 0;
    VOID_LOG_INFO("[THREAD] Spawning %u worker threads (Target: %u atomic ops)...", cores, target);
    for (uint32 i = 0; i < cores; i++) {
        void_thread_create(thread_stress_task, (void*)(uintptr_t)increments_per_thread);
    }
    while (s_shared_counter < target) {
        void_thread_sleep(5);
    }
    VOID_LOG_OK("[THREAD] Completed! Atomic counter: %u | Spinlock-guarded sum: %u", s_shared_counter, s_protected_sum);
    void_log_flush();
}

// ============================================================================
// TEST 5: INTERACTIVE SANDBOX (RENDER, GFX, INPUT, FBO & SPATIAL AUDIO)
// ============================================================================
static void play_spatialized_sfx(const VoidSound *sfx, const float src_x, const float src_y, const float listener_x, const float listener_y, const float max_dist) {
    if (!sfx) return;
    const float dx = src_x - listener_x;
    const float dy = src_y - listener_y;
    const float dist = sqrtf(dx * dx + dy * dy);
    float norm_dist = dist / max_dist * 255.0f;
    if (norm_dist > 255.0f) norm_dist = 255.0f;
    if (norm_dist < 0.0f)   norm_dist = 0.0f;
    const float rad = atan2f(dx, -dy);
    int16 final_angle = (int16)(rad * (180.0f / M_PIf));
    if (final_angle < 0) final_angle += 360;
    const int32 channel = void_audio_play_sound(sfx);
    if (channel >= 0) {
        void_audio_set_channel_volume(channel, 0.85f);
        void_audio_set_channel_position(channel, final_angle, (uint8)norm_dist);
    }
}

static void test_interactive_sandbox(void) {
    VOID_LOG_INFO("=== [TEST 5: INTERACTIVE SANDBOX] ===");
    VOID_LOG_INFO("Controls: WASD/LeftStick = Move | Mouse/RightStick = Aim | Scroll = Reticle Size | Space/SouthBtn = Pulse & Rumble | F11 = Fullscreen | ESC = Exit");
    void_render_set_driver("opengl");
    void_render_set_filter(VOID_FILTER_LINEAR);
    VoidWindow *window = void_window_create(
        "VOID Framework (v0.1.0-beta) - Interactive Sandbox",
        VOID_WINDOWPOS_CENTERED, VOID_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        VOID_WINDOW_SHOWN | VOID_WINDOW_RESIZABLE,
        VOID_RENDER_ACCELERATED | VOID_RENDER_PRESENTVSYNC | VOID_RENDER_TARGETTEXTURE
    );
    if (!window) {
        VOID_LOG_FATAL("Failed to create window.");
        return;
    }
    void_render_set_logical_size(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    // Load persistent VRAM & Audio assets in ROOT_MASTER before switching to FRAME arena
    const VoidTexture *sparkle = void_texture_load(window, "assets/gemini_sparkle.png");
    VoidTexture *fbo = void_texture_create_target(window, 320, 320);
    if (sparkle) {
        void_texture_set_blend_mode(sparkle, VOID_BLEND_ADD);
    }
    const VoidSound *bounce_sfx = void_audio_load_sound("assets/bounce.wav");
    // Switch default arena to FRAME for the real-time loop
    void_memory_set_default_arena(s_frame_arena);
    void_input_set_gamepad_deadzone(0.15f);
    float player_x = WINDOW_WIDTH * 0.5f - 32.0f;
    float player_y = WINDOW_HEIGHT * 0.5f - 32.0f;
    float box_x = 140.0f, box_y = 140.0f;
    float box_vel_x = 420.0f, box_vel_y = 360.0f;
    float reticle_radius = 14.0f;
    bool  alt_palette = false;
    double angle = 0.0;
    while (void_window_is_running(window)) {
        const float box_size = 36.0f;
        const float player_size = 64.0f;
        float player_speed = 550.0f;
        const float dt = void_time_get_delta();
        angle += 90.0 * dt;
        // 1. OS EVENTS & EDGE-TRIGGERED SHORTCUTS
        void_window_poll_events(window);
        if (void_window_should_close(window) || void_input_is_key_just_pressed(VOID_KEY_ESCAPE)) {
            void_window_close(window);
        }
        if (void_input_is_key_just_pressed(VOID_KEY_F11)) {
            const bool is_fs = void_window_is_fullscreen(window);
            void_window_set_fullscreen(window, !is_fs);
            VOID_LOG_INFO("[WINDOW] Fullscreen toggled: %s", !is_fs ? "ON" : "OFF");
        }
        if (void_input_is_key_just_pressed(VOID_KEY_SPACE) || void_input_is_gamepad_just_pressed(0, VOID_GAMEPAD_BUTTON_SOUTH)) {
            alt_palette = !alt_palette;
            void_input_rumble_gamepad(0, 0.5f, 0.9f, 120);
        }
        // 2. GAMEPAD & KEYBOARD MOVEMENT
        if (void_input_is_gamepad_connected(0)) {
            if (void_input_is_gamepad_pressed(0, VOID_GAMEPAD_BUTTON_SOUTH)) {
                player_speed *= 1.5f;
            }
            player_x += void_input_get_gamepad_axis(0, VOID_GAMEPAD_AXIS_LEFTX) * player_speed * dt;
            player_y += void_input_get_gamepad_axis(0, VOID_GAMEPAD_AXIS_LEFTY) * player_speed * dt;
        }
        if (void_input_is_key_pressed(VOID_KEY_W)) player_y -= player_speed * dt;
        if (void_input_is_key_pressed(VOID_KEY_S)) player_y += player_speed * dt;
        if (void_input_is_key_pressed(VOID_KEY_A)) player_x -= player_speed * dt;
        if (void_input_is_key_pressed(VOID_KEY_D)) player_x += player_speed * dt;
        if (player_x < 0.0f) player_x = 0.0f;
        if (player_x + player_size > WINDOW_WIDTH) player_x = WINDOW_WIDTH - player_size;
        if (player_y < 0.0f) player_y = 0.0f;
        if (player_y + player_size > WINDOW_HEIGHT) player_y = WINDOW_HEIGHT - player_size;
        // 3. MOUSE POSITION, BUTTONS & WHEEL
        float mouse_x = 0.0f, mouse_y = 0.0f, wheel_x = 0.0f, wheel_y = 0.0f;
        void_input_get_mouse_pos(window, &mouse_x, &mouse_y);
        void_input_get_mouse_wheel(&wheel_x, &wheel_y);
        if (wheel_y != 0.0f) {
            reticle_radius += wheel_y * 3.0f;
            if (reticle_radius < 6.0f)  reticle_radius = 6.0f;
            if (reticle_radius > 64.0f) reticle_radius = 64.0f;
        }
        const bool mouse_left_held  = void_input_is_mouse_pressed(VOID_MOUSE_LEFT);
        const bool mouse_right_held = void_input_is_mouse_pressed(VOID_MOUSE_RIGHT);
        // 4. BOUNCING BOX PHYSICS & SPATIAL AUDIO
        box_x += box_vel_x * dt;
        box_y += box_vel_y * dt;
        bool collided = false;
        if (box_x <= 0.0f || box_x + box_size >= WINDOW_WIDTH) {
            box_vel_x *= -1.0f;
            collided = true;
        }
        if (box_y <= 0.0f || box_y + box_size >= WINDOW_HEIGHT) {
            box_vel_y *= -1.0f;
            collided = true;
        }
        if (collided) {
            play_spatialized_sfx(
                bounce_sfx,
                box_x + box_size * 0.5f, box_y + box_size * 0.5f,
                player_x + player_size * 0.5f, player_y + player_size * 0.5f,
                1000.0f
            );
            void_input_rumble_gamepad(0, 0.25f, 0.4f, 60);
        }
        // 5. RENDERING PASS 1: OFF-SCREEN FBO
        if (fbo) {
            void_window_set_render_target(window, fbo);
            void_render_clear(window, 12, 14, 22, 255);
            void_gfx_draw_ring(window, 160.0f, 160.0f, 90.0f, 110.0f, 40, 180, 255, 180, true);
            void_gfx_draw_regular_polygon(window, 160.0f, 160.0f, 70.0f, 6, (float)angle * 0.02f, 255, 120, 40, 220, false);
            if (sparkle) {
                const int tw = void_texture_get_width(sparkle);
                const int th = void_texture_get_height(sparkle);
                const VoidPoint center = { .x = (float)tw * 0.5f, .y = (float)th * 0.5f };
                void_texture_draw_ex(window, sparkle, 0, 0, 0, 0,
                    160.0f - (float)tw * 0.5f, 160.0f - (float)th * 0.5f, (float)tw, (float)th,
                    -angle * 1.5, &center, VOID_FLIP_NONE);
            }
            void_window_set_render_target(window, NULL);
        }
        // 6. RENDERING PASS 2: MAIN SCREEN & GFX GALLERY
        void_render_clear(window, 18, 18, 24, 255);
        // Background decorative geometry (Capsules, Pies, Rounded Rects, Beziers)
        void_gfx_draw_rounded_rect(window, 30.0f, 30.0f, 260.0f, 140.0f, 20.0f, 35, 40, 55, 200, true);
        void_gfx_draw_capsule(window, 50.0f, 55.0f, 220.0f, 36.0f, 80, 200, 255, 160, true);
        void_gfx_draw_pie(window, 160.0f, 125.0f, 32.0f, 0.0f, M_PIf * 1.35f, 255, 180, 60, 220, true);
        // Center arena circle & rotating 10-point concave star
        void_gfx_draw_circle(window, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f, 150.0f, 40, 42, 54, 200, true);
        void_gfx_draw_circle(window, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f, 150.0f, 180, 190, 210, 255, false);
        VoidPoint star[10];
        const float rot = (float)angle * 0.02f;
        for (int i = 0; i < 10; i++) {
            const float r = (i % 2 == 0) ? 105.0f : 42.0f;
            const float a = rot + (float)i * (2.0f * M_PIf / 10.0f);
            star[i].x = WINDOW_WIDTH * 0.5f + cosf(a) * r;
            star[i].y = WINDOW_HEIGHT * 0.5f + sinf(a) * r;
        }
        if (alt_palette) {
            void_gfx_draw_polygon(window, star, 10, 180, 80, 255, 230, true);
        } else {
            void_gfx_draw_polygon(window, star, 10, 255, 200, 50, 230, true);
        }
        void_gfx_draw_polygon(window, star, 10, 255, 255, 255, 255, false);
        // Dynamic Bezier tether between player and bouncing box
        const float px_center = player_x + player_size * 0.5f;
        const float py_center = player_y + player_size * 0.5f;
        const float bx_center = box_x + box_size * 0.5f;
        const float by_center = box_y + box_size * 0.5f;
        void_gfx_draw_bezier(window, px_center, py_center, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f, bx_center, by_center, 32, 120, 220, 255, 140);
        // Bouncing box & Player body
        void_gfx_draw_rounded_rect(window, box_x, box_y, box_size, box_size, 8.0f, 255, 65, 85, 255, true);
        if (mouse_right_held) {
            void_gfx_draw_rounded_rect(window, player_x, player_y, player_size, player_size, 12.0f, 255, 150, 50, 255, true);
        } else {
            void_gfx_draw_rounded_rect(window, player_x, player_y, player_size, player_size, 12.0f, 50, 240, 120, 255, true);
        }
        // Sparkle sprite overlay on player (using 0,0,0,0 full-texture shortcut)
        if (sparkle) {
            const int tw = void_texture_get_width(sparkle);
            const int th = void_texture_get_height(sparkle);
            const VoidPoint center = { .x = (float)tw * 0.5f, .y = (float)th * 0.5f };
            void_texture_draw_ex(window, sparkle, 0, 0, 0, 0,
                px_center - (float)tw * 0.5f, py_center - (float)th * 0.5f, (float)tw, (float)th,
                angle, &center, VOID_FLIP_NONE);
        }
        // Picture-in-Picture FBO preview (top-right)
        if (fbo) {
            void_texture_draw(window, fbo, 0, 0, 0, 0, WINDOW_WIDTH - 340.0f, 20.0f, 320.0f, 320.0f);
            void_render_rect(window, WINDOW_WIDTH - 340.0f, 20.0f, 320.0f, 320.0f, 200, 210, 230, 255, false);
        }
        // Mouse aiming laser & Scroll-scalable reticle
        if (mouse_left_held) {
            void_gfx_draw_thick_line(window, px_center, py_center, mouse_x, mouse_y, 6.0f, 0, 255, 255, 220);
        }
        void_gfx_draw_ring(window, mouse_x, mouse_y, reticle_radius * 0.65f, reticle_radius, 0, 255, 255, 240, true);
        void_gfx_draw_circle(window, mouse_x, mouse_y, 3.0f, 255, 60, 60, 255, true);
        void_render_present(window);
        // 7. TELEMETRY & O(1) FRAME ARENA RESET
        void_memory_monitor(1000);
        void_log_flush();
        void_arena_reset(s_frame_arena);
    }
    void_memory_set_default_arena(NULL);
    void_audio_destroy_sound(bounce_sfx);
    void_texture_destroy(sparkle);
    void_texture_destroy(fbo);
    void_window_destroy(window);
}

// ============================================================================
// ENTRY POINT
// ============================================================================
int main(const int argc, char *argv[]) {
    if (!void_init()) {
        return -1;
    }
    VoidArena *root = void_memory_init(64 * MiB);
    if (!root) {
        VOID_LOG_FATAL("Failed to allocate master virtual memory!");
        void_exit();
        return -1;
    }
    s_level_arena = void_arena_suballoc(root, "LEVEL", 16 * MiB);
    s_frame_arena = void_arena_suballoc(root, "FRAME", 1 * MiB);
    bool run_all    = argc == 1;
    bool run_system = false;
    bool run_memory = false;
    bool run_file   = false;
    bool run_thread = false;
    bool run_render = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "system") == 0)      run_system = true;
        else if (strcmp(argv[i], "memory") == 0) run_memory = true;
        else if (strcmp(argv[i], "file") == 0)   run_file   = true;
        else if (strcmp(argv[i], "thread") == 0) run_thread = true;
        else if (strcmp(argv[i], "render") == 0) run_render = true;
        else if (strcmp(argv[i], "all") == 0)    run_all    = true;
        else {
            VOID_LOG_WARN("Unknown argument: %s", argv[i]);
            printf("Usage: %s [system] [memory] [file] [thread] [render] [all]\n", argv[0]);
        }
    }
    if (run_all || run_system) test_system_and_logging();
    if (run_all || run_memory) test_memory_arenas();
    if (run_all || run_file)   test_file_and_library();
    if (run_all || run_thread) test_threading();
    if (run_all || run_render) test_interactive_sandbox();
    void_exit();
    return 0;
}