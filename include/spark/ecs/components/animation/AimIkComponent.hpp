#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class Skeleton;

/**
 * Upper-body aim / look-at IK: partial spine-chain blend toward a world target or main camera.
 */
class AimIkComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::AimIk;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }
    [[nodiscard]] int UpdatePriority() const noexcept override { return 216; }

    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    SPARK_SCRIPT_BIND(get_weight)
    [[nodiscard]] float GetWeight() const noexcept { return weight; }
    SPARK_SCRIPT_BIND(is_uses_main_camera)
    [[nodiscard]] bool UsesMainCamera() const noexcept { return useMainCamera; }
    SPARK_SCRIPT_BIND(get_target_object)
    [[nodiscard]] GameObject* GetTargetObject() const noexcept { return targetObject; }
    SPARK_SCRIPT_BIND(get_world_target)
    [[nodiscard]] const Vector3& GetWorldTarget() const noexcept { return worldTarget; }
    [[nodiscard]] const Array<std::uint32_t>& GetSpineJointIndices() const noexcept { return spineJointIndices; }

    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(bool value) noexcept { enabled = value; }
    SPARK_SCRIPT_BIND(set_weight)
    void SetWeight(float value) noexcept { weight = value; }
    SPARK_SCRIPT_BIND(set_use_main_camera)
    void SetUseMainCamera(bool value) noexcept { useMainCamera = value; }
    SPARK_SCRIPT_BIND(set_target_object)
    void SetTargetObject(GameObject* object) noexcept { targetObject = object; }
    SPARK_SCRIPT_BIND(set_world_target)
    void SetWorldTarget(const Vector3& target) noexcept { worldTarget = target; useMainCamera = false; }

    /** Resolves spine/head joints from name patterns (case-insensitive substring). */
    SPARK_SCRIPT_BIND(is_configure_from_skeleton)
    bool ConfigureFromSkeleton(const Skeleton& skeleton);

    SPARK_SCRIPT_BIND(set_spine_joint_patterns)
    void SetSpineJointPatterns(const char* const* patterns, std::size_t patternCount);

private:
    bool enabled = true;
    float weight = 0.65F;
    bool useMainCamera = true;
    GameObject* targetObject = nullptr;
    Vector3 worldTarget{Vector3::Zero};
    Array<Utf8String> spinePatterns{};
    Array<std::uint32_t> spineJointIndices{};
};

}  // namespace Spark
