#include "spark/scene/assets/gltf/GltfSkinnedLoadValidator.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"

#include <cstdio>
#include <format>

namespace Spark {

namespace {

constexpr float kNearZeroClipDurationSec = 1.0e-3F;
constexpr std::uint32_t kRecommendedJointBudget = 64;

}  // namespace

GltfSkinnedLoadValidator::GltfSkinnedLoadValidator(const bool emitToStderrOnValidate) noexcept
        : emitToStderrOnValidate(emitToStderrOnValidate) {}

void GltfSkinnedLoadValidator::ValidateSkinnedLoad(
        const Skeleton& skeleton,
        const SkinnedMesh& mesh,
        const char* sourcePath) {
    ClearWarnings();
    CheckJointCount(skeleton, sourcePath);
    CheckAnimations(skeleton, sourcePath);
    CheckClipDurations(skeleton, sourcePath);
    CheckInverseBind(skeleton, sourcePath);
    CheckMeshJointIndices(mesh, skeleton, sourcePath);
    if (emitToStderrOnValidate) {
        EmitWarnings();
    }
}

void GltfSkinnedLoadValidator::EmitWarnings() const {
    for (std::size_t i = 0; i < warnings.GetSize(); ++i) {
        std::fprintf(stderr, "%s\n", warnings[i].CStr());
    }
}

void GltfSkinnedLoadValidator::CheckJointCount(const Skeleton& skeleton, const char* sourcePath) {
    const std::uint32_t jointCount = skeleton.GetJointCount();
    if (jointCount == 0) {
        RecordWarning(sourcePath, "skinned glTF has no joints");
        return;
    }
    if (jointCount >= Skeleton::MaxJoints) {
        RecordWarning(
                sourcePath,
                std::format(
                        "joint count {} is at Skeleton::MaxJoints ({}) — reduce joints or raise the limit",
                        jointCount,
                        Skeleton::MaxJoints)
                        .c_str());
    } else if (jointCount > kRecommendedJointBudget) {
        RecordWarning(
                sourcePath,
                std::format(
                        "joint count {} exceeds recommended budget ({}) for GPU skinning",
                        jointCount,
                        kRecommendedJointBudget)
                        .c_str());
    }
}

void GltfSkinnedLoadValidator::CheckAnimations(const Skeleton& skeleton, const char* sourcePath) {
    if (skeleton.GetClipCount() == 0) {
        RecordWarning(sourcePath, "skinned glTF has no animation clips");
    }
}

void GltfSkinnedLoadValidator::CheckClipDurations(const Skeleton& skeleton, const char* sourcePath) {
    for (std::uint32_t clipIndex = 0; clipIndex < skeleton.GetClipCount(); ++clipIndex) {
        const float duration = skeleton.GetClipDuration(clipIndex);
        if (duration <= kNearZeroClipDurationSec) {
            RecordWarning(
                    sourcePath,
                    std::format(
                            "animation clip \"{}\" has near-zero duration ({:.6f}s)",
                            skeleton.GetClipName(clipIndex).CStr(),
                            duration)
                            .c_str());
        }
    }
}

void GltfSkinnedLoadValidator::CheckInverseBind(const Skeleton& skeleton, const char* sourcePath) {
    if (!skeleton.WasGltfInverseBindProvided()) {
        RecordWarning(sourcePath, "glTF skin is missing inverse_bind_matrices — skinning may be incorrect");
        return;
    }
    if (!skeleton.HasValidInverseBindData()) {
        RecordWarning(sourcePath, "one or more inverse bind matrices are not invertible");
    }
}

void GltfSkinnedLoadValidator::CheckMeshJointIndices(
        const SkinnedMesh& mesh,
        const Skeleton& skeleton,
        const char* sourcePath) {
    const std::uint32_t jointCount = skeleton.GetJointCount();
    if (jointCount == 0) {
        return;
    }
    const auto& vertices = mesh.GetVertices();
    for (std::size_t vi = 0; vi < vertices.GetSize(); ++vi) {
        for (int wi = 0; wi < 4; ++wi) {
            if (vertices[vi].weights[wi] <= 0.0F) {
                continue;
            }
            if (vertices[vi].joints[wi] >= jointCount) {
                RecordWarning(
                        sourcePath,
                        std::format(
                                "vertex {} references joint {} but skeleton only has {} joints",
                                vi,
                                vertices[vi].joints[wi],
                                jointCount)
                                .c_str());
                return;
            }
        }
    }
}

void GltfSkinnedLoadValidator::RecordWarning(const char* sourcePath, const char* message) {
    const char* pathLabel = (sourcePath != nullptr && sourcePath[0] != '\0') ? sourcePath : "<unknown>";
    Utf8String line(std::format("Spark: LoadSkinnedGltf warning [{}]: {}", pathLabel, message).c_str());
    for (std::size_t i = 0; i < warnings.GetSize(); ++i) {
        if (warnings[i] == line) {
            return;
        }
    }
    warnings.PushBack(MoveTemp(line));
}

}  // namespace Spark
