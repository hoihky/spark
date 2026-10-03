#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

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

    SPARK_SCRIPT_BIND(get_source_object)
    [[nodiscard]] GameObject* GetSourceObject() const noexcept { return sourceObject; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    SPARK_SCRIPT_BIND(get_line_color)
    [[nodiscard]] const Vector3& GetLineColor() const noexcept { return lineColor; }
    SPARK_SCRIPT_BIND(get_bone_thickness)
    [[nodiscard]] float GetBoneThickness() const noexcept { return boneThickness; }
    SPARK_SCRIPT_BIND(get_joint_marker_scale)
    [[nodiscard]] float GetJointMarkerScale() const noexcept { return jointMarkerScale; }

    SPARK_SCRIPT_BIND(set_source_object)
    void SetSourceObject(GameObject* object) noexcept { sourceObject = object; }
    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(bool value) noexcept { enabled = value; }
    SPARK_SCRIPT_BIND(toggle_enabled)
    void ToggleEnabled() noexcept { enabled = !enabled; }
    SPARK_SCRIPT_BIND(set_line_color)
    void SetLineColor(const Vector3& rgb) noexcept { lineColor = rgb; }
    SPARK_SCRIPT_BIND(set_bone_thickness)
    void SetBoneThickness(float thickness) noexcept { boneThickness = thickness; }
    SPARK_SCRIPT_BIND(set_joint_marker_scale)
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
