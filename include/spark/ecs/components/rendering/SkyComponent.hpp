#pragma once

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Texture2D.hpp"

namespace Spark {

/**
 * Marks a GameObject as an environment sky (use with MeshComponent + TransformComponent).
 * SceneSkyMode must match the mesh/shader path: Box = closed env (cube or sphere), Dome = hemisphere,
 * Plane = billboard quad (camera-facing in demos).
 */
class SkyComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Sky;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit SkyComponent(SceneSkyMode mode) noexcept : skyMode(mode) {}

    [[nodiscard]] SceneSkyMode GetSkyMode() const noexcept { return skyMode; }
    void SetSkyMode(SceneSkyMode m) noexcept { skyMode = m; }

    [[nodiscard]] bool IsSkyEnabled() const noexcept { return enabled; }
    void SetSkyEnabled(bool e) noexcept { enabled = e; }

    void SetTint(const Vector3& c) noexcept { tint = c; }
    [[nodiscard]] const Vector3& GetTint() const noexcept { return tint; }

    void SetSkyTexture(SharedPtr<Texture2D> t) { skyTexture = MoveTemp(t); }
    [[nodiscard]] const SharedPtr<Texture2D>& GetSkyTexture() const noexcept { return skyTexture; }

private:
    SceneSkyMode skyMode = SceneSkyMode::Box;
    bool enabled = true;
    Vector3 tint{0.25F, 0.35F, 0.55F};
    SharedPtr<Texture2D> skyTexture{};
};

}  // namespace Spark
