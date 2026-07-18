---
title: Platformer Introduction
order: 1
---

# Platformer Introduction

We walk through `samples/platformer2d_game_template/` — a complete side-scroller with:

- Procedural checker tile art (no external sprites required)
- `Camera2D` with smooth follow
- `Rigidbody2DComponent` player + static platforms
- HUD via `TextOverlayComponent`
- Goal reach detection

## Class Design: `Platformer2DGame`

```cpp
class Platformer2DGame final : public Game {
    Camera2D camera{};
    SharedPtr<Texture2D> tileTex{};
    Array<GameObject*> roots{};
    GameObject* playerObject = nullptr;
    TransformComponent* playerTr = nullptr;
    Rigidbody2DComponent* playerRb = nullptr;
    TextOverlayComponent* hudText = nullptr;
};
```

## Entry Point

```cpp
#include "spark/engine/Engine.hpp"
#include "Platformer2DGame.hpp"

int main() {
    Spark::Engine engine(Spark::Engine::NewGame<Spark::Platformer2DGame>());
    engine.Run();
    return 0;
}
```

## Constants (Sample)

```cpp
constexpr float kRunSpeed = 9.0F;
constexpr float kJumpSpeed = 11.5F;
constexpr float kGroundTopY = -1.0F;
constexpr float kGoalMinX = 15.5F;
```

Next: [Project Setup](7-2d-game/02-project-setup.html).
