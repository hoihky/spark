# Kenney Simplified Platformer Pack (optional)

`HelloCsGame` and the C++ `Platformer2DDemo` load textures from this tree when PNGs are present. Without them, the engine registers procedural checkerboard / atlas fallbacks.

Expected layout (from [Kenney](https://kenney.nl/assets)):

- `Tilesheet/platformPack_tilesheet.png`
- `PNG/Characters/platformChar_idle.png`, `platformChar_walk1.png`, `platformChar_walk2.png`
- Optional combat frames: `platformChar_happy.png` (attack), `platformChar_duck.png` (hurt)

Paths tried at runtime (repo-relative):

- `assets/sprites/kenney_simplified-platformer-pack/...`
- CMake `SPARK_ASSETS_DIR` / `SPARK_BUILD_ASSETS_DIR` copies

HUD text needs `assets/fonts/Roboto-Regular.ttf` (fetched by CMake when missing).
