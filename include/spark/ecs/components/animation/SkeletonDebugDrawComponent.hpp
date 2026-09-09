#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class GameObject;

/**
 * Dev overlay that draws emissive bone segments for a skinned character.
 * Call @ref AppendSceneDraws from the render pass (not from @ref OnUpdate).
 */
class SkeletonDebugDrawComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SkeletonDebugDraw;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] GameObject* GetSourceObject() const noexcept { return sourceObject; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    [[nodiscard]] const Vector3& GetLineColor() const noexcept { return lineColor; }
    [[nodiscard]] float GetBoneThickness() const noexcept { return boneThickness; }
    [[nodiscard]] float GetJointMarkerScale() const noexcept { return jointMarkerScale; }

    void SetSourceObject(GameObject* object) noexcept { sourceObject = object; }
    void SetEnabled(bool value) noexcept { enabled = value; }
    void ToggleEnabled() noexcept { enabled = !enabled; }
    void SetLineColor(const Vector3& rgb) noexcept { lineColor = rgb; }
    void SetBoneThickness(float thickness) noexcept { boneThickness = thickness; }
    void SetJointMarkerScale(float scale) noexcept { jointMarkerScale = scale; }

    void AppendSceneDraws(Array<SceneDrawItem>& out) const;

private:
    void AppendBoneSegment(const Vector3& fromWorld, const Vector3& toWorld, Array<SceneDrawItem>& out) const;
    void AppendJointMarker(const Vector3& positionWorld, Array<SceneDrawItem>& out) const;

    GameObject* sourceObject = nullptr;
    bool enabled = false;
    Vector3 lineColor{0.18F, 0.95F, 0.42F};
    float boneThickness = 0.006F;
    float jointMarkerScale = 0.018F;
};

}  // namespace Spark
