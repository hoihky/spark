#include "spark/ecs/components/animation/FootIkComponent.hpp"

#include "spark/animation/Skeleton.hpp"

#include <cctype>
#include <cstring>

namespace Spark {

namespace {

[[nodiscard]] bool IsJointExcluded(
        const Array<std::uint32_t>& excludedEndJoints,
        const std::uint32_t jointIndex) noexcept {
    for (std::size_t index = 0; index < excludedEndJoints.GetSize(); ++index) {
        if (excludedEndJoints[index] == jointIndex) {
            return true;
        }
    }
    return false;
}

}  // namespace

bool FootIkComponent::MatchesJointNamePattern(const char* jointName, const char* pattern) {
    if (pattern == nullptr || pattern[0] == '\0' || jointName == nullptr) {
        return false;
    }
    const std::size_t patternLength = std::strlen(pattern);
    for (const char* cursor = jointName; *cursor != '\0'; ++cursor) {
        bool matched = true;
        for (std::size_t index = 0; index < patternLength; ++index) {
            if (cursor[index] == '\0') {
                matched = false;
                break;
            }
            if (std::tolower(static_cast<unsigned char>(cursor[index]))
                    != std::tolower(static_cast<unsigned char>(pattern[index]))) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return true;
        }
    }
    return false;
}

bool FootIkComponent::IsJointAncestorOf(
        const Skeleton& skeleton,
        const std::uint32_t ancestorJoint,
        const std::uint32_t jointIndex) {
    std::int32_t parentIndex = skeleton.GetJointParent(jointIndex);
    while (parentIndex >= 0) {
        if (static_cast<std::uint32_t>(parentIndex) == ancestorJoint) {
            return true;
        }
        parentIndex = skeleton.GetJointParent(static_cast<std::uint32_t>(parentIndex));
    }
    return false;
}

std::int32_t FootIkComponent::FindJointMatchingPatternOnChainToEnd(
        const Skeleton& skeleton,
        const Utf8String& pattern,
        const std::uint32_t endJoint,
        const bool preferLastMatch) {
    if (pattern.IsEmpty()) {
        return -1;
    }

    std::int32_t matchedJoint = -1;
    for (std::uint32_t jointIndex = 0; jointIndex < skeleton.GetJointCount(); ++jointIndex) {
        if (!IsJointAncestorOf(skeleton, jointIndex, endJoint)) {
            continue;
        }
        if (!MatchesJointNamePattern(skeleton.GetJointName(jointIndex).CStr(), pattern.CStr())) {
            continue;
        }
        matchedJoint = static_cast<std::int32_t>(jointIndex);
        if (!preferLastMatch) {
            return matchedJoint;
        }
    }
    return matchedJoint;
}

float FootIkComponent::ResolvePoleBiasX(const Skeleton& skeleton, const std::uint32_t endJoint) {
    const Utf8String& endName = skeleton.GetJointName(endJoint);
    if (MatchesJointNamePattern(endName.CStr(), "left")) {
        return -0.35F;
    }
    if (MatchesJointNamePattern(endName.CStr(), "right")) {
        return 0.35F;
    }
    return 0.0F;
}

bool FootIkComponent::TryResolveLimb(
        const Skeleton& skeleton,
        const Utf8String& rootPattern,
        const Utf8String& midPattern,
        const Utf8String& endPattern,
        const Array<std::uint32_t>& excludedEndJoints,
        Limb& outLimb) {
    std::int32_t endJoint = skeleton.FindLastJointIndexIfNameContains(endPattern.CStr());
    if (endJoint < 0) {
        endJoint = skeleton.FindJointIndexIfNameContains(endPattern.CStr());
    }
    if (endJoint < 0 || IsJointExcluded(excludedEndJoints, static_cast<std::uint32_t>(endJoint))) {
        return false;
    }
    outLimb.endJoint = static_cast<std::uint32_t>(endJoint);
    outLimb.endPattern = endPattern;

    std::int32_t midJoint = FindJointMatchingPatternOnChainToEnd(skeleton, midPattern, outLimb.endJoint, false);
    outLimb.hasMidJoint = midJoint >= 0;
    outLimb.midJoint = outLimb.hasMidJoint ? static_cast<std::uint32_t>(midJoint) : 0U;

    std::int32_t rootJoint = FindJointMatchingPatternOnChainToEnd(skeleton, rootPattern, outLimb.endJoint, false);
    if (rootJoint < 0) {
        const std::int32_t parentSource = outLimb.hasMidJoint ? midJoint : endJoint;
        rootJoint = skeleton.GetJointParent(static_cast<std::uint32_t>(parentSource));
    }
    if (rootJoint < 0) {
        return false;
    }
    if (outLimb.hasMidJoint && !IsJointAncestorOf(skeleton, static_cast<std::uint32_t>(rootJoint), outLimb.midJoint)) {
        return false;
    }
    if (!IsJointAncestorOf(skeleton, static_cast<std::uint32_t>(rootJoint), outLimb.endJoint)) {
        return false;
    }

    outLimb.rootJoint = static_cast<std::uint32_t>(rootJoint);
    outLimb.poleBiasX = ResolvePoleBiasX(skeleton, outLimb.endJoint);
    return true;
}

void FootIkComponent::SetLeftFootPatterns(
        const char* rootPattern,
        const char* midPattern,
        const char* endPattern) {
    leftRootPattern = Utf8String(rootPattern != nullptr ? rootPattern : "");
    leftMidPattern = Utf8String(midPattern != nullptr ? midPattern : "");
    leftEndPattern = Utf8String(endPattern != nullptr ? endPattern : "");
}

void FootIkComponent::SetRightFootPatterns(
        const char* rootPattern,
        const char* midPattern,
        const char* endPattern) {
    rightRootPattern = Utf8String(rootPattern != nullptr ? rootPattern : "");
    rightMidPattern = Utf8String(midPattern != nullptr ? midPattern : "");
    rightEndPattern = Utf8String(endPattern != nullptr ? endPattern : "");
}

bool FootIkComponent::ConfigureFromSkeleton(const Skeleton& skeleton) {
    limbs.Clear();
    Array<std::uint32_t> excludedEndJoints{};

    Limb leftLimb{};
    if (TryResolveLimb(skeleton, leftRootPattern, leftMidPattern, leftEndPattern, excludedEndJoints, leftLimb)) {
        limbs.PushBack(leftLimb);
        excludedEndJoints.PushBack(leftLimb.endJoint);
    }

    Limb rightLimb{};
    if (TryResolveLimb(skeleton, rightRootPattern, rightMidPattern, rightEndPattern, excludedEndJoints, rightLimb)) {
        limbs.PushBack(rightLimb);
    }
    return !limbs.IsEmpty();
}

}  // namespace Spark
