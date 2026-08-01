# VOID Framework (v0.1.0-alpha)

*Read in other languages: [Français](README.fr.md)*

## 1. Introduction and Project Vision

VOID is a high-performance Platform Abstraction Layer (PAL) developed in C17. Its mission is to provide a robust and portable architectural foundation for game engine development, similar to the SOUL engine. The architecture enforces total system isolation through the exclusive use of opaque pointers — notably `VoidWindow`, `VoidTexture`, and `VoidRender` — thereby hiding the SDL2 implementation from the Front-end.

> **Operational Note:** VOID exposes a minimalist C-API designed to be efficiently consumed by high-level C++ engines, ensuring a strict separation between application logic and system services.

## 2. Core Features

* **Arena Memory Management:** Implementation of a "Zero-Allocation" paradigm during the game loop. The system fragments pre-allocated memory between a Global Arena (persistent data) and a Frame Arena (temporary data).
* **Lock-Free Primitives:** Heavy use of C standard atomic operations for task queue management and synchronization, eliminating bottlenecks and context switches associated with mutexes.
* **Dynamic Distribution:** Native cross-platform support via the generation of shared libraries (`.dll` for Windows, `.so` for Linux), with automated dependency management to architecture-specific binary directories.
* **PAL Architecture:** Exhaustive abstraction of low-level services including window management, 2D graphical rendering, high-resolution time management, and immediate processing of user inputs.

## 3. Memory Architecture (Zero-Allocation)

The memory management system, defined in `void.h`, is optimized for Data-Oriented Design (DOD). The systematic use of alignment (`alignof`) and contiguous arenas promotes cache locality and Structure of Arrays (SoA) data layouts. Initialization must respect the operational order: `void_init()` followed by `void_memory_init()`.

* **`void_arena_alloc(uint64 size, uint32 alignment)`**
  This function allocates persistent blocks in the global arena. Manual alignment ensures data integrity on strict architectures and optimizes read performance.

* **`void_frame_alloc(uint64 size, uint32 alignment)`**
  Ultra-fast allocation in the frame arena, representing 10% of total memory (Factor 0.1). This zone is instantly freed at the end of each cycle via `void_frame_free()`, with zero CPU cost.

## 4. Task System and Multithreading

VOID implements a *job system* of the "Fibers-preparatory" type designed to minimize OS scheduler intervention. The system instantiates a pool of worker threads based on the number of logical cores detected via `void_system_get_core_count()`.

Task synchronization and distribution are performed lock-free using the atomic primitives `void_atomic_increment`, `void_atomic_decrement`, and `void_atomic_compare_exchange`. This approach guarantees smooth execution of massively parallel systems such as physics or ECS spatial integration.

## 5. Rendering and Input Specifications

| Feature | Implementation |
| :--- | :--- |
| **2D Primitives** | Points, lines, and rectangles driven by the `VoidRender` backend. |
| **Texture Management** | Opaque handles via `VoidTexture` with loading through `void_texture_load`. |
| **Input** | Immediate state reading; `VoidKeyCode` mapped directly to `SDL_SCANCODE`. |
| **Backend Rendering** | Current implementation via `SDL_Renderer` with native HighDPI support. |

## 6. Development Roadmap

1. **Rendering:** Migration of the current pipeline to a modern OpenGL-based rendering API for increased GPU control.
2. **Legacy Branches:** Maintenance and stabilization of support for SDL2 and the x86 processor architecture.
3. **Enhanced Branches:** Specific optimizations for the UCRT environment and extended compatibility for the ARM64 architecture.

## 7. Compilation and Deployment

The build system relies on `common.mk` to automate cross-compilation. The process generates binaries and dynamic libraries (`libVOID.dll` / `libVOID.so`) in subdirectories structured by target, for example, `bin/windows-x86_64` or `bin/linux-x86_64`.

To compile the entire framework and its library:
```bash
make all
```

To generate only the shared library:
```bash
make lib
```

## 8. License

The VOID framework is distributed under the **LGPL 3** license.