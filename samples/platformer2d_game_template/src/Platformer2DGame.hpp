#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/Game.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Camera2D.hpp"
#include "spark/scene/Texture2D.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class TextOverlayComponent;
class TransformComponent;
class Rigidbody2DComponent;

/**
 * Minimal side-scrolling platformer template: orthographic Camera2D, 2D physics, Kenney CC0 art when present.
 * Procedural checker tiles and tint sprites are used when asset packs are missing.
 */
class Platformer2DGame final : public Game {
public:
    void OnAttach(IEngineContext& context) override;
    void OnDetach() override;
    void OnUpdate(const FrameTiming& timing, IEngineContext& context) override;
    void OnRender(IRenderFrame& frame, IEngineContext& context) override;

private:
    void MountUiFontIfNeeded(GameWorld& world);
    void CloseWindowIfRequested(IEngineContext& context) const;

    Camera2D camera{};
    SharedPtr<Texture2D> tileTex{};
    SharedPtr<Texture2D> playerAtlasTex{};
    bool usingKenneyTiles = false;
    bool usingKenneyPlayer = false;
    std::uint32_t playerAtlasColumns = 1U;
    Array<GameObject*> roots{};
    GameObject* playerObject = nullptr;
    TransformComponent* playerTr = nullptr;
    Rigidbody2DComponent* playerRb = nullptr;
    TextOverlayComponent* hudText = nullptr;
    float sceneTimeSeconds = 0.0F;
    bool facingLeft = false;
    bool goalReached = false;
    float playerBaseScaleX = 0.88F;
    float playerBaseScaleY = 1.05F;
};

}  // namespace Spark
