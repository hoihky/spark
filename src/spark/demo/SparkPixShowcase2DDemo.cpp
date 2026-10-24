#include "spark/demo/SparkPixShowcase2DDemo.hpp"

#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"

namespace Spark {

namespace {

constexpr const char* kSlimeManifest = "/sprites/spark_pix/slime.spark-atlas.json";

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
    camera.halfExtentY = 4.0F;
    clipIndex = 0;

    if (!TryLoadSparkPixAtlasManifest(kSlimeManifest, slimeManifest, slimeManifestPath)) {
        helpHud.Mount(world, "Spark-pix showcase");
        helpHud.SetDetail("Missing slime.spark-atlas.json — run spark-pix example_slime build and copy assets.");
        return;
    }

    slimeTex = MakeShared<Texture2D>(Utf8String("SparkPixSlimeAtlas"));
    if (!TryLoadSparkPixAtlasTexture(slimeManifest, slimeManifestPath, *slimeTex)) {
        helpHud.Mount(world, "Spark-pix showcase");
        helpHud.SetDetail("Manifest found but texture PNG failed to load.");
        return;
    }
    world.RegisterTexture(slimeTex, "spark/spark_pix/slime");

    GameObject& floor = AddRoot(world, roots, "Floor");
    TransformComponent& floorTr = *floor.AddComponent<TransformComponent>();
    floorTr.SetTranslation({0.0F, -2.2F, -0.02F});
    floorTr.SetScale({14.0F, 0.4F, 1.0F});
    SharedPtr<Texture2D> floorTex = MakeShared<Texture2D>(Utf8String("SparkPixFloor"));
    *floorTex = Texture2D::CreateSolid(4, 4, Vector3{0.18F, 0.22F, 0.28F}, 1.0F);
    world.RegisterTexture(floorTex, "spark/spark_pix/floor");
    floor.AddComponent<SpriteComponent>(
            floorTex, Vector4{1.0F, 1.0F, 1.0F, 1.0F}, Vector4{0.0F, 0.0F, 1.0F, 1.0F}, -5);

    GameObject& slime = AddRoot(world, roots, "Slime");
    slime.AddComponent<TransformComponent>()->SetUniformScale(3.0F);
    const Vector4 uv0 = SpriteAnimatorComponent::ComputeUniformGridUv(
            slimeManifest.columns,
            slimeManifest.rows,
            0U,
            slimeTex->GetWidth(),
            slimeTex->GetHeight());
    slime.AddComponent<SpriteComponent>(slimeTex, Vector4{1.0F, 1.0F, 1.0F, 1.0F}, uv0, 0);
    slimeAnim = slime.AddComponent<SpriteAnimatorComponent>();
    ApplySparkPixManifestToAnimator(slimeManifest, *slimeAnim);
    if (slimeAnim->GetClipCount() > 0U) {
        slimeAnim->SetClipIndex(0U);
    }

    helpHud.Mount(world, "Spark-pix showcase");
    helpHud.SetDetail("slime.spark-atlas.json · Space cycles clips · Scroll zoom");
    helpHud.SetControlHints("Space: next clip · Scroll: zoom");
}

void SparkPixShowcase2DDemo::Unload(GameWorld& world) {
    helpHud.Unmount(world);
    slimeAnim = nullptr;
    slimeTex.Reset();
    slimeManifest = SparkPixAtlasManifest{};
    slimeManifestPath.Clear();
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            world.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
}

void SparkPixShowcase2DDemo::Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world) {
    (void)timing;
    IInput& in = context.GetInput();
    if (slimeAnim != nullptr && in.IsKeyPressedThisFrame(GLFW_KEY_SPACE)) {
        const std::uint32_t n = slimeAnim->GetClipCount();
        if (n > 0U) {
            clipIndex = (clipIndex + 1U) % n;
            slimeAnim->SetClipIndex(clipIndex);
            if (clipIndex < slimeManifest.clips.GetSize()) {
                helpHud.SetDetail(slimeManifest.clips[clipIndex].name.CStr());
            }
        }
    }
    const float scroll = in.GetScrollDeltaY();
    if (scroll != 0.0F) {
        camera.halfExtentY = (std::max)(1.5F, camera.halfExtentY - scroll * 0.35F);
    }
    (void)world;
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
