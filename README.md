# VOID Framework (v0.1.0-beta)

*Read in other languages: [Français](README.fr.md)*

## 1. Introduction and Project Vision

**VOID** is a high-performance, deterministic Platform Abstraction Layer (PAL) written in strict **C17** under the **LGPL 3** license. Its objective is to provide a reliable, zero-allocation foundation for intensive real-time graphical applications across both `Windows x86_64` and `Linux x86_64`, engineered for Data-Oriented Design (DOD).

The architecture enforces total isolation from operating system specifics and third-party dependencies through opaque pointers and types (`VoidArena`, `VoidWindow`, `VoidTexture`, `VoidSound`, `VoidMusic`). Low-level syscalls (`VirtualAlloc`/`mmap`, `CreateFile`/`open`, `BCryptGenRandom`/`getrandom`) and multimedia back-ends are strictly encapsulated behind a minimal C-API.

---

## 2. Core Architectural Features

- **Hierarchical & Scoped Linear Arenas:** A single master OS virtual memory reservation (`ROOT_MASTER`) subdivided dynamically into spinlock-protected child arenas (`LEVEL`, `FRAME`). Supports instant $O(1)$ scoped lifecycles (`VoidScope`), in-place top-of-stack reallocations, and cross-arena cut-and-paste promotions (`void_arena_memmove`).
- **Zero-Footprint 2D Geometry (`gfx.c`):** Hardware-accelerated vector tessellation (concave polygon triangulation via *Ear Clipping*, adaptive trigonometric primitives) maintaining a net memory footprint of **0 bytes** after each draw call.
- **Resolution-Aware Input & Hot-Pluggable Gamepads (`input.c`):** Automatic mapping of physical coordinates to a logical canvas (`SDL_RenderWindowToLogical`), scroll wheel tracking, edge detection (`pressed`, `just_pressed`, `just_released`), and support for up to 4 hot-pluggable controllers with radial software deadzones and haptic rumble.
- **Spatialized 2D Audio Mixer (`audio.c`):** Multi-codec playback (`WAV`, `FLAC`, `MOD/XMP`, `MP3`, `OGG`) across 32 hardware channels with individual volume control, real-time 360° stereo panning, and linear distance attenuation.
- **Lock-Free Concurrency & System Entropy (`thread.c`, `core.c`):** Atomic synchronization primitives (`void_atomic_increment`, `void_atomic_compare`), hardware spinlock backoff instructions (`void_processor_pause`), non-blocking ring-buffer logging (`DEET`), direct file I/O with modification timestamps (`mtime`) for live module hot-reloading, and 64-bit OS kernel entropy seeds (`void_get_random_seed`).

---

## 3. Memory Architecture (`memory.c`)

VOID completely eliminates runtime `malloc`/`free` calls in favor of deterministic linear memory arenas:

1. **Master Allocation:** `void_memory_init(total_capacity)` reserves a contiguous virtual memory block from the OS (`ROOT_MASTER`).
2. **Sub-Arena Partitioning:** `void_arena_suballoc(parent, name, capacity)` carves out dedicated child arenas protected by spinlocks (`LEVEL`, `FRAME`).
3. **Scoped Lifecycles:**
   ```c
   VoidScope scope = void_arena_push(arena);
   // ... Fast scratch allocations ...
   void_arena_pop(arena, scope); // Instant O(1) rollback to previous offset
   ```
4. **Data Promotion & Memory Tools:**
- `void_arena_realloc`: In-place reallocation when the block resides at the top of the arena stack.
- `void_arena_memmove`: Direct transfer of data from a temporary arena to a persistent arena while immediately reclaiming the source offset.
- `void_arena_sprintf` / `void_arena_strdup` / `void_arena_strndup`: Zero-fragmentation string formatting and duplication.
5. **Telemetry & Monitoring:** `void_memory_monitor(interval_ms)` logs visual usage gauges formatted using IEC base-2 standards (`B`, `KiB`, `MiB`, `GiB`).

---

## 4. Subsystem Overview

