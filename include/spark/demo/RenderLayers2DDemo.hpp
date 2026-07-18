#pragma once

#include "spark/demo/DemoAssetLoad.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/Camera2DComponent.hpp"
#include "spark/ecs/components/Camera2DRigComponent.hpp"
#include "spark/ecs/components/RenderLayerComponent.hpp"
#include "spark/ecs/components/SortingGroupComponent.hpp"
#include "spark/ecs/components/SpriteAnimatorComponent.hpp"
#include "spark/ecs/components/SpriteComponent.hpp"
#include "spark/ecs/components/TextOverlayComponent.hpp"
#include "spark/ecs/components/TilemapComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"
#include "spark/scene/RenderLayerRegistry.hpp"

namespace Spark {

/**
 * Farming-RPG-style 2D showcase for <c>RenderLayerComponent</c> and <c>SortingGroupComponent</c>
 * using Kenney platformer tiles, farmer sprites, and crop items (with procedural fallbacks).
 */
class RenderLayers2DDemo {
public:
    static constexpr std::uint32_t kFarmMapW = 14U;
    static constexpr std::uint32_t kFarmMapH = 8U;
    static constexpr float kTileWorld = 1.0F;
    static constexpr std::uint32_t kFarmerAtlasRows = 1U;
    static constexpr float kFarmerDrawScaleX = 0.9F;
    static constexpr float kFarmerDrawScaleY = 1.05F;

    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);

    void Unload(Spark::GameWorld& w);

    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);

    void Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context);

private:
    [[nodiscard]] Spark::Vector4 FarmTileUv(std::uint32_t tileOneBased) const noexcept;
    [[nodiscard]] Spark::Vector4 FarmerFrameUv(std::uint32_t frameIndex) const noexcept;

    Spark::Array<Spark::GameObject*> roots{};
    Spark::GameObject* mainCameraGo = nullptr;
    Spark::Camera2DComponent* mainCamera = nullptr;

    Spark::SharedPtr<Spark::Texture2D> farmTilesTex{};
    Spark::SharedPtr<Spark::Texture2D> farmerAtlasTex{};
    Spark::SharedPtr<Spark::Texture2D> cropItemTex{};
    bool usingKenneyTiles = false;
    bool usingKenneyFarmer = false;
    bool usingKenneyCrop = false;
    std::uint32_t farmerAtlasCols = 3U;

    Spark::GameObject* brokenShadowGo = nullptr;
    Spark::TransformComponent* brokenFarmerTr = nullptr;
    Spark::TransformComponent* brokenHatTr = nullptr;
    Spark::SpriteAnimatorComponent* brokenFarmerAnim = nullptr;

    Spark::TransformComponent* groupedRootTr = nullptr;
    Spark::TransformComponent* groupedFarmerTr = nullptr;
    Spark::SpriteAnimatorComponent* groupedFarmerAnim = nullptr;

    Spark::GameObject* fxSparkleGo = nullptr;

    Spark::GameObject* hudGo = nullptr;
    Spark::TextOverlayComponent* hudText = nullptr;

    bool brokenShadowOnTop = true;
    bool brokenFacingLeft = false;
    bool groupedFacingLeft = false;
    float fpsSmoothed = 0.0F;
    float sceneTime = 0.0F;
};

}  // namespace Spark
