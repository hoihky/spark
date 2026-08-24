#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class CameraComponent;
class Camera2DComponent;
class GameObject;
class GameWorld;

/** Highest-priority enabled <c>CameraComponent</c> in the world (if any). */
struct ResolvedCamera {
    GameObject* object = nullptr;
    const CameraComponent* component = nullptr;
};

/** Highest-priority enabled <c>Camera2DComponent</c> in the world (if any). */
struct ResolvedCamera2D {
    GameObject* object = nullptr;
    const Camera2DComponent* component = nullptr;
};

/** Finds the enabled camera with the greatest <c>priority</c> value. */
[[nodiscard]] bool TryResolveMainCamera(const GameWorld& world, ResolvedCamera& out) noexcept;

/** Finds the enabled 2D camera with the greatest <c>priority</c> value. */
[[nodiscard]] bool TryResolveMainCamera2D(const GameWorld& world, ResolvedCamera2D& out) noexcept;

/** View/projection and billboard axes from the main ECS camera. */
struct SceneCameraMatrices {
    Matrix4 viewProjection{};
    Vector3 positionWorld{};
    Vector3 rightWorld{Vector3::UnitX};
    Vector3 upWorld{Vector3::UnitY};
};

[[nodiscard]] bool TryBuildSceneCameraMatrices(
        const GameWorld& world,
        float framebufferWidth,
        float framebufferHeight,
        SceneCameraMatrices& out) noexcept;

/** Builds matrices from the main <c>Camera2DComponent</c> only. */
[[nodiscard]] bool TryBuildSceneCamera2DMatrices(
        const GameWorld& world,
        float framebufferWidth,
        float framebufferHeight,
        SceneCameraMatrices& out) noexcept;

}  // namespace Spark
