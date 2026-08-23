# VOIDLOCK

VOIDLOCK is a tiny, original retro first-person shooter and a learning-oriented custom game engine. Built from scratch in modern **C++20**, it features a textured CPU-based raycaster for its 3D environment, uploads a sharp 640×360 software framebuffer to OpenGL for rendering, and relies on **SDL3** to handle the platform windowing, input, and game loop execution.

All visual and logic assets in the game are either procedurally generated at runtime or loaded from custom art sheets. It does not use assets from commercial games.

---

## Table of Contents

1. [Overview & Core Architecture](#overview--core-architecture)
2. [The Gameplay Mission](#the-gameplay-mission)
3. [Building & Running the Game](#building--running-the-game)
4. [Controls & Input System](#controls--input-system)
5. [Codebase Directory Structure](#codebase-directory-structure)
6. [Detailed Module & Code Walkthrough](#detailed-module--code-walkthrough)
   - [Game Manager & Main Loop (`core`)](#game-manager--main-loop-core)
   - [Level & Collision Engine (`world`)](#level--collision-engine-world)
   - [Raycasting & CPU Render Pipeline (`renderer`)](#raycasting--cpu-render-pipeline-renderer)
   - [Procedural Texture Generator (`TextureAtlas`)](#procedural-texture-generator-textureatlas)
   - [Materials & Ambient Lighting (`Materials`)](#materials--ambient-lighting-materials)
   - [Entity State Machine (`entities`)](#entity-state-machine-entities)
   - [Weapon System & Shotgun Raycast Spread (`weapons`)](#weapon-system--shotgun-raycast-spread-weapons)
   - [Animated HUD Portrait System (`ui`)](#animated-hud-portrait-system-ui)
   - [Input Layer (`input`)](#input-layer-input)
   - [Audio Pipeline (`audio`)](#audio-pipeline-audio)
7. [Developer & Debugging Tools](#developer--debugging-tools)
8. [Next Steps for Project Expansion](#next-steps-for-project-expansion)

---

## Overview & Core Architecture

VOIDLOCK acts as a bridge between classic 90s shooters and modern C++ engineering. Instead of using contemporary GPU pipelines for 3D projection, the engine runs a **CPU raycaster** to draw walls, floors, ceilings, sprites, and particle effects.

```
                  +-----------------------------------------+
                  |               main.cpp                  |
                  +-------------------+---------------------+
                                      |
                                      v
                  +-------------------+---------------------+
                  |               Game Class                |
                  |     (Core Event Loop & Updates)          |
                  +-------+-----------+-----------+---------+
                          |           |           |
        +-----------------+           |           +-----------------+
        v                             v                             v
+-------+-------+             +-------+-------+             +-------+-------+
|  Input Class  |             |  Level Class  |             |  Audio Class  |
| (SDL3 Events) |             | (ASCII Grid)  |             | (Sound Hooks) |
+---------------+             +---------------+             +---------------+
        |                             |                             |
        |                             v                             |
        |                     +-------+-------+                     |
        |                     | Player Class  |                     |
        |                     +-------+-------+                     |
        |                             |                             |
        v                             v                             v
+-------+-----------------------------+-----------------------------+-------+
|                              Entities & Weapons                           |
|                  (Rusher, Gunner, Brute, Projectiles, Shotgun)             |
+-------------------------------------+-------------------------------------+
                                      |
                                      v
                  +-------------------+---------------------+
                  |            Renderer Engine              |
                  |  - DDA Wall Raycaster (CPU)             |
                  |  - Floor/Ceiling Plane projection       |
                  |  - Depth Buffer Sprite Painter          |
                  |  - Procedural Texture Samplers          |
                  +-------------------+---------------------+
                                      |
                                      v
                  +-------------------+---------------------+
                  |        OpenGL Presentation Quad         |
                  |     (Hardware Integer-Upscaled)         |
                  +-----------------------------------------+
```

### Core Architecture Highlights:
*   **Modern C++20 Standard:** Extensively utilizes features like type-safe standard library algorithms, standard layouts, `std::erase_if`, and structure initializations.
*   **Textured DDA Raycaster:** Uses Grid Digital Differential Analysis (DDA) to cast camera rays, ensuring perfect floating-point grid intersection checks.
*   **Floor & Ceiling Projection:** Processes horizontal rows using perspective-correct texture mapping relative to the player's position and view plane.
*   **Chroma-Key Sprite Loader:** The engine parses PNG images using SDL3, dynamically checks the upper-left pixel for a key color (flat green), and generates an alpha mask automatically.
*   **Retro Resolution with Smart Scaling:** Renders internally to a `640×360` 32-bit pixel buffer. This buffer is uploaded as a single OpenGL 2.1 2D texture mapped onto a viewport quad, utilizing integer-scaling where possible to preserve pixel art definition.

---

## The Gameplay Mission

The mission of VOIDLOCK is simple but intense:
1.  **Spawn:** The player begins in a concrete industrial cell block (the starting room).
2.  **Explore:** Search the lower storage corridors to find the **Cyan Keycard**.
3.  **Unlock:** Open the security gate marked with yellow/black warnings and a yellow door light indicator.
4.  **Engage:** Clear the final room containing the high-hazard core area and defeat the security robots blocking the exit.
5.  **Escape:** Once all enemies in the final room are terminated, the **Cyan Exit Tile** is activated. Stepping on this tile completes the level.

A full run takes approximately **2 to 5 minutes**.

---

## Building & Running the Game

### Dependencies
Before building, ensure you have the following installed on your system:
*   A **C++20** compatible compiler (GCC 11+, Clang 13+, or MSVC 2022+).
*   **CMake 3.24** or newer.
*   **OpenGL** development libraries (GL, GLU, or OpenGL.framework on macOS).
*   **SDL3** (development headers and binaries). 

> [!NOTE]
> By default, CMake is configured to automatically download and build **SDL 3.2.20** from source (`VOIDLOCK_FETCH_SDL=ON`). This provides a seamless build experience even on systems without SDL3 pre-installed.

### Compile Commands

To build and run the game, execute the following commands in your terminal:

```sh
# Generate build configuration
cmake -S . -B build

# Build target executable
cmake --build build -j

# Launch the game
./build/voidlock
```

*For offline or system-only packages (e.g. system-wide SDL3 installed via homebrew/apt):*
```sh
cmake -S . -B build -DVOIDLOCK_FETCH_SDL=OFF
cmake --build build -j
```

### Auto-Formatting Code
The codebase enforces consistent styling rules via `ClangFormat`. To auto-format all code files in the directory:
```sh
cmake --build build --target format
```
To check formatting without modifying files (e.g., in a CI pipeline):
```sh
cmake --build build --target format-check
```

---

## Controls & Input System

The game integrates traditional keyboard and mouse controls:

| Input | Action |
|---|---|
| **W** / **S** | Move Forward / Backward |
| **A** / **D** | Strafe Left / Right |
| **Mouse (Horizontal)** | Look Left / Right (Camera Rotation) |
| **Left Shift (Hold)** | Run (Increases movement speed) |
| **Space** / **Left Mouse Click** | Fire RIVET-12 Shotgun |
| **E** | Interact (Open doors) |
| **Alt + Enter** | Toggle Fullscreen |
| **F1** | Toggle Enemy Developer Debug Info overlay |
| **Escape** | Close window and Exit game |

*   **Canisters & Pickups:** Activated automatically on collision. 
    *   **Green Canister (+35 Health):** Heals the player up to a maximum of 100.
    *   **Amber Box (+8 Shells):** Refills shotgun ammo.
    *   **Cyan Device:** Unlocks the security door.

---

## Codebase Directory Structure

```text
├── CMakeLists.txt              # Build configuration with automated SDL3 fetching
├── .clang-format               # Style rules mapping (LLVM base, 4-space indent)
├── scripts/
│   └── format.sh               # Shell utility wrapping clang-format queries
├── assets/
│   ├── maps/                   # Map placeholder folder (.gitkeep)
│   ├── textures/               # Ambient texture hooks (.gitkeep)
│   ├── sounds/                 # Sound placeholder directory (.gitkeep)
│   └── sprites/
│       ├── ui/
│       │   └── status_face_atlas.png   # HUD portrait sheet (10x4 cells)
│       ├── weapons/
│       │   └── rivet12_sheet_source.png # Weapon sprite sheet (6 frames)
│       └── enemies/
│           ├── rusher_atlas.png  # Rusher model poses (8 directions x 7 animations)
│           ├── gunner_atlas.png  # Gunner model poses (8 directions x 7 animations)
│           └── brute_atlas.png   # Brute model poses (8 directions x 7 animations)
└── src/
    ├── main.cpp                # App initialization and main entry point
    ├── core/
    │   ├── Game.hpp            # Main loop configuration and delta time tracking
    │   └── Game.cpp            # Updates player inputs, physics ticks, state steps
    ├── game/
    │   └── Types.hpp           # Custom vectors (Vec2), length, norm, wrapAngle
    ├── input/
    │   ├── Input.hpp           # Keyboard keys buffer and relative mouse capture
    │   └── Input.cpp           # Pulls raw SDL3 window inputs
    ├── world/
    │   ├── Level.hpp           # Level grid definitions and pickups
    │   └── Level.cpp           # Hardcoded ASCII Map grid and spawns
    ├── entities/
    │   ├── Player.hpp          # Player collision, health, recoil calculations
    │   ├── Player.cpp          # Slide-along-wall calculations
    │   ├── Enemy.hpp           # AI configuration and directional calculations
    │   └── Enemy.cpp           # Rusher, Gunner, Brute behaviors & state updates
    ├── weapons/
    │   ├── Shotgun.hpp         # Weapon state variables
    │   └── Shotgun.cpp         # 11-pellet spread hitscan raycast checks
    ├── ui/
    │   ├── HudFace.hpp         # Face emotion mappings
    │   └── HudFace.cpp         # Custom priorities for pain, firing, and kill animations
    ├── renderer/
    │   ├── Renderer.hpp        # Framebuffer size settings and camera planes
    │   ├── Renderer.cpp        # The core CPU raycaster and sprite projection
    │   ├── TextureAtlas.hpp    # 32x32 texture enumerator
    │   ├── TextureAtlas.cpp    # Procedural texture generator using bitwise noise
    │   ├── Materials.hpp       # Wall/Floor/Ceiling material definitions
    │   ├── Materials.cpp       # Animated computer panels and tile properties
    │   ├── SpriteSheet.hpp     # Raw PNG parser
    │   └── SpriteSheet.cpp     # Sprite scaler and chroma-key filter
    └── audio/
        ├── Audio.hpp           # Audio callback structure
        └── Audio.cpp           # Hooks for sound effects
```

---

## Detailed Module & Code Walkthrough

### Game Manager & Main Loop (`core`)
Managed in [Game.hpp](file:///Users/abc/Desktop/DOOM/src/core/Game.hpp) and [Game.cpp](file:///Users/abc/Desktop/DOOM/src/core/Game.cpp), the `Game` class handles the main game loop:
*   **Ticks:** Runs a high-precision clock using `SDL_GetTicksNS()`, calculating a frame step delta `dt` capped at a maximum of `0.05` seconds to prevent frame jumps during lag.
*   **Updates:** Steps player status, processes shotgun triggers, runs individual enemy updates, and processes projectile dynamics.
*   **Respawn Mechanics:** If the player's health drops to 0, a death phase triggers for 1.2 seconds, after which health is reset to 100, inventory is restored, and the player is teleported back to the spawn coordinates `{2.5f, 2.5f}`.

### Level & Collision Engine (`world`)
Managed in [Level.hpp](file:///Users/abc/Desktop/DOOM/src/world/Level.hpp) and [Level.cpp](file:///Users/abc/Desktop/DOOM/src/world/Level.cpp), the map is defined as an ASCII layout:

```cpp
grid_ = {"#########################",
         "#....#........#.........#",
         "#....#........#.........#",
         "#....#........#.........#",
         "#....####.#####.........#",
         "#.......................#",
         "#....####.######D########",
         "#....#.........#........#",
         "######.........#........#",
         "#..............#........#",
         "#..............#........#",
         "#..............#........#",
         "#..............#........#",
         "#..............#......X.#",
         "#########################"};
```

*   `#` represent **Solid Walls**.
*   `D` represents the **Locked Security Door** (requires the Cyan Key).
*   `X` represents the **Cyan Exit Tile** (triggers completion if final room enemies are cleared).
*   `.` represents traversable space.
*   **Collisions:** Solid tiles block entity bounding circles. Radius properties prevent characters from stepping into walls.
*   **Line-of-Sight Check (`lineClear`):** Steps coordinates along a vector connecting points `a` and `b`. If any step falls inside a solid wall (`#` or locked `D`), it returns `false`. This logic controls enemy detection and projectile hit checks.

### Raycasting & CPU Render Pipeline (`renderer`)
Implemented in [Renderer.hpp](file:///Users/abc/Desktop/DOOM/src/renderer/Renderer.hpp) and [Renderer.cpp](file:///Users/abc/Desktop/DOOM/src/renderer/Renderer.cpp).
For every frame:
1.  **Clear Screen:** The ceiling is filled with dark blue (`0xff101724u`). The rest of the screen is drawn dynamically.
2.  **Floor & Ceiling Raycaster (`drawSurfaces`):** Renders the horizontal perspective plane. Ray coordinates project from the camera viewpoint to locate floor/ceiling texture pixels based on height.
3.  **Wall Raycaster:** Fires `640` vertical columns across the camera's FOV (60 degrees).
    *   Uses **DDA (Digital Differential Analysis)** to step through grid cells along the ray direction.
    *   Tracks whether a horizontal or vertical grid line was hit first to adjust lighting intensity (`0.78` shading multiplier for y-facing walls to simulate light shadows).
    *   Calculates wall heights to draw columns:
        $$\text{WallHeight} = \frac{\text{Viewport Height}}{\text{Ray Distance}}$$
    *   Saves distance checks to a depth buffer array (`depth_`) to handle occlusion sorting when drawing sprites.
4.  **Sprite Painter (Depth-Sorted Rendering):** Combines enemies and active level pickups into a single list, sorts them from furthest to closest (using the Painter's Algorithm), and renders them:
    *   Occludes sprite pixels that sit behind the wall depth recorded in `depth_`.
    *   Translates relative positions into screen column offsets based on horizontal viewing angles.
5.  **HUD Status Drawing:** Renders the status bar panel overlay, status labels (Health, Ammo, and Key status), and draws the animated HUD face.

### Procedural Texture Generator (`TextureAtlas`)
Defined in [TextureAtlas.hpp](file:///Users/abc/Desktop/DOOM/src/renderer/TextureAtlas.hpp) and [TextureAtlas.cpp](file:///Users/abc/Desktop/DOOM/src/renderer/TextureAtlas.cpp).
Rather than loading image assets from disk for the environment, the engine procedurally generates all wall, floor, and ceiling textures at startup:
*   Uses a fast, deterministic bitwise pseudo-random noise algorithm:
    ```cpp
    int noise(int x, int y, int seed) {
        std::uint32_t n = static_cast<std::uint32_t>(x * 1973 + y * 9277 + seed * 26699) | 1u;
        n = (n << 13u) ^ n;
        return static_cast<int>((n * (n * n * 15731u + 789221u) + 1376312589u) >> 27u) & 31;
    }
    ```
*   Draws specific patterns based on texture ID:
    *   **Concrete:** Alternates grey colors with dark borders to create concrete blocks.
    *   **Warning Panel:** Alternates yellow (`0xffc69123u`) and black stripes.
    *   **Computer Screen:** Draws borders around a cyan screen area, overlaying scanlines.
    *   **Hazard Floor:** Alternates gold and dark hazard stripes.

### Materials & Ambient Lighting (`Materials`)
Defined in [Materials.hpp](file:///Users/abc/Desktop/DOOM/src/renderer/Materials.hpp) and [Materials.cpp](file:///Users/abc/Desktop/DOOM/src/renderer/Materials.cpp).
Translates map grid positions and types into `Material` attributes:
*   **Door:** Assigned the Security Door texture.
*   **Computer Terminals:** Positioned at repeating grid intervals. Displays a pulsating screen color using a sine function of the current game clock:
    ```cpp
    float pulse = 0.35f + 0.15f * std::sin(time * 3.0f + static_cast<float>(x));
    ```
*   **Floor Zones:** Automatically maps starting rooms to steel plates, corridors to concrete panels, and exit areas to hazard tiles.

### Entity State Machine (`entities`)
Entities are managed in [Enemy.hpp](file:///Users/abc/Desktop/DOOM/src/entities/Enemy.hpp) and [Enemy.cpp](file:///Users/abc/Desktop/DOOM/src/entities/Enemy.cpp):

```
+------------+      Distance Check       +-------------+
|    IDLE    | ------------------------> |    ALERT    |
+------------+                           +-------------+
      ^                                         |
      | Respawn                                 | Animation Timer Ended
      |                                         v
+------------+      Within Range         +-------------+
|    DEAD    | <------------------------ |    CHASE    |
+------------+                           +-------------+
      ^                                     |       ^
      | Health <= 0                         |       | Cooldown
      |                                     v       | Ended
+------------+      Attack Cooldown      +-------------+
|   DYING    | <------------------------ |   ATTACK    |
+------------+                           +-------------+
```

Each enemy type implements unique combat profiles:
1.  **Rusher:** Fast pursuit AI ($Speed = 2.15$, $Health = 36$). Runs toward the player using a weaving side-to-side movement pattern. Performs a physical melee bite.
2.  **Gunner:** Ranged tactical AI ($Speed = 0.82$, $Health = 72$). Retreats if the player get too close, maintaining a distance of ~4.7 units. Fires 3-shot hitscan bursts before strafing.
3.  **Brute:** Heavy projectile AI ($Speed = 0.47$, $Health = 190$). Launches slow-moving, explosive projectile spheres at range. Performs a high-damage slam attack if the player enters melee range.

#### Directional Sprite Selection
To simulate 3D models using 2D billboards, the engine calculates the difference between the enemy's facing direction and the angle toward the player's camera:
```cpp
const float view = std::atan2(player.pos.y - pos.y, player.pos.x - pos.x);
const float face = std::atan2(facing.y, facing.x);
float relative = wrapAngle(view - face);
int direction = static_cast<int>(std::floor((relative + PI / 8.f) / (PI / 4.f)));
return (direction % 8 + 8) % 8;
```
This formula returns an index from `0` to `7`, mapping to the correct column of the 8-directional sprite sheet:
*   `0` = Front, `2` = Left, `4` = Back, `6` = Right, etc.

### Weapon System & Shotgun Raycast Spread (`weapons`)
Implemented in [Shotgun.hpp](file:///Users/abc/Desktop/DOOM/src/weapons/Shotgun.hpp) and [Shotgun.cpp](file:///Users/abc/Desktop/DOOM/src/weapons/Shotgun.cpp).
The RIVET-12 scattergun operates on a frame-based state machine: `Idle` $\rightarrow$ `Walk` $\rightarrow$ `Run` $\rightarrow$ `Fire` $\rightarrow$ `Recoil` $\rightarrow$ `Recover` $\rightarrow$ `Reload`.

*   **11-Pellet Hitscan Raycast:** When fired, the shotgun fires 11 separate rays. Each ray is offset using a spread angle:
    ```cpp
    const float spread = (pellet - 5) * PelletSpread / 5.f + std::sin(shotSeed_ * 12.989f + pellet * 4.17f) * 0.012f;
    ```
*   **Collision Detection:** The engine traces along each ray up to a maximum distance of 12 units.
    *   If a ray intersects an enemy's bounding cylinder, the enemy takes damage, and a blood splash particle effect is spawned.
    *   If the ray hits a solid wall first, it spawns yellow metal sparks and leaves a dark impact burn on the surface.
*   **Feedback Effects:** Adds camera kick (`viewKick_`) and screen shake (`screenShake_`), ejects a physical brass shotgun shell model that bounces according to gravity formulas, and projects a dynamic light source onto the screen coordinate buffer.

### Animated HUD Portrait System (`ui`)
Implemented in [HudFace.hpp](file:///Users/abc/Desktop/DOOM/src/ui/HudFace.hpp) and [HudFace.cpp](file:///Users/abc/Desktop/DOOM/src/ui/HudFace.cpp).
The status bar features an animated face portrait that changes state based on game events:
*   **Idle Animations:** Automatically alternates looking directions and blinks periodically.
*   **Movement Reaction:** Looks left or right based on strafing input.
*   **High-Priority Reactions:** Firing the shotgun displays a combat grimace. Inflicting damage on an enemy triggers a brief grin.
*   **Directional Pain:** When taking damage, the engine calculates the incoming vector relative to the player's view direction. The face turns and displays pain toward the source (front, left, or right):
    ```cpp
    const FaceState pain = player.damageSide() < -.25f  ? FaceState::PainLeft
                           : player.damageSide() > .25f ? FaceState::PainRight
                                                        : FaceState::PainFront;
    ```
*   **Health Rows:** The sprite sheet organizes portrait states across 4 rows corresponding to health levels:
    *   `Row 0:` Healthy ($>75\%$ Health)
    *   `Row 1:` Scratched ($50\% - 75\%$ Health)
    *   `Row 2:` Damaged ($25\% - 50\%$ Health)
    *   `Row 3:` Critical ($<25\%$ Health) or Dead.

### Input Layer (`input`)
Defined in [Input.hpp](file:///Users/abc/Desktop/DOOM/src/input/Input.hpp) and [Input.cpp](file:///Users/abc/Desktop/DOOM/src/input/Input.cpp).
*   Manages window state through SDL event polling.
*   Enables **Relative Mouse Mode** via `SDL_SetWindowRelativeMouseMode(window, true)`. This locks the cursor inside the screen area and tracks raw relative movements, providing responsive mouse-look controls.
*   Maintains separate key state buffers for the current and previous frames. This allows checking if a button was pressed during the current frame (for one-off triggers like Alt+Enter or F1) rather than just being held down.

### Audio Pipeline (`audio`)
Defined in [Audio.hpp](file:///Users/abc/Desktop/DOOM/src/audio/Audio.hpp) and [Audio.cpp](file:///Users/abc/Desktop/DOOM/src/audio/Audio.cpp).
*   Contains initialization routines and hooks for key game events (`playShot`, `playPickup`, `playEnemyAlert`, `playEnemyAttack`, and `playEnemyCharge`).
*   Intentionally runs silently by default. To add sound, WAV files can be loaded and played through SDL3's audio stream utilities.

---

## Developer & Debugging Tools

VOIDLOCK includes several built-in environment variables and keyboard hooks to assist developers during testing:

1.  **F1 Debug Mode:** While playing, press **F1** to overlay developer information over each enemy sprite. This displays the enemy's AI state, health, distance from the camera, and active animation frame.
2.  **Forced Spawning (`VOIDLOCK_CAPTURE_ENEMY`):** Force the engine to spawn only a single enemy type at fixed coordinates to test behavior:
    ```sh
    # Options: RUSHER, GUNNER, BRUTE
    VOIDLOCK_CAPTURE_ENEMY=BRUTE ./build/voidlock
    ```
3.  **Hurt State Capture (`VOIDLOCK_CAPTURE_FACE`):** Force the player's face HUD portrait to render in a specific state for testing:
    ```sh
    # Options: CRITICAL, PAIN_LEFT, DEAD, KILL
    VOIDLOCK_CAPTURE_FACE=CRITICAL ./build/voidlock
    ```
4.  **Save Gameplay Snapshot (`VOIDLOCK_CAPTURE_FRAME`):** Captures the screen buffer on the 12th frame and writes it as a bitmap image:
    ```sh
    VOIDLOCK_CAPTURE_FRAME=screenshot.bmp ./build/voidlock
    ```
5.  **Simulate Immediate Shot (`VOIDLOCK_CAPTURE_FIRE`):** Triggers a weapon discharge immediately at startup.

---

## Next Steps for Project Expansion

VOIDLOCK is structured to serve as an extensible learning foundation. Recommended steps for expanding the project include:

1.  **External Map Loading:** Replace the hardcoded `Level::Level()` ASCII array with a text file loader to load layouts from external files.
2.  **Sound Asset Integration:** Bind WAV files in `Audio.cpp` utilizing the `SDL_LoadWAV` and `SDL_CreateAudioStream` functions.
3.  **Modern Renderer Presentation:** Upgrade the presentation viewport in `Renderer::present()` from legacy OpenGL fixed-function pipeline quads (`glBegin(GL_QUADS)`) to modern shaders.
4.  **Additional Weapon Options:** Expand the player inventory to support multiple weapons (e.g. rapid-fire plasma guns or melee punches) by adding states to the weapon machine.
5.  **A* Pathfinding:** Implement standard pathfinding models around solid grid blocks to allow enemies to navigate corners more effectively.
