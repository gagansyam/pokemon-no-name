# Pokémon Battlegrounds: Real-Time Anime Battle Engine

A high-fidelity 2.5D real-time combat engine written in **C++20** and **SDL2**, inspired by anime tournament battles and competitive arena fighting games.

Featuring authentic 8-directional Mystery Dungeon pixel-art sprite animation, high-speed movement physics, directional aim, stamina pacing, 3D Z-axis elevation evasion, true hit-confirm combos, hitstop impact freezes, persistent ground decals, and responsive tactical AI.

---

## Playable Species & Roster

| Pokémon | Archetype | Signature Moves |
| :--- | :--- | :--- |
| **#0025 Pikachu** | High-Voltage Speedster | Quick Attack, Thunderbolt, Thunder, Iron Tail, Electro Ball, Volt Tackle, Agility, Double Team, Protect |
| **#0006 Charizard** | Aerial Heavy Juggernaut | Scratch, Flamethrower, Fire Spin, Wing Attack, Dragon Claw, Scorching Sands, Dragon Rage, Earthquake, Seismic Toss |
| **#0448 Lucario** | Aura Combo Striker | Force Palm, Aura Sphere, Close Combat, Extreme Speed, Bone Rush, Flash Cannon, Dragon Pulse, Protect |
| **#0658 Greninja** | Ninja Assassin | Water Shuriken, Night Slash, Hydro Pump, Aerial Ace, Double Team, Protect, Quick Attack |

---

## Key Gameplay Mechanics

- **Pacing & Stamina**: 100-120 SP stamina pool with dynamic stamina depletion, dash costs, and recharge windows.
- **3D Z-Axis Elevation**: Low-profile ground attacks (e.g. Earthquake, Scratch) can be evaded by leaping or flying into the air (e.g. Aerial Ace, Seismic Toss, Charizard flight).
- **Juiced Combat Impacts**: Dynamic frame freezes (hitstop), multi-frequency screen shake, additive energy particle bursts, motion-blur afterimages, and persistent arena decals (scorch marks, sand furrows, craters).
- **Melee Clashes**: Opposing high-speed melee moves (e.g. Night Slash vs Volt Tackle) clash at impact points with spark explosions and camera tremors.
- **Custom Move Drafter**: Fully customizable 4-slot movesets drafted from each Pokémon's canonical movepool.
- **Tactical Battle AI**: Features Casual, Training Dummy, and Boss AI modes with human-like reaction latencies, strafe spacing, hazard avoidance, whiff punishes, and hit-confirm combos.

---

## Controls

### In Battle
| Input | Action |
| :--- | :--- |
| **W, A, S, D** / **Arrows** | 8-Directional Movement |
| **Mouse Crosshair** | Free Aim Aiming |
| **Tab** | Toggle Lock-On / Free Aim |
| **Left Click** / **1** | Ability Slot 0 |
| **Right Click** / **2** | Ability Slot 1 |
| **3** / **Left Shift** / **Q** | Ability Slot 2 |
| **4** / **F** / **E** | Ability Slot 3 |
| **Space** | Directional Burst Dash (Stamina Cost: 18 SP) |
| **R** | Instant Match Restart |
| **Escape** | Return to Character Select Menu |

### In Spectator Mode (CPU vs CPU)
| Input | Action |
| :--- | :--- |
| **Space** | Pause / Resume Match |
| **1, 2, 3** | Set Playback Speed (1.0x, 1.5x, 2.0x) |
| **0** | Slow-Motion (0.5x) |

---

## Building & Running

### Prerequisites
- **Linux** (Ubuntu/Debian, Fedora, Arch, etc.)
- **Clang** (or GCC) with **C++20** support
- **SDL2 Development Libraries** (`libsdl2-dev` on Debian/Ubuntu, `SDL2-devel` on Fedora, `sdl2` on Arch)
- Python 3 (for sprite processing pipeline)

### Quick Start
```bash
# Clone the repository
git clone https://github.com/<your-username>/pokemon-battlegrounds.git
cd pokemon-battlegrounds

# Build and run
make
./pokemon_battle

# Run automated headless simulation test suite (20 tests)
make test
```

---

## Non-Commercial Disclaimer

This is an unofficial, non-commercial open-source fan project created purely for educational and technical demonstration purposes. Pokémon and Pokémon character names, sprites, and properties are trademarks of Nintendo, Creatures Inc., and GAME FREAK Inc. No copyright infringement is intended.
