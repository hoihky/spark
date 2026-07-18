#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"

namespace Spark {

class Texture2D;

/**
 * Projects a decal into the scene each frame (data path via <c>SceneRenderParams::decals</c>).
 * Full GPU projection is renderer-dependent; this component collects authoritative decal instances.
 */
class DecalProjectorComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::DecalProjector;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] const SharedPtr<Texture2D>& GetTexture() const noexcept { return texture; }
    [[nodiscard]] const Vector3& GetSize() const noexcept { return size; }
    [[nodiscard]] float GetOpacity() const noexcept { return opacity; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetTexture(const SharedPtr<Texture2D>& tex) noexcept { texture = tex; }
    void SetSize(const Vector3& s) noexcept { size = s; }
    void SetOpacity(float o) noexcept { opacity = o; }
    void SetEnabled(bool e) noexcept { enabled = e; }

private:
    SharedPtr<Texture2D> texture{};
    Vector3 size{1.0F, 1.0F, 0.5F};
    float opacity = 1.0F;
    bool enabled = true;
};

}  // namespace Spark
