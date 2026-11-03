#include "spark/demo/SparkPixShowcase2DDemo.hpp"

#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Spark {

namespace {

struct HeroSpec {
    const char* manifestPath;
    const char* textureRegisterId;
    const char* label;
    float worldX;
};

constexpr HeroSpec kHeroSpecs[] = {
        {"/sprites/spark_pix/ref_youth.spark-atlas.json",
         "spark/spark_pix/ref_youth",
         "ref_youth",
         -4.2F},
        {"/sprites/spark_pix/ref_elder.spark-atlas.json",
         "spark/spark_pix/ref_elder",
         "ref_elder",
         0.0F},
        {"/sprites/spark_pix/rose_hero.spark-atlas.json",
         "spark/spark_pix/rose_hero",
         "rose_hero",
         4.2F},
};

constexpr float kWalkSpeedMetersPerSecond = 2.8F;
constexpr float kRunSpeedMetersPerSecond = 5.6F;
constexpr float kMoveHalfExtentX = 7.5F;
constexpr float kHeroUniformScale = 0.75F;
constexpr float kFloorCenterY = -2.4F;
constexpr float kFloorScaleY = 0.4F;
/** Sprite quads are unit squares centered on the transform; top of the floor strip in world Y. */
constexpr float kFloorTopY = kFloorCenterY + 0.5F * kFloorScaleY;
/** Place transform so the bottom of the scaled sprite sits on the floor. */
constexpr float kHeroCenterY = kFloorTopY + 0.5F * kHeroUniformScale;

[[nodiscard]] std::uint32_t FindClipIndexByName(
        const SparkPixAtlasManifest& manifest,
        const char* clipName,
        const std::uint32_t fallback) noexcept {
    if (clipName == nullptr) {
        return fallback;
    }
    for (std::size_t i = 0; i < manifest.clips.GetSize(); ++i) {
        if (std::strcmp(manifest.clips[i].name.CStr(), clipName) == 0) {
            return static_cast<std::uint32_t>(i);
        }
    }
    return fallback;
}

void ApplyClip(SpriteAnimatorComponent* animator, const std::uint32_t clipIndex) noexcept {
    if (animator == nullptr) {
        return;
    }
    if (clipIndex < animator->GetClipCount()) {
        animator->SetClipIndex(clipIndex);
    }
}

void SetHeroFacing(TransformComponent& transform, const float facingSign) noexcept {
    const float sign = facingSign >= 0.0F ? 1.0F : -1.0F;
    transform.SetScale({sign * kHeroUniformScale, kHeroUniformScale, kHeroUniformScale});
}

GameObject& AddRoot(GameWorld& world, Array<GameObject*>& roots, const char* name) {
    GameObject* go = world.CreateGameObject();
    go->GetName() = Utf8String(name);
    roots.PushBack(go);
    return *go;
}

}  // namespace

void SparkPixShowcase2DDemo::Load(GameWorld& world, IEngineContext& context) {
    Unload(world);
    (void)context;
    camera.position = {0.0F, 0.0F, 0.0F};
    camera.halfExtentY = 4.5F;
    activeHeroIndex = 0;

    GameObject& floor = AddRoot(world, roots, "Floor");
    TransformComponent& floorTr = *floor.AddComponent<TransformComponent>();
    floorTr.SetTranslation({0.0F, kFloorCenterY, -0.02F});
    floorTr.SetScale({16.0F, kFloorScaleY, 1.0F});
    SharedPtr<Texture2D> floorTex = MakeShared<Texture2D>(Utf8String("SparkPixFloor"));
    *floorTex = Texture2D::CreateSolid(4, 4, Vector3{0.18F, 0.22F, 0.28F}, 1.0F);
    world.RegisterTexture(floorTex, "spark/spark_pix/floor");
    floor.AddComponent<SpriteComponent>(
            floorTex, Vector4{1.0F, 1.0F, 1.0F, 1.0F}, Vector4{0.0F, 0.0F, 1.0F, 1.0F}, -5);

    for (std::size_t i = 0; i < sizeof(kHeroSpecs) / sizeof(kHeroSpecs[0]); ++i) {
        const HeroSpec& spec = kHeroSpecs[i];
        LoadedHero hero{};
        hero.label = Utf8String(spec.label);
        if (!TryLoadSparkPixAtlasManifest(spec.manifestPath, hero.manifest, hero.manifestPath)) {
            continue;
        }
        hero.texture = MakeShared<Texture2D>(Utf8String(spec.label));
        if (!TryLoadSparkPixAtlasTexture(hero.manifest, hero.manifestPath, *hero.texture)) {
            continue;
        }
        world.RegisterTexture(hero.texture, spec.textureRegisterId);

        Utf8String goName{};
        goName.AppendUtf8("Hero_");
        goName.AppendUtf8(spec.label);
        GameObject& go = AddRoot(world, roots, goName.CStr());
        hero.object = &go;
        TransformComponent& tr = *go.AddComponent<TransformComponent>();
        hero.transform = &tr;
        tr.SetTranslation({spec.worldX, kHeroCenterY, 0.0F});
        SetHeroFacing(tr, 1.0F);

        const Vector4 uv0 = SpriteAnimatorComponent::ComputeUniformGridUv(
                hero.manifest.columns,
                hero.manifest.rows,
                0U,
                hero.texture->GetWidth(),
                hero.texture->GetHeight());
        go.AddComponent<SpriteComponent>(hero.texture, Vector4{1.0F, 1.0F, 1.0F, 1.0F}, uv0, static_cast<int>(i));
        hero.animator = go.AddComponent<SpriteAnimatorComponent>();
        ApplySparkPixManifestToAnimator(hero.manifest, *hero.animator);
        hero.idleClipIndex = FindClipIndexByName(hero.manifest, "idle", 0U);
        hero.walkClipIndex = FindClipIndexByName(hero.manifest, "walk", hero.idleClipIndex);
        hero.runClipIndex = FindClipIndexByName(hero.manifest, "run", hero.walkClipIndex);
        hero.facingSign = 1.0F;
        ApplyClip(hero.animator, hero.idleClipIndex);
        heroes.PushBack(MoveTemp(hero));
    }

    helpHud.Mount(world, "Spark-pix showcase");
    if (heroes.IsEmpty()) {
        helpHud.SetDetail(
                "Missing atlases — build spark-pix ref_youth/ref_elder/rose_hero and copy to assets/sprites/spark_pix/");
        helpHud.SetControlHints("Scroll: zoom");
        return;
    }

    helpHud.SetDetail("idle");
    helpHud.SetControlHints(
            "Arrows/A/D: move · Shift: run · Tab/1-3: switch hero · Scroll: zoom");
}

