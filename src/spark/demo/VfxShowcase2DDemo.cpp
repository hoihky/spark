#include "spark/demo/VfxShowcase2DDemo.hpp"

#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/vfx/VfxSubsystemProcess.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace Spark {

namespace {

GameObject& AddRoot(GameWorld& world, Array<GameObject*>& roots, const char* name) {
    GameObject* go = world.CreateGameObject();
    go->GetName() = Utf8String(name);
    roots.PushBack(go);
    return *go;
}

}  // namespace

void VfxShowcase2DDemo::Load(GameWorld& world, IEngineContext& context) {
    Unload(world);
    (void)context;
    selectedIndex = 0;
    spawnPoint = {0.0F, 0.0F, 0.05F};
    camera.position = {0.0F, 0.0F, 0.0F};
    camera.halfExtentY = 5.5F;

    SharedPtr<Texture2D> floorTex = MakeShared<Texture2D>(Utf8String("VfxShowcase2DFloor"));
    *floorTex = Texture2D::CreateCheckerboard(
            256, 32, Vector3{0.22F, 0.26F, 0.32F}, Vector3{0.28F, 0.34F, 0.42F});
    world.RegisterTexture(floorTex, "spark/vfx_showcase_2d/floor");

    GameObject& floor = AddRoot(world, roots, "Floor");
    TransformComponent& floorTr = *floor.AddComponent<TransformComponent>();
    floorTr.SetTranslation({0.0F, 0.0F, -0.02F});
    floorTr.SetScale({24.0F, 14.0F, 1.0F});
    floor.AddComponent<SpriteComponent>(floorTex, Vector4{1.0F, 1.0F, 1.0F, 1.0F}, Vector4{0.0F, 0.0F, 1.0F, 1.0F}, -10);

    SharedPtr<Texture2D> markerTex = MakeShared<Texture2D>(Utf8String("VfxShowcase2DMarker"));
    *markerTex = Texture2D::CreateSolid(8, 8, Vector3{0.95F, 0.82F, 0.22F}, 0.85F);
    world.RegisterTexture(markerTex, "spark/vfx_showcase_2d/marker");

    GameObject& marker = AddRoot(world, roots, "SpawnMarker");
    TransformComponent& markerTr = *marker.AddComponent<TransformComponent>();
    markerTr.SetTranslation(spawnPoint);
    markerTr.SetUniformScale(0.35F);
    marker.AddComponent<SpriteComponent>(
            markerTex, Vector4{1.0F, 1.0F, 1.0F, 1.0F}, Vector4{0.0F, 0.0F, 1.0F, 1.0F}, 5);

    helpHud.Mount(world, "2D VFX Showcase");
    helpHud.SetDetail(VfxShowcase2DDetail::EntryAt(0).label);
    helpHud.SetControlHints(
            "Up/Down or [ ] cycle · Space play at center · Left-click play at cursor · Scroll zoom");
}

void VfxShowcase2DDemo::Unload(GameWorld& world) {
    helpHud.Unmount(world);
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            world.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
}

void VfxShowcase2DDemo::PlayAt(GameWorld& world, const Vector3& worldPos) {
    const VfxShowcase2DDetail::Entry& entry = VfxShowcase2DDetail::EntryAt(selectedIndex);
    world.GetVfxSubsystem().Queue(entry.assetKey, worldPos);
    ProcessVfx(world);
}

bool VfxShowcase2DDemo::PickWorldXY(
        IEngineContext& context,
        const float px,
        const float py,
        Vector2& outWorld) const {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0 || fbH <= 0) {
        return false;
    }
    const Matrix4 vp = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
    Matrix4 invVp{};
    if (!vp.TryInvert(invVp)) {
        return false;
    }
    Vector3 ro{};
    Vector3 rd{};
    if (!TerrainScreenToWorldRay(fbW, fbH, px, py, invVp, ro, rd)) {
        return false;
    }
    if (std::fabs(rd.z) < 1.0e-5F) {
        return false;
    }
    const float t = -ro.z / rd.z;
    outWorld.x = ro.x + rd.x * t;
    outWorld.y = ro.y + rd.y * t;
    return true;
}

void VfxShowcase2DDemo::Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world) {
    ProcessVfx(world);
    IInput& in = context.GetInput();

    if (in.IsKeyPressedThisFrame(GLFW_KEY_UP) || in.IsKeyPressedThisFrame(GLFW_KEY_LEFT_BRACKET)) {
        selectedIndex = (selectedIndex - 1 + VfxShowcase2DDetail::kEntryCount) % VfxShowcase2DDetail::kEntryCount;
        helpHud.SetDetail(VfxShowcase2DDetail::EntryAt(selectedIndex).label);
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_DOWN) || in.IsKeyPressedThisFrame(GLFW_KEY_RIGHT_BRACKET)) {
        selectedIndex = (selectedIndex + 1) % VfxShowcase2DDetail::kEntryCount;
        helpHud.SetDetail(VfxShowcase2DDetail::EntryAt(selectedIndex).label);
    }

    if (std::fabs(in.GetScrollDeltaY()) > 1.0e-4F) {
        camera.halfExtentY = std::clamp(camera.halfExtentY - in.GetScrollDeltaY() * 0.08F, 2.5F, 12.0F);
    }

    if (in.IsKeyPressedThisFrame(GLFW_KEY_SPACE) || in.IsKeyPressedThisFrame(GLFW_KEY_ENTER)) {
        PlayAt(world, spawnPoint);
    }

    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (in.IsMouseButtonPressedThisFrame(GLFW_MOUSE_BUTTON_LEFT)) {
        float mx = 0.0F;
        float my = 0.0F;
        in.GetCursorFramebufferPixels(mx, my, fbW, fbH);
        Vector2 hitWorld{};
        if (PickWorldXY(context, mx, my, hitWorld)) {
            PlayAt(world, {hitWorld.x, hitWorld.y, spawnPoint.z});
        }
    }

    const VfxShowcase2DDetail::Entry& entry = VfxShowcase2DDetail::EntryAt(selectedIndex);
    const std::uint32_t burst = VfxLibrary::GetDefaultBurstCount(entry.builtin);
    helpHud.SetDetail(std::format(
            "[{}/{}] {} ({}){}",
            selectedIndex + 1,
            VfxShowcase2DDetail::kEntryCount,
            entry.label,
            entry.assetKey,
            burst > 0 ? std::format(" · default burst {}", burst) : " · continuous")
                          .c_str());
    helpHud.Update(timing, context);
}

void VfxShowcase2DDemo::Render(Scene& /*scene*/, GameWorld& world, IEngineContext& context) {
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
            true,
            pr,
            pu,
            0.0F,
            SceneSpriteSortMode::SortOrderThenWorldY);

    SceneRenderParams* sceneParams = nullptr;
    if (context.TryGetMutableSceneRenderParams(sceneParams) && sceneParams != nullptr) {
        helpHud.PatchSceneRenderParams(*sceneParams, world);
    }
}

}  // namespace Spark
