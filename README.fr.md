# VOID Framework (v0.1.0-alpha)

*Lire dans d'autres langues : [English](README.md)*

## 1. Introduction et Vision du Projet

VOID est une couche d'abstraction de plateforme (PAL) haute performance développée selon la norme C17. Sa mission est de fournir un socle architectural robuste et portable pour le développement de moteurs de jeu, à l'instar du moteur SOUL. L'architecture impose une isolation totale du système via l'usage exclusif de pointeurs opaques — notamment `VoidWindow`, `VoidTexture` et `VoidRender` — masquant ainsi l'implémentation SDL2 vis-à-vis du Front-end.

> **Note opérationnelle :** VOID expose une C-API minimaliste conçue pour être consommée efficacement par des moteurs C++ de haut niveau, garantissant une séparation stricte entre la logique applicative et les services système.

## 2. Caractéristiques Fondamentales

* **Gestion Mémoire par Arènes :** Implémentation d'un paradigme de "Zéro-Allocation" durant la boucle de jeu. Le système fragmente la mémoire pré-allouée entre une Arène Globale (données persistantes) et une Arène de Frame (données temporaires).
* **Primitives Lock-Free :** Utilisation intensive des opérations atomiques du standard C pour la gestion des files de tâches et la synchronisation, éliminant les goulots d'étranglement et les changements de contexte liés aux mutex.
* **Distribution Dynamique :** Support natif multiplateforme via la génération de bibliothèques partagées (`.dll` pour Windows, `.so` pour Linux), avec une gestion automatisée des dépendances vers les répertoires binaires spécifiques à l'architecture.
* **Architecture PAL :** Abstraction exhaustive des services de bas niveau incluant la gestion des fenêtres, le rendu graphique 2D, la gestion du temps haute résolution et le traitement immédiat des entrées utilisateur.

## 3. Architecture de la Mémoire (Zéro-Allocation)

Le système de gestion mémoire, défini dans `void.h`, est optimisé pour le Data-Oriented Design (DOD). L'usage systématique de l'alignement (`alignof`) et d'arènes contiguës favorise la localité du cache et les structures de données de type SoA (Structure of Arrays). L'initialisation doit respecter l'ordre opérationnel : `void_init()` suivi de `void_memory_init()`.

* **`void_arena_alloc(uint64 size, uint32 alignment)`**
  Cette fonction alloue des blocs persistants dans l'arène globale. L'alignement manuel garantit l'intégrité des données sur les architectures strictes et optimise les performances de lecture.

* **`void_frame_alloc(uint64 size, uint32 alignment)`**
  Allocation ultra-rapide dans l'arène de frame, représentant 10% de la mémoire totale (Facteur 0.1). Cette zone est libérée instantanément à chaque fin de cycle via `void_frame_free()`, sans aucun coût processeur.

## 4. Système de Tâches et Multithreading

VOID implémente un *job system* de type "Fibers-preparatory" conçu pour minimiser l'intervention de l'ordonnanceur de l'OS. Le système instancie un pool de worker threads basé sur le nombre de cœurs logiques détectés via `void_system_get_core_count()`.

La synchronisation et la distribution des tâches s'effectuent sans verrouillage grâce aux primitives atomiques `void_atomic_increment`, `void_atomic_decrement` et `void_atomic_compare_exchange`. Cette approche garantit une exécution fluide des systèmes massivement parallèles comme la physique ou l'intégration spatiale de l'ECS.

## 5. Spécifications du Rendu et de l'Input

| Fonctionnalité | Implémentation |
| :--- | :--- |
| **Primitives 2D** | Points, lignes et rectangles pilotés par le backend `VoidRender`. |
| **Gestion des Textures** | Handles opaques via `VoidTexture` avec chargement via `void_texture_load`. |
| **Entrées (Input)** | Lecture immédiate de l'état ; `VoidKeyCode` mappé directement sur `SDL_SCANCODE`. |
| **Rendu Backend** | Implémentation actuelle via `SDL_Renderer` avec support natif HighDPI. |

## 6. Roadmap de Développement

1. **Rendu :** Migration du pipeline actuel vers une API de rendu moderne basée sur OpenGL pour un contrôle accru du GPU.
2. **Branches Legacy :** Maintenance et stabilisation du support pour SDL2 et l'architecture processeur x86.
3. **Branches Enhanced :** Optimisations spécifiques pour l'environnement UCRT et extension de la compatibilité vers l'architecture ARM64.

## 7. Compilation et Déploiement

Le build système s'appuie sur `common.mk` pour automatiser la compilation croisée. Le processus génère les binaires et les bibliothèques dynamiques (`libVOID.dll` / `libVOID.so`) dans des sous-répertoires structurés selon la cible, par exemple `bin/windows-x86_64` ou `bin/linux-x86_64`.

Pour compiler l'intégralité du framework et de sa bibliothèque :
```bash
make all
```

Pour générer uniquement la bibliothèque partagée :
```bash
make lib
```

## 8. Licence

Le framework VOID est distribué sous licence **LGPL 3**.