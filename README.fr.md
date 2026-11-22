# Framework VOID (`v0.1.0-beta`)

*Lire dans d'autres langues : [English](README.md)*

**VOID** est une couche d'abstraction de plateforme (PAL) bas niveau et déterministe, écrite en **C17** strict et distribuée sous licence **LGPL 3**. Conçu pour le *Data-Oriented Design* (DOD) et les applications graphiques temps réel (notamment en tant que socle natif du moteur **SOUL**), VOID impose un modèle d'exécution sans allocation dynamique et une isolation stricte vis-à-vis du système d'exploitation et des dépendances tierces.

## 1. Philosophie & Architecture

VOID établit une frontière stricte entre la logique applicative/moteur et le matériel ou le système d'exploitation sous-jacent :

* **Zéro `malloc`/`free` à l'exécution :** Toutes les allocations du moteur et des sous-systèmes sont servies depuis une unique réservation de mémoire virtuelle OS, subdivisée en arènes linéaires déterministes protégées par spinlocks (`VoidArena`).
* **ABI C17 Opaque :** Chaque ressource (`VoidArena`, `VoidWindow`, `VoidTexture`, `VoidSound`, `VoidMusic`) est exposée sous forme de type opaque derrière une API `extern "C"` épurée (`void.h`), garantissant une interopérabilité native C17/C++20 et une gestion propre de la visibilité des symboles DLL/SO (`VOID_API`).
* **Distribution Relocalisable :** La compilation produit une bibliothèque partagée (`libVOID.so` / `VOID.dll`), l'exécutable principal dans `bin/`, ainsi qu'un lanceur natif léger (`VOID_Launcher`) à la racine qui résout son propre répertoire, configure le dossier de travail et relaie l'exécution de manière transparente.

## 2. Vue d'ensemble des Sous-systèmes

