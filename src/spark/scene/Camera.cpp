#include "spark/scene/Camera.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/camera/Camera2DComponent.hpp"
#include "spark/ecs/components/camera/CameraComponent.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

bool TryResolveMainCamera(const GameWorld& world, ResolvedCamera& out) noexcept {
    out.object = nullptr;
    out.component = nullptr;
    bool found = false;
    std::int32_t bestPriority = 0;
    world.ForEachGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const CameraComponent* cam = o->GetComponent<CameraComponent>();
        if (cam == nullptr || !cam->IsEnabled()) {
            return;
        }
        if (!found || cam->GetPriority() > bestPriority) {
            found = true;
            bestPriority = cam->GetPriority();
            out.object = o;
            out.component = cam;
        }
    });
    return found;
}

bool TryResolveMainCamera2D(const GameWorld& world, ResolvedCamera2D& out) noexcept {
    out.object = nullptr;
    out.component = nullptr;
    bool found = false;
    std::int32_t bestPriority = 0;
    world.ForEachGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const Camera2DComponent* cam = o->GetComponent<Camera2DComponent>();
        if (cam == nullptr || !cam->IsEnabled()) {
            return;
        }
        if (!found || cam->GetPriority() > bestPriority) {
            found = true;
            bestPriority = cam->GetPriority();
            out.object = o;
            out.component = cam;
        }
    });
    return found;
}

bool TryBuildSceneCamera2DMatrices(
        const GameWorld& world,
        const float framebufferWidth,
        const float framebufferHeight,
        SceneCameraMatrices& out) noexcept {
    ResolvedCamera2D resolved{};
    if (!TryResolveMainCamera2D(world, resolved) || resolved.object == nullptr ||
        resolved.component == nullptr) {
        return false;
    }
    float h = framebufferHeight;
    if (h <= 0.0F) {
        h = 1.0F;
    }
    float w = framebufferWidth;
    if (w <= 0.0F) {
        w = h;
    }
    out.viewProjection =
            resolved.component->ViewProjection(*resolved.object, w, h);
    out.positionWorld = resolved.component->WorldPosition(*resolved.object);
    resolved.component->BillboardBasisWorld(*resolved.object, out.rightWorld, out.upWorld);
    return true;
}

bool TryBuildSceneCameraMatrices(
        const GameWorld& world,
        const float framebufferWidth,
        const float framebufferHeight,
        SceneCameraMatrices& out) noexcept {
    ResolvedCamera2D cam2d{};
    ResolvedCamera cam3d{};
    const bool has2d = TryResolveMainCamera2D(world, cam2d);
    const bool has3d = TryResolveMainCamera(world, cam3d);
    if (!has2d && !has3d) {
        return false;
    }
    if (has2d && !has3d) {
        return TryBuildSceneCamera2DMatrices(world, framebufferWidth, framebufferHeight, out);
    }
    if (!has2d && has3d) {
        ResolvedCamera resolved = cam3d;
        if (resolved.object == nullptr || resolved.component == nullptr) {
            return false;
        }
        float h = framebufferHeight;
        if (h <= 0.0F) {
            h = 1.0F;
        }
        float w = framebufferWidth;
        if (w <= 0.0F) {
            w = h;
        }
        const float aspect = w / h;
        out.viewProjection = resolved.component->ViewProjection(*resolved.object, aspect);
        out.positionWorld = resolved.component->WorldPosition(*resolved.object);
        resolved.component->BillboardBasisWorld(*resolved.object, out.rightWorld, out.upWorld);
        return true;
    }
    const std::int32_t p2d = cam2d.component->GetPriority();
    const std::int32_t p3d = cam3d.component->GetPriority();
    if (p2d >= p3d) {
        return TryBuildSceneCamera2DMatrices(world, framebufferWidth, framebufferHeight, out);
    }
    ResolvedCamera resolved = cam3d;
    if (resolved.object == nullptr || resolved.component == nullptr) {
        return false;
    }
    float h = framebufferHeight;
    if (h <= 0.0F) {
        h = 1.0F;
    }
    float w = framebufferWidth;
    if (w <= 0.0F) {
        w = h;
    }
    const float aspect = w / h;
    out.viewProjection = resolved.component->ViewProjection(*resolved.object, aspect);
    out.positionWorld = resolved.component->WorldPosition(*resolved.object);
    resolved.component->BillboardBasisWorld(*resolved.object, out.rightWorld, out.upWorld);
    return true;
}

}  // namespace Spark
