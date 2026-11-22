# VOID Framework (`v0.1.0-beta`)

*Read in other languages: [Français](README.fr.md)*

**VOID** is a deterministic, low-level Platform Abstraction Layer (PAL) written in strict **C17** and distributed under the **LGPL 3** license. Designed for Data-Oriented Design (DOD) and real-time graphical applications (serving as the native foundation for the **SOUL** engine), VOID enforces a zero-allocation runtime model and strict isolation from OS specifics and third-party backends.

## 1. Philosophy & Architecture

VOID acts as a strict boundary between application/engine logic and the underlying hardware or operating system:

* **Zero Runtime `malloc`/`free`:** All engine and subsystem allocations are served from a single OS virtual memory reservation subdivided into deterministic, spinlock-protected linear arenas (`VoidArena`).
* **Opaque C17 ABI:** Every handle (`VoidArena`, `VoidWindow`, `VoidTexture`, `VoidSound`, `VoidMusic`) is exposed as an opaque type behind a clean `extern "C"` API (`void.h`), ensuring seamless C17/C++20 interoperability and clean DLL/SO symbol visibility (`VOID_API`).
* **Relocatable Distribution:** Builds produce a shared library (`libVOID.so` / `VOID.dll`), the main executable inside `bin/`, and a lightweight native stub launcher (`VOID_Launcher`) at the root that resolves its own directory, sets the working directory, and forwards execution transparently.

## 2. Subsystems Overview

| Module | Source Files | Key Capabilities |
| :--- | :--- | :--- |
| **System & Time** | `core.c`, `time.c` | Subsystem lifecycle, logical CPU core detection (`void_get_core_count`), 64-bit OS kernel entropy (`void_get_random_seed` via `BCryptGenRandom` / `/dev/urandom`), high-resolution monotonic ticks, and delta-time calculation. |
| **Memory Arenas** | `memory.c` | Single OS virtual memory reservation (`VirtualAlloc` / `mmap`), hierarchical sub-arenas (`ROOT_MASTER`, `LEVEL`, `FRAME`), $O(1)$ scoped rollbacks (`VoidScope`), in-place top-of-stack `realloc`, cross-arena promotion (`void_arena_memmove`), arena string helpers (`strdup`, `strndup`, `sprintf`), and real-time IEC base-2 telemetry (`B`, `KiB`, `MiB`, `GiB`). |
| **Windowing** | `window.c` | HighDPI-aware window creation with bitmask flags, runtime setters/getters (fullscreen, borderless, resizable, mouse grab, minimize/maximize), resize event detection, and logical canvas scaling. |
| **2D Rendering & VRAM** | `render.c`, `texture.c` | Hardware-accelerated 2D primitives (single & batched), texture loading from disk or memory, streaming pixel uploads (`void_texture_update`, `void_texture_update_rect`), off-screen FBO render targets, filtering (`NEAREST`, `LINEAR`, `BEST`), and custom blend modes (including premultiplied alpha). |
| **Zero-Alloc Geometry** | `gfx.c` | Hardware-accelerated 2D vector tessellation using temporary frame-arena allocations (net 0 B footprint per draw): triangles, quads, textured quads, circles, ellipses, rings, capsules, pie wedges, rounded rectangles, thick lines, quadratic Bézier curves, and concave polygons (*Ear Clipping* triangulation). |
| **Human Interface (HID)** | `input.c` | Edge-triggered keyboard, mouse, and controller states (`pressed`, `just_pressed`, `just_released`), logical-canvas mouse coordinates, scroll wheel tracking, and up to 4 hot-pluggable gamepads with radial deadzones and haptic rumble. |
| **Spatial Audio** | `audio.c` | Multi-codec playback (`WAV`, `FLAC`, `MOD`, `MP3`, `OGG`) over 32 hardware channels with per-channel volume, master volume, and real-time 2D spatialization (360° angle panning and distance attenuation). |
| **File I/O & Hot-Reload** | `file.c`, `library.c` | Direct OS syscalls (`CreateFileA`/`open`) for arena-backed file reads, writes, copies, and `mtime` queries, paired with dynamic library loading (`LoadLibraryA`/`dlopen`) for live gameplay module hot-reloading. |
| **Concurrency** | `thread.c` | Detached worker threads, thread IDs, sleep, hardware spinlock backoff (`void_processor_pause`), and lock-free atomic operations (`load`, `store`, `increment`, `decrement`, `compare`). |
| **Lock-Free Logging** | `log.c` | Allocation-free 1024-entry lock-free ring buffer supporting severity levels (`DEET`, `INFO`, `OK`, `WARN`, `ERROR`, `FATAL`), ANSI & Win32 console colors, automatic 1 MiB file rotation, log pruning, hidden log directories, and session headers (version, Git commit, compiler, OS/arch, CPU cores, PID). |

