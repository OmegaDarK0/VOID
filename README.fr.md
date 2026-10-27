# VOID Framework (v0.1.0-beta)

*Lire dans d'autres langues : [English](README.md)*

## 1. Introduction et Vision du Projet

**VOID** est une couche d'abstraction de plateforme (PAL) haute performance, garantie sans allocation dynamique sauvage (*Zero-Allocation*), développée selon la norme stricte **C17** sous licence **LGPL 3**. Sa mission est de fournir un socle matériel déterministe, robuste et multiplateforme (`Windows x86_64` / `Linux x86_64`) pensé pour le *Data-Oriented Design* (DOD) et les applications graphiques interactives temps réel intensives.

L'architecture applique un principe d'isolation totale vis-à-vis du système d'exploitation et des bibliothèques tierces via l'usage exclusif de pointeurs et types opaques (`VoidArena`, `VoidWindow`, `VoidTexture`, `VoidSound`, `VoidMusic`). Les appels système directs (`VirtualAlloc`/`mmap`, `CreateFile`/`open`, `BCryptGenRandom`/`getrandom`) et les back-ends multimédias sous-jacents sont rigoureusement encapsulés derrière une C-API minimaliste.

---

## 2. Caractéristiques Fondamentales

- **Arènes Mémoire Hiérarchiques et Scopées :** Une réservation unique de mémoire virtuelle auprès de l'OS (`ROOT_MASTER`) subdivisée dynamiquement en sous-arènes nommées et protégées par spinlock (`LEVEL`, `FRAME`). Supporte les cycles de vie scopés en $O(1)$ (`VoidScope`), la réallocation sur place au sommet de la pile et la promotion de données par coupé-collé inter-arènes (`void_arena_memmove`).
- **Géométrie 2D à Empreinte Nulle (`gfx.c`) :** Tessellation matérielle pour polygones concaves (*Ear-Clipping*), cercles adaptatifs, ellipses, anneaux, capsules, secteurs circulaires, rectangles arrondis, lignes épaisses et courbes de Bézier quadratiques. Chaque appel encapsule ses sommets temporaires dans un `VoidScope`, garantissant une **empreinte mémoire nette de 0 octet**.
- **Entrées Logiques & Manettes Hot-Plug :** Lecture immédiate et détection de front (`pressed`, `just_pressed`, `just_released`) pour le clavier, la souris (convertie automatiquement dans la résolution logique virtuelle via `SDL_RenderWindowToLogical`), la molette et jusqu'à 4 manettes branchables à chaud avec zone morte radiale et vibrations haptiques (*rumble*).
- **Mixeur Audio Spatialisé 2D :** Pipeline audio multi-codecs (`WAV`, `FLAC`, `MOD/XMP`, `MP3`, `OGG`) à 32 canaux matériels permettant le contrôle dynamique du volume, du panoramique stéréo sur 360° et de l'atténuation de distance en temps réel.
- **Concurrence Lock-Free & Services OS :** Opérations atomiques (`void_atomic_increment`, `void_atomic_compare`), instruction matérielle de relâchement CPU (`void_processor_pause`), journalisation asynchrone sans verrou (`DEET`), E/S fichiers natives avec lecture d'horodatage (`mtime`) pour le rechargement dynamique de modules, et génération de graines d'entropie matérielle 64 bits (`void_get_random_seed`).

---

## 3. Architecture de la Mémoire (`memory.c`)

VOID élimine tout appel à `malloc`/`free` durant l'exécution au profit d'arènes linéaires déterministes :

1. **Initialisation Maîtresse :** `void_memory_init(total_capacity)` réserve un bloc contigu de mémoire virtuelle auprès du noyau OS (`ROOT_MASTER`).
2. **Découpage en Sous-Arènes :** `void_arena_suballoc(parent, name, capacity)` découpe des arènes dédiées avec protection multithread par spinlocks (`LEVEL`, `FRAME`).
3. **Cycles de vie scopés :**
   ```c
   VoidScope scope = void_arena_push(arena);
   // ... Allocations temporaires ultra-rapides ...
   void_arena_pop(arena, scope); // Restauration instantanée en O(1) de l'offset initial
   ```
4. **Promotion & Manipulation de Données :**
- `void_arena_realloc` : Redimensionne un bloc sur place s'il se situe au sommet de l'arène.
- `void_arena_memmove` : Déplace une donnée temporaire d'une arène de travail (`FRAME`) vers une arène persistante (`LEVEL`) tout en récupérant immédiatement l'espace source.
- `void_arena_sprintf` / `void_arena_strdup` / `void_arena_strndup` : Formatage et duplication de chaînes sans fragmentation.
5. **Télémétrie Temps Réel :** `void_memory_monitor(interval_ms)` affiche périodiquement des jauges visuelles d'occupation formatées selon la norme IEC base 2 (`B`, `KiB`, `MiB`, `GiB`).

---

## 4. Spécifications des Sous-Systèmes

