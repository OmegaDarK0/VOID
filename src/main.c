#include <stdio.h>
#include <string.h>
#include "void.h"

// --- CONSTANTS ---
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

// ============================================================================
// TEST 1: LOGGING
// ============================================================================
static void test_logging(void) {
    VOID_LOG_INFO("Starting VOID FRAMEWORK test suite...");
    VOID_LOG_OK("Log test OK.");
    VOID_LOG_WARN("This is a test warning.");
    VOID_LOG_DEBUG("This is a magenta DEET debug tag!");
    // VOID_LOG_ERROR("This is a test error.");
    void_log_flush();
}

// ============================================================================
// TEST 2: MEMORY ARENAS (ECS DOD Simulation)
// ============================================================================
static void test_memory_arenas(void) {
    VOID_LOG_INFO("[MEMORY TEST] Allocating persistent data...");
    const VoidArenaMarker initial_marker = void_arena_get_marker();
    void_arena_alloc(16 * MiB, 16);
    void_memory_print();
    VOID_LOG_INFO("[MEMORY TEST] Simulating 3 ECS frames (Frame Arena)...");
    for (int frame = 0; frame < 3; frame++) {
        for (int i = 0; i < 10000; i++) {
            void_frame_alloc(64, 8);
        }
        VOID_LOG_INFO("  -> Frame %d finished. Cleaning frame arena.", frame);
        void_frame_free();
    }
    void_memory_print();
    void_arena_rollback_to(initial_marker);
    VOID_LOG_INFO("[MEMORY TEST] Test finished successfully. Arena rolled back.");
}

// ============================================================================
// TEST 3: FILE I/O (NATIVE OS CALLS)
// ============================================================================
static void test_file_io(void) {
    VOID_LOG_INFO("[FILE TEST] Testing native OS file I/O (Zero Cost)...");
    const VoidArenaMarker marker = void_arena_get_marker();
    uint64 file_size = 0;
    void *data = void_file_read("README.md", &file_size);
    if (data) {
        VOID_LOG_OK("[FILE TEST] Successfully read README.md (%" PRIu64 " bytes).", file_size);
        VOID_LOG_DEBUG("[FILE TEST] Snippet: %.15s...", (char*)data);
    } else {
        VOID_LOG_WARN("[FILE TEST] README.md not found, but API handled it gracefully.");
    }
    void_arena_rollback_to(marker);
    VOID_LOG_INFO("[FILE TEST] Arena restored.");
}

// ============================================================================
// TEST 4: MULTITHREADING
// ============================================================================
static volatile uint32 s_shared_counter = 0;

static void thread_stress_task(void *data) {
    const uint32 increments_to_do = (uint32)(uintptr_t)data;
    for (uint32 i = 0; i < increments_to_do; i++) {
        void_atomic_increment(&s_shared_counter);
    }
}

static void test_thread_fibers(void) {
    const uint32 cores = void_get_core_count();
    const uint32 increments_per_thread = 1000000;
    const uint32 target = cores * increments_per_thread;
    s_shared_counter = 0;
    VOID_LOG_INFO("[THREAD TEST] CPU detected: %u logical cores.", cores);
    VOID_LOG_INFO("[THREAD TEST] Launching %u threads (Target: %u)...", cores, target);
    for (uint32 i = 0; i < cores; i++) {
        void_thread_create(thread_stress_task, (void*)(uintptr_t)increments_per_thread);
    }
    while (s_shared_counter < target) {
        void_thread_sleep(10);
    }
    VOID_LOG_OK("[THREAD TEST] Success! Final counter: %u", s_shared_counter);
}

