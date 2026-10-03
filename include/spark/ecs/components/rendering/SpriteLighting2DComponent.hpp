#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/sprites2d/SpriteLighting2D.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Optional per-sprite 2D lighting / shading (see SpriteLighting2DMode). Consumed when building SceneSpriteDraw;
 * fragment work happens in sprite.frag. Pair with SpriteComponent on the same GameObject.
 */
class SpriteLighting2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SpriteLighting2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit SpriteLighting2DComponent(
            SpriteLighting2DMode modeIn = SpriteLighting2DMode::None,
            Vector4 param0In = {1.0F, 1.0F, 1.0F, 1.0F},
            Vector4 param1In = {1.0F, 0.0F, 0.0F, 0.0F}) noexcept
            : mode(modeIn), param0(param0In), param1(param1In) {}

    SPARK_SCRIPT_BIND(get_mode)
    [[nodiscard]] SpriteLighting2DMode GetMode() const noexcept { return mode; }
    SPARK_SCRIPT_BIND(get_param0)
    [[nodiscard]] const Vector4& GetParam0() const noexcept { return param0; }
    SPARK_SCRIPT_BIND(get_param1)
    [[nodiscard]] const Vector4& GetParam1() const noexcept { return param1; }

    SPARK_SCRIPT_BIND(set_mode)
    void SetMode(SpriteLighting2DMode m) noexcept { mode = m; }
    SPARK_SCRIPT_BIND(set_param0)
    void SetParam0(const Vector4& v) noexcept { param0 = v; }
    SPARK_SCRIPT_BIND(set_param1)
    void SetParam1(const Vector4& v) noexcept { param1 = v; }

    void SetNormalMap(SharedPtr<Texture2D> texture) noexcept { normalMap = MoveTemp(texture); }
    [[nodiscard]] const SharedPtr<Texture2D>& GetNormalMap() const noexcept { return normalMap; }
    void SetRampMap(SharedPtr<Texture2D> texture) noexcept { rampMap = MoveTemp(texture); }
    [[nodiscard]] const SharedPtr<Texture2D>& GetRampMap() const noexcept { return rampMap; }

    /** When true, normal map UVs follow <c>SpriteComponent</c> atlas UV each frame (animated sprites). */
    SPARK_SCRIPT_BIND(set_sync_normal_uv_with_sprite)
    void SetSyncNormalUvWithSprite(const bool sync) noexcept { syncNormalUvWithSprite = sync; }
    SPARK_SCRIPT_BIND(get_sync_normal_uv_with_sprite)
    [[nodiscard]] bool GetSyncNormalUvWithSprite() const noexcept { return syncNormalUvWithSprite; }
    /** Used when <c>syncNormalUvWithSprite</c> is false (separate normal-atlas region). */
    SPARK_SCRIPT_BIND(set_normal_uv_rect)
    void SetNormalUvRect(const Vector4& rect) noexcept { normalUvRect = rect; }
    SPARK_SCRIPT_BIND(get_normal_uv_rect)
    [[nodiscard]] const Vector4& GetNormalUvRect() const noexcept { return normalUvRect; }

private:
    SpriteLighting2DMode mode = SpriteLighting2DMode::None;
    Vector4 param0{1.0F, 1.0F, 1.0F, 1.0F};
    Vector4 param1{1.0F, 0.0F, 0.0F, 0.0F};
    SharedPtr<Texture2D> normalMap{};
    SharedPtr<Texture2D> rampMap{};
    bool syncNormalUvWithSprite = true;
    Vector4 normalUvRect{0.0F, 0.0F, 1.0F, 1.0F};
};

}  // namespace Spark