| Module | Key Capabilities |
| :--- | :--- |
| **System & Entropy (`core.c`, `time.c`)** | Subsystem initialization, logical CPU core detection (`void_get_core_count`), 64-bit kernel entropy (`void_get_random_seed`), monotonic timer, and clamped delta-time calculation. |
| **Windowing & 2D Render (`window.c`, `render.c`)** | Bitmask creation/render flags, HighDPI logical canvas scaling, off-screen FBO targets, event polling, and alpha-blended primitives (`SDL_BLENDMODE_BLEND`). |
| **Advanced Geometry (`gfx.c`)** | Zero-footprint tessellation for concave polygons (*Ear Clipping*), circles, ellipses, rings, capsules, pie wedges, rounded rectangles, thick lines, and quadratic Bézier curves. |
| **Human Interface (`input.c`)** | Keyboard polling, logical mouse position & wheel tracking, edge detection, and up to 4 hot-pluggable gamepads with radial deadzones and haptic rumble. |
| **Spatial Audio (`audio.c`)** | 32 hardware channels with discrete channel tracking, per-channel volume, and real-time 2D spatial positioning (`void_audio_set_channel_position`). |
| **File I/O & Dynamic Loading (`file.c`, `library.c`)** | Direct OS syscalls for reading, writing, and copying files, querying file `mtime`, and dynamic symbol loading (`.dll` / `.so`). |
| **Threading & Logging (`thread.c`, `log.c`)** | Detached worker threads, atomic CAS operations, CPU pause instructions, and an allocation-free lock-free ring-buffer logger. |

---

## 5. Building and Running

The build system (`Makefile` & `common.mk`) handles native and cross-compilation target output folders (`bin/linux-x86_64`, `bin/windows-x86_64`) bundling pre-built runtime libraries.

```bash
# Build the test binary and shared library (libVOID.so / VOID.dll)
make all

# Build only the shared library
make lib

# Run the complete test suite
make run
```

---

## 6. Development Roadmap

- **Phase 1: PAL Foundations** [Completed]
  - C17 micro-kernel architecture, master virtual memory reservation, and thread-safe synchronization.
  - Windowing abstraction, accelerated 2D primitive rendering, and basic texture loading.
  - Lock-free ring-buffer logger with severity tags and `DEET` real-time telemetry.
- **Phase 2: PAL Back-end Evolution & Zero-Allocation** [Current]
  - *Scoped & Hierarchical Arenas:* Sub-arena carving (`LEVEL`, `FRAME`), instant $O(1)$ scoped lifecycles (`VoidScope`), cross-arena data promotion (`void_arena_memmove`), and in-place reallocations.
  - *2D Vector Geometry:* Zero-footprint hardware tessellation pipeline (`gfx.c`): concave polygon triangulation (*Ear Clipping*), perfect circles, ellipses, rings, capsules, pie wedges, and quadratic Bézier curves.
  - *Input & Peripherals:* Physical-to-virtual logical coordinate mapping, edge-triggered inputs (`pressed`, `just_pressed`, `just_released`), 4 hot-pluggable controllers with radial deadzones and haptic rumble.
  - *Spatialized Audio Mixer:* Explicit tracking of 32 physical hardware channels, per-channel volume control, real-time 360° stereo panning, and linear distance attenuation.
  - *Native OS Services:* Low-level file I/O (read, write, copy, `mtime` querying for hot-reloading), dynamic symbol loading (`library.c`), and 64-bit kernel hardware entropy (`void_get_random_seed`).
- **Phase 3: Modular `*.void` Architecture** [Next Step]
  - Decoupling into a lightweight `void-core` micro-kernel and hot-swappable dynamic modules (`gfx.void`, `audio.void`, `input.void`, `net.void`).
  - Binary protocol for live module hot-swapping without main-thread or event loop interruption.
- **Phase 4: Binary Asset Containers & Streaming** [Upcoming]
  - Structured binary container format (`assets.void`) enabling direct memory-mapped streaming to VRAM and audio buffers without runtime decompression overhead.
  - Contiguous data streaming via aligned memory mapping.
- **Phase 5: Modern Rendering Pipeline & 3D Abstraction** [Long Term]
  - Migration toward a standalone native OpenGL backend (GLFW, GLAD, VBO/VAO, programmable GLSL shaders).
  - Hybrid 3D pipeline: textured 3D billboarding, normal mapping, and post-processing shaders (dynamic cel-shading, retro pixelation filters).
  - Total decoupling from legacy third-party dependencies, extended portability targeting ARM64, and low-level Vulkan backends.

---

## 7. License

The VOID framework is distributed under the **LGPL 3** license.