| Module | Fichiers sources | Capacités clés |
| :--- | :--- | :--- |
| **Système & Temps** | `core.c`, `time.c` | Cycle de vie des sous-systèmes, détection des cœurs CPU logiques (`void_get_core_count`), entropie noyau 64 bits (`void_get_random_seed` via `BCryptGenRandom` / `/dev/urandom`), horloge monotone haute résolution et calcul du delta-time. |
| **Arènes Mémoire** | `memory.c` | Réservation unique de mémoire virtuelle OS (`VirtualAlloc` / `mmap`), sous-arènes hiérarchiques (`ROOT_MASTER`, `LEVEL`, `FRAME`), retours arrière par portée en $O(1)$ (`VoidScope`), réallocation sur place en sommet de pile (`realloc`), promotion inter-arènes (`void_arena_memmove`), utilitaires de chaînes (`strdup`, `strndup`, `sprintf`) et télémétrie temps réel aux normes IEC base 2 (`B`, `KiB`, `MiB`, `GiB`). |
| **Fenêtrage** | `window.c` | Création de fenêtre compatible HighDPI par masques de bits, mutateurs/accesseurs à chaud (plein écran, sans bordure, redimensionnable, capture souris, réduction/maximisation), détection de redimensionnement et mise à l'échelle logique du canevas. |
| **Rendu 2D & VRAM** | `render.c`, `texture.c` | Primitives 2D accélérées matériellement (unitaires et par lots), chargement de textures depuis le disque ou la mémoire, mise à jour de pixels en streaming (`void_texture_update`, `void_texture_update_rect`), cibles de rendu hors écran (FBO), filtrage (`NEAREST`, `LINEAR`, `BEST`) et modes de fusion personnalisés (dont l'alpha prémultiplié). |
| **Géométrie Zéro-Alloc** | `gfx.c` | Tessellation vectorielle 2D accélérée utilisant des allocations temporaires dans l'arène de frame (empreinte nette de 0 octet par appel de dessin) : triangles, quads, quads texturés, cercles, ellipses, anneaux, capsules, secteurs circulaires, rectangles arrondis, lignes épaisses, courbes de Bézier quadratiques et polygones concaves (triangulation par *Ear Clipping*). |
| **Interfaces Humaines (HID)** | `input.c` | Détection d'états et de fronts clavier, souris et manettes (`pressed`, `just_pressed`, `just_released`), coordonnées souris alignées sur la résolution logique, suivi de la molette et support jusqu'à 4 manettes branchables à chaud avec zones mortes radiales et retour haptique. |
| **Audio Spatialisé** | `audio.c` | Lecture multi-codecs (`WAV`, `FLAC`, `MOD`, `MP3`, `OGG`) sur 32 canaux matériels avec contrôle de volume individuel, volume principal et spatialisation 2D en temps réel (panoramique à 360° et atténuation linéaire selon la distance). |
| **Fichiers & Hot-Reload** | `file.c`, `library.c` | Appels système directs (`CreateFileA`/`open`) pour la lecture dans l'arène, l'écriture, la copie et la lecture de `mtime`, couplés au chargement dynamique de bibliothèques (`LoadLibraryA`/`dlopen`) pour le rechargement à chaud des modules de gameplay. |
| **Concurrence** | `thread.c` | Threads workers détachés, identifiants de threads, mise en veille, pause matérielle de spinlock (`void_processor_pause`) et opérations atomiques lock-free (`load`, `store`, `increment`, `decrement`, `compare`). |
| **Journalisation Lock-Free** | `log.c` | Tampon circulaire lock-free de 1024 entrées sans allocation, niveaux de sévérité (`DEET`, `INFO`, `OK`, `WARN`, `ERROR`, `FATAL`), coloration console ANSI et Win32, rotation automatique de fichiers à 1 MiB, purge des anciens logs, dossiers cachés et en-têtes de session (version, commit Git, compilateur, OS/arch, cœurs CPU, PID). |

## 3. Plateformes Supportées & Profils de Build

VOID s'appuie sur un système de build par profils (`legacy` vs `enhanced`) pour concilier une compatibilité maximale avec les anciens systèmes et l'exploitation des runtimes modernes depuis un unique `Makefile`.

| OS Cible | Architectures | Profil `legacy` | Profil `enhanced` | Toolchain |
| :--- | :--- | :--- | :--- | :--- |
| **Linux** | `x86_64`, `x86` | `glibc 2.17` | `glibc 2.31+` | GCC |
| **Windows** | `x86_64`, `x86` | `msvcrt` | `ucrt` | MinGW-w64 GCC |

Les dépendances précompilées (`SDL2`, `SDL2_image`, `SDL2_mixer`) sont résolues automatiquement depuis `vendor/lib/<triplet>/`.

## 4. Compilation, Exécution & Packaging

### Variables de Build

| Variable | Valeurs | Défaut | Description |
| :--- | :--- | :--- | :--- |
| `OS` | `linux`, `windows` | OS hôte | Système d'exploitation cible. |
| `ARCH` | `x86_64`, `x86` | Arch hôte | Architecture CPU cible. |
| `MODE` | `debug`, `release` | `debug` | Debug (`-Og -g`, console) ou Release durcie (`-O2`, PIE, RELRO, protection de pile, `-mwindows`). |
| `TAG` | `legacy`, `enhanced`, *(libre)* | *(aucun)* | Suffixe de profil ; sélectionne `glibc 2.17` / `2.31` sous Linux et isole les dossiers de sortie. |
| `GLIBC` | ex. `2.17`, `2.31` | *(via `TAG`)* | Surcharge explicite du triplet `glibc` sous Linux. |
| `RUNTIME` | `msvcrt`, `ucrt` | `msvcrt` | Sélection du runtime C Windows (`x86_64`). |
| `ARGS` | Drapeaux de test | *(aucun)* | Arguments transmis à l'exécutable lors de `make run`. |

### Commandes Courantes

```bash
# Inspecter la toolchain résolue, les répertoires et les flags
make info

# Compiler l'exécutable, la bibliothèque partagée (libVOID.so / VOID.dll) et le lanceur
make all

# Compiler uniquement la bibliothèque partagée ou le lanceur
make lib
make stub

# Cross-compiler pour Windows x86_64 (UCRT) en mode release depuis Linux
make OS=windows ARCH=x86_64 RUNTIME=ucrt TAG=enhanced MODE=release all

# Compiler pour Linux x86_64 en ciblant le profil legacy (glibc 2.17)
make OS=linux ARCH=x86_64 TAG=legacy MODE=release all

# Lancer le binaire compilé (utilise automatiquement Wine pour Windows depuis Linux)
make run
make run ARGS="all"

# Générer un dossier de distribution autonome et les archives .zip / .tar.gz dans dist/
make release

# Cibles de nettoyage
make clean      # Supprime les objets/dépendances pour la cible et le mode courants
make fclean     # Supprime tout l'arbre de build de la cible courante
make distclean  # Supprime les archives dans dist/
make purge      # Supprime intégralement build/ et dist/
```

### Suite de Tests Intégrée & Bac à Sable

L'exécutable de test (`src/main.c`) accepte des drapeaux par module ou lance par défaut le bac à sable interactif :

```bash
./build/linux-x86_64/bin/VOID [system] [memory] [file] [thread] [render] [all]
```

### Structure de Distribution (`make release`)

```text
dist/VOID-0.1.0-beta-<os>-<arch>[-<tag>]/
├── VOID_Launcher[.exe]   # Point d'entrée relocalisable (définit le CWD et lance bin/VOID)
├── bin/
│   ├── VOID[.exe]        # Binaire principal
│   └── *.dll             # (Windows) VOID.dll et dépendances runtime
├── lib/
│   ├── libVOID.so        # (Linux) Bibliothèque partagée et fichiers .so vendor
│   └── libVOID.dll.a     # (Windows) Bibliothèque d'importation pour l'édition de liens
├── include/              # En-têtes publics C17 (void.h, voiddef.h)
├── assets/               # Ressources runtime (si présentes)
└── README.md
```

## 5. Démarrage Rapide (C17)

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
        void_arena_reset(frame); // Nettoyage O(1) à chaque frame
    }

    void_memory_set_default_arena(NULL);
    void_window_destroy(window);
    void_exit();
    return 0;
}
```

## 6. Feuille de Route

### `v0.1.x` — Consolidation de la PAL & Fondations ARM64 *(Branche actuelle)*

* [x] Micro-noyau C17 strict, arènes de mémoire virtuelle hiérarchiques/scopées (`VoidArena` / `VoidScope`) et logger lock-free avec rotation de fichiers.
* [x] Fenêtrage, rendu 2D, cibles FBO, géométrie vectorielle zéro-allocation (`gfx.c`), entrées adaptées à la résolution logique, 4 manettes et audio spatialisé sur 32 canaux.
* [x] Entrées/sorties fichiers natives (`mtime`, copie, lecture en arène), chargement dynamique pour hot-reload, entropie matérielle 64 bits et lanceur relocalisable `VOID_Launcher`.
* [x] Système de build GCC / MinGW-w64 multi-profils (`legacy` / `enhanced`) pour Linux & Windows (`x86_64`, `x86`) avec packaging automatisé.
* [ ] **Support Linux `arm64`** via cross-compilation GCC intégrée au `Makefile` et au système de profils.
* [ ] **Support Windows `arm64`** (via MinGW-w64 GCC dès maturité de la toolchain, ou `llvm-mingw`).

### `v0.2.0` — Stack OpenGL Native, Typographie & Intégration Clang *(Prochaine majeure)*

* [ ] **Migration de SDL2 vers une stack OpenGL native :** Remplacement de `SDL2`, `SDL2_image` et `SDL2_mixer` par une pile bas niveau (**OpenGL**, **GLFW**, **GLAD**, **OpenAL**) pour débloquer les shaders GLSL programmables, le batching VBO/VAO et un contrôle total du pipeline de rendu.
* [ ] **Typographie native (`FreeType`) :** Ajout de **FreeType** pour la rastérisation vectorielle de polices et la génération d'atlas de glyphes.
* [ ] **Intégration Clang/LLVM :** Support complet de Clang/LLVM aux côtés de GCC pour étendre la cross-compilation vers le desktop, le mobile, le web et les consoles.
* [ ] **Évolution du système de build (`CMake`) :** Introduction de **CMake** (en complément ou remplacement du `Makefile`) pour faciliter l'intégration dans **SOUL** et les IDE modernes.

### Horizon Long Terme

* [ ] **Architecture modulaire `*.void` :** Découpage en un micro-noyau `void-core` et des modules rechargeables à chaud (`gfx.void`, `audio.void`, `input.void`, `net.void`).
* [ ] **Conteneurs d'assets binaires (`assets.void`) :** Packs binaires alignés et mappés en mémoire pour le streaming direct vers la VRAM et les tampons audio.
* [ ] **RHI multi-backends & cibles étendues :** Abstraction du rendu vers **Vulkan**, **DirectX 11/12** et **Metal**, et portage élargi vers **macOS / iOS**, **Android**, **WebAssembly** et plateformes consoles.

## 7. Licence

Le framework **VOID** est distribué sous licence **GNU Lesser General Public License v3.0 (LGPL-3.0)**.