void SparkPixShowcase2DDemo::Unload(GameWorld& world) {
    helpHud.Unmount(world);
    heroes.Clear();
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            world.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
}

void SparkPixShowcase2DDemo::Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world) {
    (void)world;
    IInput& in = context.GetInput();

    if (!heroes.IsEmpty()) {
        if (in.IsKeyPressedThisFrame(GLFW_KEY_TAB)) {
            activeHeroIndex = (activeHeroIndex + 1U) % heroes.GetSize();
        }
        for (std::size_t i = 0; i < heroes.GetSize() && i < 3U; ++i) {
            const int digitKey = static_cast<int>(GLFW_KEY_1 + i);
            if (in.IsKeyPressedThisFrame(digitKey)) {
                activeHeroIndex = i;
            }
        }

        float moveAxis = 0.0F;
        if (in.IsKeyDown(GLFW_KEY_A) || in.IsKeyDown(GLFW_KEY_LEFT)) {
            moveAxis -= 1.0F;
        }
        if (in.IsKeyDown(GLFW_KEY_D) || in.IsKeyDown(GLFW_KEY_RIGHT)) {
            moveAxis += 1.0F;
        }
        const bool running =
                in.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || in.IsKeyDown(GLFW_KEY_RIGHT_SHIFT);
        const float speed =
                running ? kRunSpeedMetersPerSecond : kWalkSpeedMetersPerSecond;

        LoadedHero& active = heroes[activeHeroIndex];
        if (active.transform != nullptr && moveAxis != 0.0F) {
            Vector3 pos = active.transform->GetLocalTransform().translation;
            pos.x += moveAxis * speed * timing.deltaTimeSeconds;
            pos.x = (std::clamp)(pos.x, -kMoveHalfExtentX, kMoveHalfExtentX);
            active.transform->SetTranslation(pos);
            active.facingSign = moveAxis >= 0.0F ? 1.0F : -1.0F;
            SetHeroFacing(*active.transform, active.facingSign);
        }

        camera.position.x =
                active.transform != nullptr ? active.transform->GetLocalTransform().translation.x : 0.0F;

        const char* detail = "idle";
        if (moveAxis != 0.0F) {
            detail = running ? "run" : "walk";
        }

        for (std::size_t h = 0; h < heroes.GetSize(); ++h) {
            LoadedHero& hero = heroes[h];
            if (h == activeHeroIndex) {
                if (moveAxis == 0.0F) {
                    ApplyClip(hero.animator, hero.idleClipIndex);
                } else if (running) {
                    ApplyClip(hero.animator, hero.runClipIndex);
                } else {
                    ApplyClip(hero.animator, hero.walkClipIndex);
                }
            } else {
                ApplyClip(hero.animator, hero.idleClipIndex);
            }
        }

        Utf8String detailLine{};
        detailLine.AppendUtf8(active.label.CStr());
        detailLine.AppendUtf8(" · ");
        detailLine.AppendUtf8(detail);
        if (activeHeroIndex < heroes.GetSize()) {
            helpHud.SetDetail(detailLine.CStr());
        }
    }

    const float scroll = in.GetScrollDeltaY();
    if (scroll != 0.0F) {
        camera.halfExtentY = (std::max)(1.5F, camera.halfExtentY - scroll * 0.35F);
    }
}

void SparkPixShowcase2DDemo::Render(Scene& /*scene*/, GameWorld& world, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) {
        fbW = 1;
    }
    if (fbH <= 0) {
        fbH = 1;
    }
    const Matrix4 viewProj = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
    Vector3 pr{};
    Vector3 pu{};
    camera.BillboardBasisWorld(pr, pu);
    SubmitStandardLitSceneFromWorld(
            world,
            context,
            viewProj,
            camera.position,
            Vector3{0.55F, 0.62F, 0.72F}.Normalized(),
            Vector3{0.96F, 0.98F, 1.0F},
            0.65F,
            Vector3{0.08F, 0.10F, 0.14F},
            false,
            pr,
            pu,
            0.0F,
            SceneSpriteSortMode::SortOrderOnly);

    SceneRenderParams* sceneParams = nullptr;
    if (context.TryGetMutableSceneRenderParams(sceneParams) && sceneParams != nullptr) {
        helpHud.PatchSceneRenderParams(*sceneParams, world);
    }
}

}  // namespace Spark
