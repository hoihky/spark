#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/sprites2d/Scene2DComposite.hpp"
#include "spark/scene/render/RenderTexture.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/** Authoring hook for minimap / fog / outline RT targets (see <c>Scene2DComposite.hpp</c>). */
class Scene2DCompositeViewComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Scene2DCompositeView;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(set_feature)
    void SetFeature(const Scene2DCompositeFeature value) noexcept { feature = value; }
    SPARK_SCRIPT_BIND(get_feature)
    [[nodiscard]] Scene2DCompositeFeature GetFeature() const noexcept { return feature; }

    void SetTarget(SharedPtr<RenderTexture> texture) noexcept { target = MoveTemp(texture); }
    [[nodiscard]] const SharedPtr<RenderTexture>& GetTarget() const noexcept { return target; }

    void SetHudTexture(SharedPtr<Texture2D> texture) noexcept { hudTexture = MoveTemp(texture); }
    [[nodiscard]] const SharedPtr<Texture2D>& GetHudTexture() const noexcept { return hudTexture; }

    void SetScreenRect(const float x, const float y, const float w, const float h) noexcept {
        screenX = x;
        screenY = y;
        screenW = w;
        screenH = h;
    }

    void SetWorldCapture(const Vector3& center, const float orthoHalfExtent) noexcept {
        worldCenter = center;
        worldOrthoHalfExtent = orthoHalfExtent;
    }

    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool value) noexcept { enabled = value; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    SPARK_SCRIPT_BIND(build_desc)
    [[nodiscard]] Scene2DCompositeViewDesc BuildDesc() const noexcept;

private:
    Scene2DCompositeFeature feature = Scene2DCompositeFeature::Minimap;
    SharedPtr<RenderTexture> target{};
    SharedPtr<Texture2D> hudTexture{};
    float screenX = 0.82F;
    float screenY = 0.02F;
    float screenW = 0.16F;
    float screenH = 0.16F;
    Vector3 worldCenter{};
    float worldOrthoHalfExtent = 24.0F;
    bool enabled = true;
};

}  // namespace Spark