## 3. Supported Platforms & Build Profiles

VOID uses a profile-based build system (`legacy` vs `enhanced`) to balance broad OS compatibility with modern runtimes from a single `Makefile`.

| Target OS | Architectures | `legacy` Profile | `enhanced` Profile | Toolchain |
| :--- | :--- | :--- | :--- | :--- |
| **Linux** | `x86_64`, `x86` | `glibc 2.17` | `glibc 2.31+` | GCC |
| **Windows** | `x86_64`, `x86` | `msvcrt` | `ucrt` | MinGW-w64 GCC |

Pre-compiled dependencies (`SDL2`, `SDL2_image`, `SDL2_mixer`) are resolved automatically from `vendor/lib/<triplet>/`.

## 4. Building, Running & Packaging

### Build Variables

| Variable | Values | Default | Description |
| :--- | :--- | :--- | :--- |
| `OS` | `linux`, `windows` | Host OS | Target operating system. |
| `ARCH` | `x86_64`, `x86` | Host Arch | Target CPU architecture. |
| `MODE` | `debug`, `release` | `debug` | Debug (`-Og -g`, console) or hardened Release (`-O2`, PIE, RELRO, stack protection, `-mwindows`). |
| `TAG` | `legacy`, `enhanced`, *(custom)* | *(none)* | Build profile suffix; selects `glibc 2.17` / `2.31` on Linux and isolates output directories. |
| `GLIBC` | e.g., `2.17`, `2.31` | *(from `TAG`)* | Explicit `glibc` vendor triplet override on Linux. |
| `RUNTIME` | `msvcrt`, `ucrt` | `msvcrt` | Windows C runtime selection (`x86_64`). |
| `ARGS` | Test flags | *(none)* | Arguments forwarded to the executable by `make run`. |

### Common Commands

```bash
# Inspect resolved toolchain, directories, and flags
make info

# Build executable, shared library (libVOID.so / VOID.dll), and stub launcher
make all

# Build only the shared library or the stub launcher
make lib
make stub

# Cross-compile for Windows x86_64 (UCRT) in release mode from Linux
make OS=windows ARCH=x86_64 RUNTIME=ucrt TAG=enhanced MODE=release all

# Build Linux x86_64 targeting the legacy profile (glibc 2.17)
make OS=linux ARCH=x86_64 TAG=legacy MODE=release all

# Run the built binary (automatically uses Wine when cross-compiling Windows on Linux)
make run
make run ARGS="all"

# Create a self-contained release folder and .zip / .tar.gz archives in dist/
make release

# Cleanup targets
make clean      # Remove objects/deps for current target & mode
make fclean     # Remove entire build tree for current target
make distclean  # Remove dist/ archives
make purge      # Remove both build/ and dist/
```

### Built-in Test Suite & Sandbox

The test executable (`src/main.c`) accepts individual module test flags or launches the interactive sandbox by default:

```bash
./build/linux-x86_64/bin/VOID [system] [memory] [file] [thread] [render] [all]
```

### Distribution Layout (`make release`)

```text
dist/VOID-0.1.0-beta-<os>-<arch>[-<tag>]/
├── VOID_Launcher[.exe]   # Relocatable entry point (sets CWD & launches bin/VOID)
├── bin/
│   ├── VOID[.exe]        # Main binary
│   └── *.dll             # (Windows) VOID.dll and runtime dependencies
├── lib/
│   ├── libVOID.so        # (Linux) Shared library and vendor .so files
│   └── libVOID.dll.a     # (Windows) Import library for linking
├── include/              # Public C17 headers (void.h, voiddef.h)
├── assets/               # Runtime assets (if present)
└── README.md
```