// ============================================================================
// TEST 5: RENDERING, AUDIO, DELTA TIME AND INPUT
// ============================================================================
static void test_render_loop(void) {
    VOID_LOG_INFO("[RENDER TEST] Creating window...");
    VoidWindow *window = void_window_create(
        "VOID FRAMEWORK",
        VOID_WINDOWPOS_CENTERED, VOID_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        VOID_WINDOW_SHOWN | VOID_WINDOW_RESIZABLE,
        VOID_RENDER_ACCELERATED | VOID_RENDER_PRESENTVSYNC
        );
    if (!window) {
        VOID_LOG_FATAL("Failed to create window.");
        return;
    }
    void_render_set_logical_size(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    float player_x = WINDOW_WIDTH / 2.0f;
    float player_y = WINDOW_HEIGHT / 2.0f;
    float box_x = 100.0f;
    float box_y = 100.0f;
    float box_vel_x = 400.0f;
    float box_vel_y = 350.0f;
    // --- TODO AUDIO ---
    // VoidMusic* bgm = void_audio_load_music("assets/music.ogg");
    // void_audio_play_music(bgm, -1);
    // VoidSound* bounce_sfx = void_audio_load_sound("assets/bounce.wav");
    VOID_LOG_INFO("[RENDER TEST] Starting main loop.");
    while (void_window_is_running(window)) {
        const float box_size = 32.0f;
        const float player_speed = 600.0f;
        const float player_size = 64.0f;
        // 1. TIME MANAGEMENT
        const float dt = void_time_get_delta();
        // 2. OS EVENTS
        void_window_poll_events(window);
        if (void_window_should_close(window) || void_input_is_key_pressed(VOID_KEY_ESCAPE)) {
            void_window_close(window);
        }
        // 3. GAME LOGIC
        if (void_input_is_key_pressed(VOID_KEY_W)) player_y -= player_speed * dt;
        if (void_input_is_key_pressed(VOID_KEY_S)) player_y += player_speed * dt;
        if (void_input_is_key_pressed(VOID_KEY_A)) player_x -= player_speed * dt;
        if (void_input_is_key_pressed(VOID_KEY_D)) player_x += player_speed * dt;
        box_x += box_vel_x * dt;
        box_y += box_vel_y * dt;
        if (box_x <= 0 || box_x + box_size >= WINDOW_WIDTH) {
            box_vel_x *= -1.0f;
            // void_audio_play_sound(bounce_sfx);
        }
        if (box_y <= 0 || box_y + box_size >= WINDOW_HEIGHT) {
            box_vel_y *= -1.0f;
            // void_audio_play_sound(bounce_sfx);
        }
        // 4. RENDERING
        void_render_clear(window, 20, 20, 25, 255);
        void_render_rect(window, box_x, box_y, box_size, box_size, 255, 50, 50, 255, true);
        void_render_rect(window, player_x, player_y, player_size, player_size, 50, 255, 100, 255, true);
        void_render_present(window);
        // 5. END OF FRAME CLEANUP
        void_log_flush();
        void_frame_free();
    }
    VOID_LOG_INFO("[RENDER TEST] Closing window.");
    void_window_destroy(window);
}

// ============================================================================
// TEST: HOT-RELOADING (DYNAMIC LIBRARY)
// ============================================================================
void test_hot_reload(void) {
    VOID_LOG_INFO("[HOT-RELOAD TEST] Attempting to load a non-existent gameplay library...");
    void *gameplay_dll = void_library_load("gameplay_logic.dll");
    if (gameplay_dll) {
        VOID_LOG_OK("[HOT-RELOAD TEST] Library loaded successfully!");
        void *update_func = void_library_get_symbol(gameplay_dll, "soul_on_update");
        if (update_func) {
            VOID_LOG_OK("[HOT-RELOAD TEST] Function 'soul_on_update' found at %p", update_func);
        }
        void_library_free(gameplay_dll);
    } else {
        VOID_LOG_WARN("[HOT-RELOAD TEST] Handled library load failure perfectly. The engine did not crash.");
    }
}

// ============================================================================
// ENTRY POINT
// ============================================================================
int main(const int argc, char *argv[]) {
    if (!void_init()) {
        return -1;
    }
    if (!void_memory_init(32 * MiB, 32 * KiB)) {
        VOID_LOG_FATAL("Failed to allocate global engine memory!");
        void_exit();
        return -1;
    }
    bool run_all = argc == 1;
    bool run_memory = false;
    bool run_file = false;
    bool run_thread = false;
    bool run_render = false;
    // Parsing
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "memory") == 0) run_memory = true;
        else if (strcmp(argv[i], "file") == 0) run_file = true;
        else if (strcmp(argv[i], "thread") == 0) run_thread = true;
        else if (strcmp(argv[i], "render") == 0) run_render = true;
        else if (strcmp(argv[i], "all") == 0) run_all = true;
        else {
            VOID_LOG_WARN("Unknown argument: %s", argv[i]);
            printf("Usage: %s [memory] [file] [thread] [render] [all]\n", argv[0]);
        }
    }
    test_logging();
    if (run_all || run_memory) test_memory_arenas();
    if (run_all || run_file)   test_file_io();
    if (run_all || run_thread) test_thread_fibers();
    if (run_all || run_render) test_render_loop();
    void_exit();
    return 0;
}