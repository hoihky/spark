#pragma once

#include <cstdint>

namespace Spark {

class Skeleton;

/**
 * Resolves which skeleton joint carries authored root translation (hips / pelvis / root).
 * Strategy interface — swap implementations without changing <c>RootMotionComponent</c>.
 */
class IRootMotionJointResolver {
public:
    virtual ~IRootMotionJointResolver() = default;

    [[nodiscard]] virtual std::uint32_t ResolveJointIndex(const Skeleton& skeleton) const = 0;
};

/** Always returns a fixed joint index (clamped to skeleton joint count). */
class FixedRootMotionJointResolver final : public IRootMotionJointResolver {
public:
    explicit FixedRootMotionJointResolver(const std::uint32_t jointIndexIn = 0) noexcept
            : jointIndex(jointIndexIn) {}

    void SetJointIndex(const std::uint32_t jointIndexIn) noexcept { jointIndex = jointIndexIn; }
    [[nodiscard]] std::uint32_t GetJointIndex() const noexcept { return jointIndex; }

    [[nodiscard]] std::uint32_t ResolveJointIndex(const Skeleton& skeleton) const override;

private:
    std::uint32_t jointIndex = 0;
};

/** Tries common hip/root name substrings in order; falls back to joint 0. */
class PatternRootMotionJointResolver final : public IRootMotionJointResolver {
public:
    [[nodiscard]] std::uint32_t ResolveJointIndex(const Skeleton& skeleton) const override;
};

}  // namespace Spark
