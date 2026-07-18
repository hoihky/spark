#include "spark/demo/RenderLayers2DDemo.hpp"

#include "spark/ecs/components/camera/Camera2DComponent.hpp"
#include "spark/ecs/components/camera/Camera2DRigComponent.hpp"
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/scene/SceneSubmit.hpp"

#include <cmath>
#include <format>

namespace Spark {

namespace {

Spark::GameObject& AddRoot(Spark::GameWorld& w, Spark::Array<Spark::GameObject*>& roots, const char* name) {
    Spark::GameObject* go = w.CreateGameObject();
    go->GetName() = Spark::Utf8String(name);
    roots.PushBack(go);
    return *go;
}

Spark::SpriteComponent& AddSprite(
        Spark::GameObject& go,
        const Spark::SharedPtr<Spark::Texture2D>& tex,
        const Spark::Vector4& tint,
        const Spark::Vector4& uv,
        const std::int32_t sortOrder,
        const Spark::SceneBlendMode blend = Spark::kSceneBlendModeDefault) {
    if (blend != Spark::kSceneBlendModeDefault) {
        go.AddComponent<Spark::BlendModeComponent>(blend);
    }
    return *go.AddComponent<Spark::SpriteComponent>(tex, tint, uv, sortOrder);
}

void PlaceSprite(
        Spark::TransformComponent& tr,
        const Spark::Vector3& center,
        const Spark::Vector2& halfSize,
        const float z = 0.0F) {
    tr.SetTranslation({center.x, center.y, z});
    tr.SetScale({halfSize.x * 2.0F, halfSize.y * 2.0F, 1.0F});
}

Spark::SpriteAnimatorComponent& AddFarmerAnimator(
        Spark::GameObject& go,
        const std::uint32_t atlasCols) {
    auto& anim = *go.AddComponent<Spark::SpriteAnimatorComponent>();
    anim.SetUniformGrid(atlasCols, RenderLayers2DDemo::kFarmerAtlasRows);
    anim.AddClip(Spark::SpriteAnimationClip{0, 1, 1.0F, true});
    anim.AddClip(Spark::SpriteAnimationClip{1, 2, 9.0F, true});
    anim.SetClipIndex(0);
    return anim;
}

}  // namespace

Spark::Vector4 RenderLayers2DDemo::FarmTileUv(const std::uint32_t tileOneBased) const noexcept {
    if (usingKenneyTiles) {
        return DemoAssets::KenneySimplifiedPlatformerTileUv(tileOneBased);
    }
    const float u = static_cast<float>((tileOneBased - 1U) % 4U) * 0.25F;
    return Spark::Vector4{u, 0.0F, u + 0.25F, 0.5F};
}

Spark::Vector4 RenderLayers2DDemo::FarmerFrameUv(const std::uint32_t frameIndex) const noexcept {
    return Spark::SpriteAnimatorComponent::ComputeUniformGridUv(farmerAtlasCols, kFarmerAtlasRows, frameIndex);
}

void RenderLayers2DDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context) {
    (void)context;
    roots.Clear();
    brokenShadowOnTop = true;
    sceneTime = 0.0F;

    farmTilesTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("FarmTilesheet"));
    usingKenneyTiles = DemoAssets::TryLoadKenneySimplifiedPlatformerTilesheet(*farmTilesTex);
    if (!usingKenneyTiles) {
        *farmTilesTex = Spark::Texture2D::CreateCheckerboard(
                256,
                32,
                Spark::Vector3{0.42F, 0.62F, 0.28F},
                Spark::Vector3{0.55F, 0.38F, 0.22F});
        farmTilesTex->GetName() = Spark::Utf8String("FarmTilesFallback");
    }
    w.RegisterTexture(farmTilesTex, "spark/render_layers/farm_tiles");

    farmerAtlasTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("FarmFarmerAtlas"));
    usingKenneyFarmer = DemoAssets::TryBuildKenneyPlayerAtlas(*farmerAtlasTex, farmerAtlasCols);
    if (!usingKenneyFarmer) {
        *farmerAtlasTex = DemoAssets::MakePlayerRunAtlasFallback();
        farmerAtlasCols = DemoAssets::kPlayerAtlasFallbackCols;
    }
    w.RegisterTexture(farmerAtlasTex, "spark/render_layers/farmer_atlas");

    cropItemTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("FarmCropItem"));
    usingKenneyCrop = DemoAssets::TryLoadKenneyGemCollectible(*cropItemTex);
    if (!usingKenneyCrop) {
        *cropItemTex = DemoAssets::MakeGemTextureFallback();
        cropItemTex->GetName() = Spark::Utf8String("FarmCropFallback");
    }
    w.RegisterTexture(cropItemTex, "spark/render_layers/crop_item");

    Spark::RenderLayerRegistry& layers = Spark::RenderLayerRegistry::Instance();
    const Spark::RenderLayerId backgroundLayer = layers.FindLayerIdByName("Background");
    const Spark::RenderLayerId charactersLayer = layers.FindLayerIdByName("Characters");
    const Spark::RenderLayerId effectsLayer = layers.FindLayerIdByName("Effects");

  {
        Spark::GameObject& farm = AddRoot(w, roots, "FarmTilemap");
        farm.AddComponent<Spark::RenderLayerComponent>(backgroundLayer);
        farm.AddComponent<Spark::TransformComponent>();
        Spark::TilemapComponent* tm = farm.AddComponent<Spark::TilemapComponent>(
                farmTilesTex,
                kFarmMapW,
                kFarmMapH,
                DemoAssets::kKenneyTilesheetCols,
                DemoAssets::kKenneyTilesheetRows,
                kTileWorld,
                0);
        for (std::uint32_t y = 0; y < kFarmMapH; ++y) {
            for (std::uint32_t x = 0; x < kFarmMapW; ++x) {
                std::uint16_t tile = 1U;
                if (y == 0U || y == kFarmMapH - 1U) {
                    tile = 1U;
                } else if ((y % 2U) == 1U) {
                    tile = static_cast<std::uint16_t>(2U + (x % 2U));
                } else {
                    tile = 1U;
                }
                tm->SetTile(x, y, tile);
            }
        }
    }

    auto addFencePost = [&](const char* name, const float x, const float y) {
        Spark::GameObject& post = AddRoot(w, roots, name);
        post.AddComponent<Spark::RenderLayerComponent>(backgroundLayer);
        Spark::TransformComponent& tr = *post.AddComponent<Spark::TransformComponent>();
        PlaceSprite(tr, {x, y, 0.0F}, {0.45F, 0.55F}, 0.015F);
        AddSprite(
                post,
                farmTilesTex,
                Spark::Vector4{0.92F, 0.90F, 0.84F, 1.0F},
                FarmTileUv(20U),
                2);
    };
    addFencePost("FencePostL", 0.6F, 7.1F);
    addFencePost("FencePostM", 7.0F, 7.1F);
    addFencePost("FencePostR", 13.4F, 7.1F);

    auto addCropPatch = [&](const char* name, const float x, const float y, const std::int32_t order) {
        Spark::GameObject& crop = AddRoot(w, roots, name);
        crop.AddComponent<Spark::RenderLayerComponent>(backgroundLayer);
        Spark::TransformComponent& tr = *crop.AddComponent<Spark::TransformComponent>();
        PlaceSprite(tr, {x, y, 0.0F}, {0.22F, 0.28F}, 0.02F);
        const Spark::Vector3 tint{
                0.35F + 0.12F * std::sin(x * 0.7F),
                0.78F + 0.08F * std::cos(y * 0.5F),
                0.22F};
        AddSprite(
                crop,
                cropItemTex,
                Spark::Vector4{tint.x, tint.y, tint.z, 1.0F},
                Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                order);
    };
    addCropPatch("CropA", 2.5F, 2.5F, 3);
    addCropPatch("CropB", 5.5F, 4.5F, 4);
    addCropPatch("CropC", 9.0F, 2.5F, 3);
    addCropPatch("CropD", 11.5F, 4.5F, 4);

    {
        Spark::GameObject& barn = AddRoot(w, roots, "Barn");
        barn.AddComponent<Spark::RenderLayerComponent>(backgroundLayer);
        Spark::TransformComponent& tr = *barn.AddComponent<Spark::TransformComponent>();
        PlaceSprite(tr, {7.0F, 7.55F, 0.0F}, {2.2F, 0.85F}, 0.012F);
        AddSprite(
                barn,
                farmTilesTex,
                Spark::Vector4{0.78F, 0.42F, 0.28F, 0.95F},
                FarmTileUv(40U),
                1);
    }

    const Spark::Vector4 farmerTint{1.0F, 1.0F, 1.0F, 1.0F};
    const Spark::Vector4 shadowTint{0.08F, 0.10F, 0.06F, 0.70F};
    const Spark::Vector4 hatTint{0.92F, 0.82F, 0.38F, 0.98F};

    {
        Spark::GameObject& brokenFarmer = AddRoot(w, roots, "BrokenFarmer");
        brokenFarmer.AddComponent<Spark::RenderLayerComponent>(charactersLayer);
        brokenFarmerTr = brokenFarmer.AddComponent<Spark::TransformComponent>();
        PlaceSprite(*brokenFarmerTr, {4.0F, 3.6F, 0.0F}, {kFarmerDrawScaleX * 0.5F, kFarmerDrawScaleY * 0.5F}, 0.04F);
        AddSprite(brokenFarmer, farmerAtlasTex, farmerTint, FarmerFrameUv(0U), 10);
        brokenFarmerAnim = &AddFarmerAnimator(brokenFarmer, farmerAtlasCols);

        Spark::GameObject& brokenHat = AddRoot(w, roots, "BrokenHat");
        brokenHat.AddComponent<Spark::RenderLayerComponent>(charactersLayer);
        brokenHatTr = brokenHat.AddComponent<Spark::TransformComponent>();
        PlaceSprite(*brokenHatTr, {4.0F, 4.35F, 0.0F}, {0.28F, 0.14F}, 0.05F);
        AddSprite(
                brokenHat,
                cropItemTex,
                hatTint,
                Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                20);

        brokenShadowGo = &AddRoot(w, roots, "BrokenShadow");
        brokenShadowGo->AddComponent<Spark::RenderLayerComponent>(charactersLayer);
        Spark::TransformComponent& shadowTr = *brokenShadowGo->AddComponent<Spark::TransformComponent>();
        PlaceSprite(shadowTr, {4.15F, 3.15F, 0.0F}, {0.42F, 0.10F}, 0.02F);
        Spark::SpriteComponent& shadowSp =
                AddSprite(*brokenShadowGo, farmerAtlasTex, shadowTint, FarmerFrameUv(0U), 50);
        brokenShadowGo->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Multiply);
        shadowSp.SetSortOrder(50);
    }

    {
        Spark::GameObject& groupedRoot = AddRoot(w, roots, "GroupedFarmerRoot");
        groupedRoot.AddComponent<Spark::RenderLayerComponent>(charactersLayer);
        groupedRoot.AddComponent<Spark::SortingGroupComponent>(100);
        groupedRootTr = groupedRoot.AddComponent<Spark::TransformComponent>();
        groupedRootTr->SetTranslation({10.5F, 3.6F, 0.0F});

        Spark::GameObject& groupedShadow = AddRoot(w, roots, "GroupedShadow");
        groupedShadow.SetParent(&groupedRoot);
        Spark::TransformComponent& shadowTr = *groupedShadow.AddComponent<Spark::TransformComponent>();
        PlaceSprite(shadowTr, {0.15F, -0.45F, 0.0F}, {0.42F, 0.10F}, 0.02F);
        AddSprite(groupedShadow, farmerAtlasTex, shadowTint, FarmerFrameUv(0U), 0);
        groupedShadow.AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Multiply);

        Spark::GameObject& groupedBody = AddRoot(w, roots, "GroupedFarmer");
        groupedBody.SetParent(&groupedRoot);
        Spark::TransformComponent& bodyTr = *groupedBody.AddComponent<Spark::TransformComponent>();
        groupedFarmerTr = &bodyTr;
        PlaceSprite(bodyTr, {0.0F, 0.0F, 0.0F}, {kFarmerDrawScaleX * 0.5F, kFarmerDrawScaleY * 0.5F}, 0.04F);
        AddSprite(groupedBody, farmerAtlasTex, farmerTint, FarmerFrameUv(0U), 1);
        groupedFarmerAnim = &AddFarmerAnimator(groupedBody, farmerAtlasCols);

        Spark::GameObject& groupedHat = AddRoot(w, roots, "GroupedHat");
        groupedHat.SetParent(&groupedRoot);
        Spark::TransformComponent& hatTr = *groupedHat.AddComponent<Spark::TransformComponent>();
        PlaceSprite(hatTr, {0.0F, 0.75F, 0.0F}, {0.28F, 0.14F}, 0.05F);
        AddSprite(
                groupedHat,
                cropItemTex,
                hatTint,
                Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                2);
    }

    {
        fxSparkleGo = &AddRoot(w, roots, "HarvestSparkle");
        fxSparkleGo->AddComponent<Spark::RenderLayerComponent>(effectsLayer);
        Spark::TransformComponent& tr = *fxSparkleGo->AddComponent<Spark::TransformComponent>();
        PlaceSprite(tr, {7.0F, 5.2F, 0.0F}, {0.32F, 0.32F}, 0.06F);
        fxSparkleGo->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Additive);
        AddSprite(
                *fxSparkleGo,
                cropItemTex,
                Spark::Vector4{1.0F, 0.95F, 0.55F, 0.9F},
                Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                0);
    }

    {
        Spark::GameObject& camGo = AddRoot(w, roots, "MainCamera");
        mainCameraGo = &camGo;
        Spark::TransformComponent& camTr = *camGo.AddComponent<Spark::TransformComponent>();
        camTr.SetTranslation({kFarmMapW * kTileWorld * 0.5F, kFarmMapH * kTileWorld * 0.48F, 0.0F});
        mainCamera = camGo.AddComponent<Spark::Camera2DComponent>();
        mainCamera->SetHalfExtentY(5.2F);
        mainCamera->SetPriority(10);
        Spark::Camera2DRigComponent* rig = camGo.AddComponent<Spark::Camera2DRigComponent>();
        rig->SetMode(Spark::Camera2DRigMode::Manual);
    }

    hudGo = &AddRoot(w, roots, "Hud");
    hudText = hudGo->AddComponent<Spark::TextOverlayComponent>();
    hudText->SetScreenPosition(12.0F, 12.0F);
    hudText->SetFontSizePixels(18.0F);
    hudText->SetColor({0.95F, 0.98F, 0.90F});
    {
        const char* art = usingKenneyTiles && usingKenneyFarmer ? "Kenney farm art" : "procedural farm fallback";
        hudText->SetText(Spark::Utf8String(
                std::format(
                        "Farming RPG render layers ({}) — WASD: lone farmer · IJKL: grouped farmer · Q: broken shadow · "
                        "ESC menu",
                        art)
                        .c_str()));
    }
}