## 5. Minimal Quickstart (C17)

```c
#include "void.h"

int main(void) {
    if (!void_init()) return -1;

    VoidArena *root  = void_memory_init(64 * MiB);
    VoidArena *frame = void_arena_suballoc(root, "FRAME", 4 * MiB);

    VoidWindow *window = void_window_create(
        "VOID Quickstart",
        VOID_WINDOWPOS_CENTERED, VOID_WINDOWPOS_CENTERED,
        1280, 720,
        VOID_WINDOW_SHOWN | VOID_WINDOW_RESIZABLE,
        VOID_RENDER_ACCELERATED | VOID_RENDER_PRESENTVSYNC
    );

    void_memory_set_default_arena(frame);

    while (void_window_is_running(window)) {
        void_window_poll_events(window);
        if (void_window_should_close(window) || void_input_is_key_just_pressed(VOID_KEY_ESCAPE)) {
            void_window_close(window);
        }

        void_render_clear(window, 18, 18, 24, 255);
        void_gfx_draw_circle(window, 640.0f, 360.0f, 80.0f, 80, 200, 255, 255, true);
        void_render_present(window);

        void_log_flush();
        void_arena_reset(frame); // O(1) per-frame cleanup
    }

    void_memory_set_default_arena(NULL);
    void_window_destroy(window);
    void_exit();
    return 0;
}
```

## 6. Development Roadmap

### `v0.1.x` — PAL Consolidation & ARM64 Foundations *(Current Branch)*

* [x] Strict C17 micro-kernel, hierarchical/scoped virtual memory arenas (`VoidArena` / `VoidScope`), and lock-free ring-buffer logger with file rotation.
* [x] Windowing, 2D rendering, FBO targets, zero-allocation vector geometry (`gfx.c`), logical-resolution input, 4-player gamepads, and 32-channel spatial audio.
* [x] Native file I/O (`mtime`, copy, arena read), dynamic library hot-reloading, 64-bit hardware entropy, and relocatable `VOID_Launcher` stub.
* [x] Multi-profile GCC / MinGW-w64 build system (`legacy` / `enhanced`) for Linux & Windows (`x86_64`, `x86`) with automated release packaging.
* [ ] **Linux `arm64` support** via GCC cross-compilation integrated into the `Makefile` and profile system.
* [ ] **Windows `arm64` support** (via MinGW-w64 GCC as toolchain support matures, or `llvm-mingw`).

### `v0.2.0` — Native OpenGL Stack, Typography & Clang Integration *(Next Major)*

* [ ] **Migration from SDL2 to a Native OpenGL Stack:** Replace `SDL2`, `SDL2_image`, and `SDL2_mixer` with a lower-level stack (**OpenGL**, **GLFW**, **GLAD**, **OpenAL**) to unlock programmable GLSL shaders, VBO/VAO batching, and full rendering control.
* [ ] **Native Typography (`FreeType`):** Introduce **FreeType** for high-quality vector font rasterization and glyph atlas generation.
* [ ] **Clang/LLVM Integration:** Add first-class Clang/LLVM support alongside GCC to broaden cross-compilation across desktop, mobile, web, and console targets.
* [ ] **Build System Evolution (`CMake`):** Introduce **CMake** (alongside or replacing the `Makefile`) for seamless downstream integration with **SOUL** and modern IDEs.

### Long-Term Horizon

* [ ] **Modular `*.void` Architecture:** Split into a minimal `void-core` kernel and hot-swappable modules (`gfx.void`, `audio.void`, `input.void`, `net.void`).
* [ ] **Binary Asset Containers (`assets.void`):** Memory-mapped, aligned asset packs for zero-overhead streaming into VRAM and audio buffers.
* [ ] **Multi-Backend RHI & Extended Targets:** Abstract the rendering layer to support **Vulkan**, **DirectX 11/12**, and **Metal**, expanding portability to **macOS / iOS**, **Android**, **WebAssembly**, and console platforms.

## 7. License

The **VOID** framework is distributed under the **GNU Lesser General Public License v3.0 (LGPL-3.0)**.
