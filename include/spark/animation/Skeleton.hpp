#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Transform.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"

#include <cstdint>

namespace Spark {

class SkinnedMesh;
class Texture2D;

/**
 * Joint hierarchy + inverse bind matrices + sampled glTF animations.
 * Palettes are computed in bind/character space for the vertex shader (world = model * skinnedPos).
 */
class Skeleton {
public:
    static constexpr std::uint32_t MaxJoints = 64;

    Skeleton() = default;

    [[nodiscard]] std::uint32_t GetJointCount() const noexcept { return jointCount; }
    [[nodiscard]] std::uint32_t GetClipCount() const noexcept { return clipNames.GetSize(); }
    [[nodiscard]] float GetClipDuration(std::uint32_t clipIndex) const;
    [[nodiscard]] const Utf8String& GetClipName(std::uint32_t clipIndex) const;
    /** Returns -1 when no clip matches (case-sensitive UTF-8 name). */
    [[nodiscard]] std::int32_t FindClipIndexByName(const char* name) const;
    /** Case-insensitive exact name match; returns -1 when no clip matches. */
    [[nodiscard]] std::int32_t FindClipIndexByNameCaseInsensitive(const char* name) const;
    /** First clip whose name contains <c>substring</c> (case-insensitive); returns -1 when none match. */
    [[nodiscard]] std::int32_t FindClipIndexIfNameContains(const char* substring) const;

    /**
     * Fills the first jointCount entries: skinMatrix[j] = worldJoint[j] * inverseBind[j].
     * clipIndex must be < GetClipCount(); timeSec wraps with fmod (legacy sampling).
     */
    void ComputePalette(std::uint32_t clipIndex, float timeSec, Matrix4* outPalette, std::uint32_t paletteMax)
            const;

    /** Samples clip pose at timeSec (clamped to [0, duration], no wrap). */
    void SampleClipPose(std::uint32_t clipIndex, float timeSec, Array<Transform>& outPose) const;

    /** Builds skin palette from a local joint pose (size must be jointCount). */
    void BuildPaletteFromPose(const Array<Transform>& pose, Matrix4* outPalette, std::uint32_t paletteMax) const;

    /**
     * Copies animation clips from another skeleton (same joint count / hierarchy, e.g. Quaternius mesh + anim pack).
     * Replaces any clips already on this skeleton.
     */
    void AdoptAnimationClipsFrom(const Skeleton& source);

    /**
     * Blends two clip poses (blendB: 0 = clipA, 1 = clipB) then builds the skin palette.
     * Times are clamped per clip duration.
     */
    void ComputeBlendedPalette(
            std::uint32_t clipA,
            float timeA,
            std::uint32_t clipB,
            float timeB,
            float blendB,
            Matrix4* outPalette,
            std::uint32_t paletteMax) const;

    /**
     * Joint world matrix in skeleton space (before owner world transform).
     * Returns false when clip/joint indices are out of range.
     */
    [[nodiscard]] bool TryComputeJointWorldMatrix(
            std::uint32_t clipIndex,
            float timeSec,
            std::uint32_t jointIndex,
            Matrix4& outJointWorld) const;

private:
    static void ComputeJointWorldMatrices(
            std::uint32_t jointCount,
            const Array<std::int32_t>& parents,
            const Array<Matrix4>& locals,
            Array<Matrix4>& outWorld);
    friend bool TryLoadSkinnedCharacterFromGltf(
            const char* path,
            SkinnedMesh& outMesh,
            Skeleton& outSkeleton,
            SharedPtr<Texture2D>* outBaseColor,
            std::uint32_t* outWalkClipIndex,
            Quaternion* outBindUpAlignment,
            float* outBindFacingYawOffset);

    struct Vec3Channel {
        std::uint32_t jointIndex = 0;
        Array<float> times;
        Array<Vector3> values;
    };

    struct QuatChannel {
        std::uint32_t jointIndex = 0;
        Array<float> times;
        Array<Quaternion> values;
    };

    struct AnimationClip {
        float duration = 0.0F;
        Array<Vec3Channel> translations;
        Array<Vec3Channel> scales;
        Array<QuatChannel> rotations;
    };

    std::uint32_t jointCount = 0;
    Array<std::int32_t> jointParents;
    /** fullBindWorld * inv(partialBindWorld); maps joint-only hierarchy world to glTF scene global. */
    Array<Matrix4> jointGlobalPrefix;
    Array<Matrix4> inverseBind;
    Array<Transform> restLocal;
    Array<AnimationClip> clips;
    Array<Utf8String> clipNames;
};

bool TryLoadSkinnedCharacterFromGltf(
        const char* path,
        SkinnedMesh& outMesh,
        Skeleton& outSkeleton,
        SharedPtr<Texture2D>* outBaseColor = nullptr,
        std::uint32_t* outWalkClipIndex = nullptr,
        Quaternion* outBindUpAlignment = nullptr,
        float* outBindFacingYawOffset = nullptr);

}  // namespace Spark