void RenderLayers2DDemo::Unload(Spark::GameWorld& w) {
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            w.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    farmTilesTex.Reset();
    farmerAtlasTex.Reset();
    cropItemTex.Reset();
    brokenShadowGo = nullptr;
    brokenFarmerTr = nullptr;
    brokenHatTr = nullptr;
    brokenFarmerAnim = nullptr;
    groupedRootTr = nullptr;
    groupedFarmerTr = nullptr;
    groupedFarmerAnim = nullptr;
    fxSparkleGo = nullptr;
    hudGo = nullptr;
    hudText = nullptr;
    mainCameraGo = nullptr;
    mainCamera = nullptr;
}

void RenderLayers2DDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context) {
    Spark::IInput& in = context.GetInput();
    const float dt = timing.deltaTimeSeconds;
    sceneTime += dt;

    const float move = 4.8F * dt;
    auto updateFarmerFacing = [](Spark::TransformComponent* tr, const bool movingLeft, bool& facingLeft) {
        if (tr == nullptr) {
            return;
        }
        if (movingLeft) {
            facingLeft = true;
        }
        Spark::Vector3 s = tr->GetLocalTransform().scale;
        s.x = std::fabs(s.x) * (facingLeft ? -1.0F : 1.0F);
        tr->SetScale(s);
    };
    auto setWalkClip = [](Spark::SpriteAnimatorComponent* anim, const bool walking) {
        if (anim == nullptr) {
            return;
        }
        anim->SetClipIndex(walking ? 1U : 0U);
    };

    if (brokenFarmerTr != nullptr) {
        Spark::Vector3 p = brokenFarmerTr->GetLocalTransform().translation;
        bool walking = false;
        if (in.IsKeyDown(GLFW_KEY_W)) {
            p.y += move;
            walking = true;
        }
        if (in.IsKeyDown(GLFW_KEY_S)) {
            p.y -= move;
            walking = true;
        }
        if (in.IsKeyDown(GLFW_KEY_A)) {
            p.x -= move;
            walking = true;
            brokenFacingLeft = true;
        }
        if (in.IsKeyDown(GLFW_KEY_D)) {
            p.x += move;
            walking = true;
            brokenFacingLeft = false;
        }
        brokenFarmerTr->SetTranslation({p.x, p.y, 0.04F});
        updateFarmerFacing(brokenFarmerTr, brokenFacingLeft, brokenFacingLeft);
        setWalkClip(brokenFarmerAnim, walking);

        if (brokenShadowGo != nullptr) {
            if (Spark::TransformComponent* shadowTr = brokenShadowGo->GetComponent<Spark::TransformComponent>()) {
                shadowTr->SetTranslation({p.x + 0.15F, p.y - 0.45F, 0.02F});
            }
        }
        if (brokenHatTr != nullptr) {
            brokenHatTr->SetTranslation({p.x, p.y + 0.75F, 0.05F});
        }
    }

    if (groupedRootTr != nullptr) {
        Spark::Vector3 p = groupedRootTr->GetLocalTransform().translation;
        bool walking = false;
        if (in.IsKeyDown(GLFW_KEY_I)) {
            p.y += move;
            walking = true;
        }
        if (in.IsKeyDown(GLFW_KEY_K)) {
            p.y -= move;
            walking = true;
        }
        if (in.IsKeyDown(GLFW_KEY_J)) {
            p.x -= move;
            walking = true;
            groupedFacingLeft = true;
        }
        if (in.IsKeyDown(GLFW_KEY_L)) {
            p.x += move;
            walking = true;
            groupedFacingLeft = false;
        }
        groupedRootTr->SetTranslation({p.x, p.y, 0.0F});
        updateFarmerFacing(groupedFarmerTr, groupedFacingLeft, groupedFacingLeft);
        setWalkClip(groupedFarmerAnim, walking);
    }

    if (in.IsKeyPressedThisFrame(GLFW_KEY_Q) && brokenShadowGo != nullptr) {
        brokenShadowOnTop = !brokenShadowOnTop;
        if (Spark::SpriteComponent* shadowSp = brokenShadowGo->GetComponent<Spark::SpriteComponent>()) {
            shadowSp->SetSortOrder(brokenShadowOnTop ? 50 : 5);
        }
    }

    if (fxSparkleGo != nullptr) {
        if (Spark::TransformComponent* tr = fxSparkleGo->GetComponent<Spark::TransformComponent>()) {
            const float bob = std::sin(sceneTime * 2.8F) * 0.22F;
            const float sway = std::cos(sceneTime * 1.9F) * 0.28F;
            tr->SetTranslation({7.0F + sway, 5.2F + bob, 0.06F});
        }
    }

    if (hudText != nullptr) {
        const float instant = (dt > 1.0e-6F) ? (1.0F / dt) : 0.0F;
        fpsSmoothed = (timing.frameIndex < 2U) ? instant : fpsSmoothed * 0.88F + instant * 0.12F;
        const char* shadowState =
                brokenShadowOnTop ? "shadow covers farmer (broken)" : "shadow under farmer (fixed)";
        const char* art = usingKenneyTiles && usingKenneyFarmer ? "Kenney" : "fallback";
        hudText->SetText(Spark::Utf8String(
                std::format(
                        "Farming RPG layers — {:.0f} FPS — {} art\n"
                        "Background: tilemap + fence + crops · Characters: farmers (Y-sort) · Effects: harvest sparkle\n"
                        "Left farmer (no group): {} · Right farmer: SortingGroup shadow→body→hat · Q toggles shadow",
                        static_cast<double>(fpsSmoothed),
                        art,
                        shadowState)
                        .c_str()));
    }
}

void RenderLayers2DDemo::Render(
        Spark::Scene& /*scene*/,
        Spark::GameWorld& world,
        Spark::IEngineContext& context) {
    (void)Spark::SubmitStandardLitSceneFromWorldWithCamera(
            world,
            context,
            Spark::Vector3{0.35F, 0.88F, 0.48F}.Normalized(),
            Spark::Vector3{1.0F, 0.98F, 0.92F},
            0.95F,
            Spark::Vector3{0.42F, 0.52F, 0.38F},
            false,
            sceneTime,
            Spark::SceneSpriteSortMode::SortOrderThenWorldY);
}

}  // namespace Spark
