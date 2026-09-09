#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

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

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    [[nodiscard]] float GetWeight() const noexcept { return weight; }
    [[nodiscard]] bool UsesMainCamera() const noexcept { return useMainCamera; }
    [[nodiscard]] GameObject* GetTargetObject() const noexcept { return targetObject; }
    [[nodiscard]] const Vector3& GetWorldTarget() const noexcept { return worldTarget; }
    [[nodiscard]] const Array<std::uint32_t>& GetSpineJointIndices() const noexcept { return spineJointIndices; }

    void SetEnabled(bool value) noexcept { enabled = value; }
    void SetWeight(float value) noexcept { weight = value; }
    void SetUseMainCamera(bool value) noexcept { useMainCamera = value; }
    void SetTargetObject(GameObject* object) noexcept { targetObject = object; }
    void SetWorldTarget(const Vector3& target) noexcept { worldTarget = target; useMainCamera = false; }

    /** Resolves spine/head joints from name patterns (case-insensitive substring). */
    bool ConfigureFromSkeleton(const Skeleton& skeleton);

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
