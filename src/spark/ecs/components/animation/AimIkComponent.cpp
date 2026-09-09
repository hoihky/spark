#include "spark/ecs/components/animation/AimIkComponent.hpp"

#include "spark/animation/Skeleton.hpp"

namespace Spark {

void AimIkComponent::SetSpineJointPatterns(const char* const* patterns, const std::size_t patternCount) {
    spinePatterns.Clear();
    if (patterns == nullptr) {
        return;
    }
    for (std::size_t index = 0; index < patternCount; ++index) {
        if (patterns[index] != nullptr && patterns[index][0] != '\0') {
            spinePatterns.PushBack(Utf8String(patterns[index]));
        }
    }
}

bool AimIkComponent::ConfigureFromSkeleton(const Skeleton& skeleton) {
    spineJointIndices.Clear();
    if (spinePatterns.IsEmpty()) {
        const char* defaultPatterns[] = {"spine", "chest", "neck", "head"};
        SetSpineJointPatterns(defaultPatterns, 4);
    }

    for (std::size_t patternIndex = 0; patternIndex < spinePatterns.GetSize(); ++patternIndex) {
        const std::int32_t jointIndex = skeleton.FindJointIndexIfNameContains(spinePatterns[patternIndex].CStr());
        if (jointIndex < 0) {
            continue;
        }
        const std::uint32_t resolved = static_cast<std::uint32_t>(jointIndex);
        bool alreadyPresent = false;
        for (std::size_t existingIndex = 0; existingIndex < spineJointIndices.GetSize(); ++existingIndex) {
            if (spineJointIndices[existingIndex] == resolved) {
                alreadyPresent = true;
                break;
            }
        }
        if (!alreadyPresent) {
            spineJointIndices.PushBack(resolved);
        }
    }
    return !spineJointIndices.IsEmpty();
}

}  // namespace Spark
