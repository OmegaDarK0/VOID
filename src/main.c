#include <stdio.h>
#include <string.h>
#include "void.h"

// --- CONSTANTS ---
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define ANIMATION_SPEED 50

// ============================================================================
// TEST 1: LOGGING
// ============================================================================
void test_logging(void) {
    VOID_LOG_INFO("Starting SOUL ENGINE test suite...");
    VOID_LOG_OK("Log test OK.");
    VOID_LOG_WARN("This is a test warning.");
    // VOID_LOG_ERROR("This is a test error.");
    void_log_flush();
}

// ============================================================================
// TEST 2: MEMORY ARENAS (ECS DOD Simulation)
// ============================================================================
void test_memory_arenas(void) {
    VOID_LOG_INFO("[MEMORY TEST] Allocating persistent data...");
    const VoidArenaMarker initial_marker = void_arena_get_marker();
    void_arena_alloc(16 * MB, 16);
    void_memory_print();
    VOID_LOG_INFO("[MEMORY TEST] Simulating 3 ECS frames (Frame Arena)...");
    for (int frame = 0; frame < 3; frame++) {
        // Simulating the creation of 10,000 temporary components
        for (int i = 0; i < 10000; i++) {
            void_frame_alloc(64, 8); // 64 bytes per component
        }
        VOID_LOG_INFO("  -> Frame %d finished. Cleaning frame arena.", frame);
        void_frame_free(); // Zero cost, instant cleanup
    }
    void_memory_print();
    void_arena_rollback_to(initial_marker);
    VOID_LOG_INFO("[MEMORY TEST] Test finished successfully.");
}

// ============================================================================
// TEST 3: MULTITHREADING (Preparation for job.cpp)
// ============================================================================
static volatile uint32 s_shared_counter = 0;

void thread_stress_task(void *data) {
    const uint32 increments_to_do = (uint32)(uintptr_t)data;
    for (uint32 i = 0; i < increments_to_do; i++) {
        void_atomic_increment(&s_shared_counter);
    }
}

void test_thread_fibers(void) {
    const uint32 cores = void_system_get_core_count();
    const uint32 increments_per_thread = 1000000;
    const uint32 target = cores * increments_per_thread;
    s_shared_counter = 0;
    VOID_LOG_INFO("[THREAD TEST] CPU detected: %u logical cores.", cores);
    VOID_LOG_INFO("[THREAD TEST] Launching %u threads (Target: %u)...", cores, target);
    for (uint32 i = 0; i < cores; i++) {
        void_thread_create(thread_stress_task, (void*)(uintptr_t)increments_per_thread);
    }
    while (s_shared_counter < target) {
        void_thread_sleep(10); // Avoid burning the CPU while waiting
    }
    VOID_LOG_OK("[THREAD TEST] Success! Final counter: %u", s_shared_counter);
}

// ============================================================================
// TEST 4: RENDERING, DELTA TIME AND INPUT
// ============================================================================
void test_render_loop(void) {
    VOID_LOG_INFO("[RENDER TEST] Creating window...");
    VoidWindow *window = void_window_create("SOUL ENGINE - Render Test", WINDOW_WIDTH, WINDOW_HEIGHT);

    if (!window) {
        VOID_LOG_FATAL("Failed to create window.");
        return;
    }
    // --- Player Variables ---
    float player_x = WINDOW_WIDTH / 2.0f;
    float player_y = WINDOW_HEIGHT / 2.0f;
    // --- Autonomous Object Variables (Bouncing Box) ---
    float box_x = 100.0f;
    float box_y = 100.0f;
    float box_vel_x = 400.0f;
    float box_vel_y = 350.0f;
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
        // 3. GAME LOGIC (Resolution independent thanks to DT)
        // -> Player
        if (void_input_is_key_pressed(VOID_KEY_W)) player_y -= player_speed * dt;
        if (void_input_is_key_pressed(VOID_KEY_S)) player_y += player_speed * dt;
        if (void_input_is_key_pressed(VOID_KEY_A)) player_x -= player_speed * dt;
        if (void_input_is_key_pressed(VOID_KEY_D)) player_x += player_speed * dt;
        // -> Bouncing Box (Autonomous physics)
        box_x += box_vel_x * dt;
        box_y += box_vel_y * dt;
        // Simple screen edge collisions for the box
        if (box_x <= 0 || box_x + box_size >= WINDOW_WIDTH)  box_vel_x *= -1.0f;
        if (box_y <= 0 || box_y + box_size >= WINDOW_HEIGHT) box_vel_y *= -1.0f;
        // 4. RENDERING
        void_render_clear(window, 20, 20, 25, 255); // Very dark gray background
        // Draw Bouncing Box (Red)
        void_render_rect(window, box_x, box_y, box_size, box_size, 255, 50, 50, 255, true);
        // Draw Player (Neon Green)
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
// ENTRY POINT
// ============================================================================
int main(const int argc, char *argv[]) {
    // Global Back-end initialization
    if (!void_init()) {
        return -1;
    }
    if (!void_memory_init(32 * MB, 32 * KB)) {
        VOID_LOG_FATAL("Failed to allocate global engine memory!");
        void_exit();
        return -1;
    }
    // If no argument is passed, run everything by default
    bool run_all = argc == 1;
    bool run_memory = false;
    bool run_thread = false;
    bool run_render = false;
    // Parsing arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "memory") == 0) {
            run_memory = true;
        } else if (strcmp(argv[i], "thread") == 0) {
            run_thread = true;
        } else if (strcmp(argv[i], "render") == 0) {
            run_render = true;
        } else if (strcmp(argv[i], "all") == 0) {
            run_all = true;
        } else {
            VOID_LOG_WARN("Unknown argument: %s", argv[i]);
            printf("Usage: %s [memory] [thread] [render] [all]\n", argv[0]);
        }
    }
    // Conditional execution
    test_logging(); // Always keep the log test to verify it works
    if (run_all || run_memory) {
        test_memory_arenas();
    }
    if (run_all || run_thread) {
        test_thread_fibers();
    }
    // The rendering test is blocking, always run it last
    if (run_all || run_render) {
        test_render_loop();
    }
    // Global Back-end shutdown
    void_exit();
    return 0;
}