# Space Defender Arcade

A 2D arcade space shooter built with **C++** and **OpenGL/GLUT**.

## What is implemented

This project includes:

- **Game states**: Menu, Playing, Paused, and Game Over
- **Player movement + combat**:
  - Move with `W A S D` or Arrow keys
  - Shoot with `Space` (and left mouse click)
- **Enemy system**:
  - Multiple enemy types (circle, triangle, square, diamond)
  - Progressive spawn rate and speed
- **Power-ups**:
  - Green life power-up with score bonus
- **Arcade feedback systems**:
  - Combo multiplier
  - Floating score text
  - Particle explosions
  - Screen shake + damage flash
  - Level-up banner
- **Progression and balancing**:
  - Dynamic difficulty up to Level 10
  - No-hit penalty logic
- **UI polish**:
  - Neon-style menu/HUD/game-over panels
  - Animated space background with stars and shooting-star effect
- **Persistence**:
  - Best score saved in `highscore.dat`

## Technical details (code implementation)

- **Language & graphics stack**:
  - C++ with OpenGL (immediate mode) and GLUT/FreeGLUT
  - 2D orthographic projection via `gluOrtho2D(0, WIDTH, 0, HEIGHT)`

- **Core architecture**:
  - Single-file game structure with modular draw/update/input functions
  - State machine using `GameState` enum: `MENU`, `PLAYING`, `PAUSED`, `GAME_OVER`
  - Timer-driven loop using `glutTimerFunc(16, update, 0)` for ~60 FPS updates

- **Data structures used**:
  - `struct Player`, `Bullet`, `Enemy`, `PowerUp`, `Particle`, `FloatingText`
  - Dynamic entity management with `std::vector` containers
  - Efficient cleanup of inactive entities via `std::remove_if` + `erase`

- **Rendering and classic algorithms**:
  - Custom primitive routines:
    - DDA line drawing (`drawLineDDA`)
    - Bresenham line drawing (`drawLineBresenham`)
    - Midpoint circle algorithm (`drawCircleMidpoint`)
    - Triangle-fan filled circle (`drawFilledCircle`)
  - Layered rendering order:
    - Animated background → gameplay entities → particles/text FX → HUD → overlays
  - Alpha blending enabled for glow, flash, and neon panel effects

- **Gameplay logic implemented**:
  - Keyboard and mouse input handling for movement, shooting, and state actions
  - Enemy and power-up spawning with timed intervals and randomized positions/types
  - Collision system using squared-distance checks (avoids expensive `sqrt`)
  - Type-based enemy collision radius via helper functions
  - Combo system with multiplier window and score scaling

- **Difficulty and progression systems**:
  - Progressive level system up to level 10
  - Dynamic enemy spawn rate and movement scaling by level multiplier
  - No-hit penalty timer (score penalty / life penalty flow)
  - Timed level-up banner and combat feedback effects

- **Arcade feedback effects in code**:
  - Particle explosions with velocity, gravity-like falloff, and lifespan
  - Floating score popups with fade and upward motion
  - Screen shake and damage flash for impact feedback
  - Animated stars, nebula-like background layers, and shooting-star/comet trail

- **Persistence and file I/O**:
  - High-score loading/saving through `std::ifstream` / `std::ofstream`
  - Runtime storage in `highscore.dat`

## Controls

- `W A S D` or Arrow keys: Move ship
- `Space`: Start game / Shoot / Redeploy from game over
- `Left Mouse Click`: Start game / Shoot
- `P`: Pause / Resume
- `Enter` (on game over): Return to menu
- `Esc`: Quit

## Project files (important)

- `space_defender.cpp` → Main complete game source
- `highscore.dat` → Auto-created at runtime to store best score

## How to run (Windows)

You need a C++ compiler and **FreeGLUT** installed.

### Option A: Code::Blocks (easy)

1. Install **Code::Blocks with MinGW**.
2. Install/copy **FreeGLUT** headers and libraries into your MinGW/Code::Blocks setup.
3. Open the project or create a new console/OpenGL project.
4. Add `space_defender.cpp`.
5. Link these libraries:
   - `freeglut`
   - `opengl32`
   - `glu32`
6. Build and run.

### Option B: Command line (MinGW g++)

From the folder containing `space_defender.cpp`, compile with:

```bash
g++ space_defender.cpp -o SpaceDefenderArcade.exe -lfreeglut -lopengl32 -lglu32
```

Then run:

```bash
./SpaceDefenderArcade.exe
```

> If your environment reports `cannot open source file GL/glut.h`, FreeGLUT include paths are not set yet.

## Notes

- The game uses immediate-mode OpenGL for educational/arcade-style rendering.
- Frame update uses GLUT timer callbacks (~60 FPS target).