| Module | Fonctionnalités Clés |
| :--- | :--- |
| **Système & Entropie (`core.c`, `time.c`)** | Initialisation des sous-systèmes, détection des cœurs logiques (`void_get_core_count`), entropie noyau 64 bits (`void_get_random_seed`), temps monotone et delta-time borné. |
| **Fenêtrage & Rendu 2D (`window.c`, `render.c`)** | Support HighDPI, mise à l'échelle logique, cibles de rendu hors-écran FBO (`VoidTexture`), primitives avec fusion Alpha et quads texturés à coordonnées UV libres. |
| **Géométrie Avancée (`gfx.c`)** | Triangulation à empreinte mémoire nulle pour polygones concaves, anneaux, capsules, secteurs, rectangles arrondis, ellipses et courbes de Bézier. |
| **Périphériques (`input.c`)** | Clavier avec détection de front, souris en coordonnées logiques + molette, et jusqu'à 4 manettes à branchement à chaud avec zone morte logicielle et vibrations. |
| **Audio Spatialisé (`audio.c`)** | Mixeur 32 canaux avec suivi d'identifiant de canal, volume individuel et spatialisation 2D (`void_audio_set_channel_position`). |
| **Fichiers & Dynamic Loader (`file.c`, `library.c`)** | Appels systèmes natifs (lecture, écriture, copie, horodatage `mtime`) et chargement dynamique de symboles (`.dll` / `.so`) à l'exécution. |
| **Threads & Logs (`thread.c`, `log.c`)** | Threads de travail détachés, primitives atomiques CAS, pause processeur et logger lock-free sur tampon circulaire. |

---

## 5. Compilation et Déploiement

Le système de build (`Makefile` & `common.mk`) automatise la compilation native et croisée en intégrant les bibliothèques compilées sur mesure (`SDL2`, `SDL2_image`, `SDL2_mixer`, `ogg`, `xmp`, `webp`, `tiff`) vers les répertoires cibles (`bin/linux-x86_64`, `bin/windows-x86_64`).

```bash
# Compiler la bibliothèque partagée et la suite de tests
make all

# Compiler uniquement la bibliothèque partagée (libVOID.so / VOID.dll)
make lib

# Lancer la suite de tests interactive (arguments : system, memory, file, thread, render, all)
make run
```

---

## 6. Feuille de Route

- **Phase 1 : Fondations PAL** [Terminé]
  - Architecture du micro-noyau C17, réservation contiguë initiale de mémoire virtuelle et synchronisation thread-safe.
  - Abstraction de base du fenêtrage, primitives de dessin 2D accélérées et gestion des textures.
  - Logger lock-free asynchrone sur tampon circulaire avec niveaux de gravité colorisés et télémétrie `DEET`.
- **Phase 2 : Évolution du Back-end & Zéro-Allocation** [Actuel]
  - *Arènes Scopées & Hiérarchie :* Gestion de sous-arènes contiguës (`LEVEL`, `FRAME`), cycles de vie scopés en $O(1)$ (`VoidScope`), promotion de données inter-arènes (`void_arena_memmove`) et réallocation sur place.
  - *Géométrie Vectorielle 2D :* Moteur de tessellation matérielle à empreinte mémoire nette nulle (`gfx.c`) : triangulation de polygones concaves (*Ear Clipping*), cercles parfaits, ellipses, anneaux, capsules, secteurs circulaires et courbes de Bézier.
  - *Périphériques & Entrées :* Conversion de coordonnées physiques vers résolution virtuelle logique, détection d'états et de fronts (`pressed`, `just_pressed`, `just_released`), support à chaud de 4 manettes avec zone morte radiale et retour haptique (*rumble*).
  - *Mixeur Audio Spatialisé :* Suivi individuel de 32 canaux matériels, volume par canal, panoramique stéréo temps réel sur 360° et atténuation linéaire selon la distance.
  - *Services OS Natifs :* E/S fichiers bas niveau (lecture, écriture, copie, timestamp `mtime` pour le rechargement dynamique), chargeur de bibliothèques partagées (`library.c`) et extraction de graines d'entropie matérielle 64 bits (`void_get_random_seed`).
- **Phase 3 : Architecture Modulaire `*.void`** [Prochaine étape]
  - Scission du runtime en micro-noyau `void-core` et modules dynamiques interchangeables rechargeables à chaud (`gfx.void`, `audio.void`, `input.void`, `net.void`).
  - Protocole binaire d'échange et d'interconnexion de modules à chaud sans interruption de la boucle d'événements.
- **Phase 4 : Format de Conteneurs Binaires & Streaming d'Assets** [À venir]
  - Spécification d'un conteneur mémoire structuré (`assets.void`) garantissant une injection directe VRAM/Audio sans étape de décompression intermédiaire.
  - Pipeline de lecture séquentielle alignée par projection mémoire (*Memory Mapping*).
- **Phase 5 : Pipeline Graphique Moderne & Abstraction 3D** [Long terme]
  - Migration vers un contexte OpenGL natif autonome (GLFW, GLAD, VBO/VAO, shaders GLSL programmables).
  - Pipeline 3D hybride : rendu de *billboards* avec projection de textures, *Normal Mapping* et shaders de post-traitement (*Cel-Shading*, pixellisation dynamique).
  - Abstraction complète du sous-système de rendu pour s'affranchir de toute dépendance tierce résiduelle, et portabilité vers l'architecture processeur ARM64 et Vulkan.

---

## 7. Licence

Le framework VOID est distribué sous licence **LGPL 3**.