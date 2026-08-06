#include "spark/animation/Skeleton.hpp"

#include <cmath>
#include <cstring>

namespace Spark {

float Skeleton::GetClipDuration(std::uint32_t clipIndex) const {
    if (clipIndex >= clips.GetSize()) {
        return 0.0F;
    }
    return clips[clipIndex].duration;
}

const Utf8String& Skeleton::GetClipName(std::uint32_t clipIndex) const {
    static const Utf8String kEmpty{};
    if (clipIndex >= clipNames.GetSize()) {
        return kEmpty;
    }
    return clipNames[clipIndex];
}

namespace {

bool Utf8EqualsCaseInsensitive(const char* a, const char* b) {
    if (a == nullptr || b == nullptr) {
        return a == b;
    }
    while (*a != '\0' && *b != '\0') {
        const unsigned char ca = static_cast<unsigned char>(*a);
        const unsigned char cb = static_cast<unsigned char>(*b);
        const unsigned char la = (ca >= 'A' && ca <= 'Z') ? static_cast<unsigned char>(ca + ('a' - 'A')) : ca;
        const unsigned char lb = (cb >= 'A' && cb <= 'Z') ? static_cast<unsigned char>(cb + ('a' - 'A')) : cb;
        if (la != lb) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

bool Utf8ContainsCaseInsensitive(const char* haystack, const char* needle) {
    if (needle == nullptr || needle[0] == '\0') {
        return false;
    }
    if (haystack == nullptr) {
        return false;
    }
    const std::size_t needleLen = std::strlen(needle);
    for (const char* p = haystack; *p != '\0'; ++p) {
        const char* h = p;
        const char* n = needle;
        bool match = true;
        for (std::size_t i = 0; i < needleLen; ++i) {
            if (h[i] == '\0') {
                match = false;
                break;
            }
            const unsigned char ca = static_cast<unsigned char>(h[i]);
            const unsigned char cn = static_cast<unsigned char>(n[i]);
            const unsigned char la = (ca >= 'A' && ca <= 'Z') ? static_cast<unsigned char>(ca + ('a' - 'A')) : ca;
            const unsigned char ln = (cn >= 'A' && cn <= 'Z') ? static_cast<unsigned char>(cn + ('a' - 'A')) : cn;
            if (la != ln) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}

}  // namespace

std::int32_t Skeleton::FindClipIndexByName(const char* name) const {
    if (name == nullptr || name[0] == '\0') {
        return -1;
    }
    for (std::size_t i = 0; i < clipNames.GetSize(); ++i) {
        if (std::strcmp(clipNames[i].CStr(), name) == 0) {
            return static_cast<std::int32_t>(i);
        }
    }
    return -1;
}

std::int32_t Skeleton::FindClipIndexByNameCaseInsensitive(const char* name) const {
    if (name == nullptr || name[0] == '\0') {
        return -1;
    }
    for (std::size_t i = 0; i < clipNames.GetSize(); ++i) {
        if (Utf8EqualsCaseInsensitive(clipNames[i].CStr(), name)) {
            return static_cast<std::int32_t>(i);
        }
    }
    return -1;
}

std::int32_t Skeleton::FindClipIndexIfNameContains(const char* substring) const {
    if (substring == nullptr || substring[0] == '\0') {
        return -1;
    }
    for (std::size_t i = 0; i < clipNames.GetSize(); ++i) {
        if (Utf8ContainsCaseInsensitive(clipNames[i].CStr(), substring)) {
            return static_cast<std::int32_t>(i);
        }
    }
    return -1;
}

namespace {

std::size_t FindSegment(const Array<float>& times, float t) {
    if (times.IsEmpty()) {
        return 0;
    }
    if (t <= times[0]) {
        return 0;
    }
    const std::size_t n = times.GetSize();
    if (t >= times[n - 1]) {
        return n - 2;
    }
    for (std::size_t i = 0; i + 1 < n; ++i) {
        if (t >= times[i] && t <= times[i + 1]) {
            return i;
        }
    }
    return n - 2;
}

Vector3 SampleVec3Linear(const Array<float>& times, const Array<Vector3>& vals, float t) {
    if (times.IsEmpty() || vals.GetSize() != times.GetSize()) {
        return Vector3::Zero;
    }
    if (vals.GetSize() == 1) {
        return vals[0];
    }
    const std::size_t i = FindSegment(times, t);
    const std::size_t i1 = i + 1 < times.GetSize() ? i + 1 : i;
    const float t0 = times[i];
    const float t1 = times[i1];
    const float u = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0F;
    const Vector3& a = vals[i];
    const Vector3& b = vals[i1];
    return {a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u, a.z + (b.z - a.z) * u};
}

Quaternion SampleQuatSlerp(const Array<float>& times, const Array<Quaternion>& vals, float t) {
    if (times.IsEmpty() || vals.GetSize() != times.GetSize()) {
        return Quaternion::Identity;
    }
    if (vals.GetSize() == 1) {
        return vals[0];
    }
    const std::size_t i = FindSegment(times, t);
    const std::size_t i1 = i + 1 < times.GetSize() ? i + 1 : i;
    const float t0 = times[i];
    const float t1 = times[i1];
    const float u = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0F;
    return Quaternion::Slerp(vals[i], vals[i1], u);
}

void SolveWorldMatrix(
        std::uint32_t j,
        const Array<std::int32_t>& parents,
        const Array<Matrix4>& locals,
        Array<Matrix4>& world,
        Array<bool>& done) {
    if (done[j]) {
        return;
    }
    const std::int32_t p = parents[j];
    if (p < 0) {
        world[j] = locals[j];
        done[j] = true;
        return;
    }
    SolveWorldMatrix(static_cast<std::uint32_t>(p), parents, locals, world, done);
    world[j] = world[static_cast<std::uint32_t>(p)] * locals[j];
    done[j] = true;
}

}  // namespace

void Skeleton::ComputeJointWorldMatrices(
        std::uint32_t jc,
        const Array<std::int32_t>& parents,
        const Array<Matrix4>& locals,
        Array<Matrix4>& outWorld) {
    if (jc == 0) {
        return;
    }
    outWorld.Resize(jc);
    Array<bool> done;
    done.Resize(jc);
    for (std::uint32_t j = 0; j < jc; ++j) {
        done[j] = false;
    }
    for (std::uint32_t j = 0; j < jc; ++j) {
        SolveWorldMatrix(j, parents, locals, outWorld, done);
    }
}

void Skeleton::SampleClipPose(std::uint32_t clipIndex, float timeSec, Array<Transform>& outPose) const {
    if (jointCount == 0 || clipIndex >= clips.GetSize()) {
        outPose.Clear();
        return;
    }
    const AnimationClip& clip = clips[clipIndex];
    const float dur = clip.duration > 0.0F ? clip.duration : 1.0e-4F;
    float t = timeSec;
    if (t < 0.0F) {
        t = 0.0F;
    }
    if (t > dur) {
        t = dur;
    }

    outPose.Resize(jointCount);
    for (std::uint32_t j = 0; j < jointCount; ++j) {
        outPose[j] = restLocal[j];
    }

    for (std::size_t ci = 0; ci < clip.translations.GetSize(); ++ci) {
        const Vec3Channel& ch = clip.translations[ci];
        if (ch.jointIndex < jointCount) {
            outPose[ch.jointIndex].translation = SampleVec3Linear(ch.times, ch.values, t);
        }
    }
    for (std::size_t ci = 0; ci < clip.scales.GetSize(); ++ci) {
        const Vec3Channel& ch = clip.scales[ci];
        if (ch.jointIndex < jointCount) {
            outPose[ch.jointIndex].scale = SampleVec3Linear(ch.times, ch.values, t);
        }
    }
    for (std::size_t ci = 0; ci < clip.rotations.GetSize(); ++ci) {
        const QuatChannel& ch = clip.rotations[ci];
        if (ch.jointIndex < jointCount) {
            outPose[ch.jointIndex].rotation = SampleQuatSlerp(ch.times, ch.values, t);
        }
    }
}

void Skeleton::BuildPaletteFromPose(
        const Array<Transform>& pose,
        Matrix4* outPalette,
        std::uint32_t paletteMax) const {
    if (outPalette == nullptr || jointCount == 0 || paletteMax < jointCount || pose.GetSize() < jointCount) {
        return;
    }

    Array<Matrix4> locals;
    locals.Resize(jointCount);
    for (std::uint32_t j = 0; j < jointCount; ++j) {
        locals[j] = pose[j].ToMatrix4();
    }

    Array<Matrix4> world;
    ComputeJointWorldMatrices(jointCount, jointParents, locals, world);

    const bool usePrefix = jointGlobalPrefix.GetSize() == jointCount;
    for (std::uint32_t j = 0; j < jointCount; ++j) {
        const Matrix4& pre = usePrefix ? jointGlobalPrefix[j] : Matrix4::Identity;
        outPalette[j] = pre * world[j] * inverseBind[j];
    }
}

void Skeleton::AdoptAnimationClipsFrom(const Skeleton& source) {
    if (source.jointCount != jointCount || jointCount == 0) {
        return;
    }
    clips.Clear();
    clipNames.Clear();
    for (std::size_t i = 0; i < source.clips.GetSize(); ++i) {
        clips.PushBack(source.clips[i]);
        clipNames.PushBack(source.clipNames[i]);
    }
}

void Skeleton::ComputeBlendedPalette(
        std::uint32_t clipA,
        float timeA,
        std::uint32_t clipB,
        float timeB,
        float blendB,
        Matrix4* outPalette,
        std::uint32_t paletteMax) const {
    if (outPalette == nullptr || jointCount == 0 || paletteMax < jointCount) {
        return;
    }
    const float w = (blendB < 0.0F) ? 0.0F : (blendB > 1.0F ? 1.0F : blendB);
    if (w <= 1.0e-6F) {
        Array<Transform> poseA;
        SampleClipPose(clipA, timeA, poseA);
        BuildPaletteFromPose(poseA, outPalette, paletteMax);
        return;
    }
    if (w >= 1.0F - 1.0e-6F) {
        Array<Transform> poseB;
        SampleClipPose(clipB, timeB, poseB);
        BuildPaletteFromPose(poseB, outPalette, paletteMax);
        return;
    }
    const float wA = 1.0F - w;
    Array<Transform> poseA;
    Array<Transform> poseB;
    SampleClipPose(clipA, timeA, poseA);
    SampleClipPose(clipB, timeB, poseB);
    if (poseA.GetSize() < jointCount || poseB.GetSize() < jointCount) {
        return;
    }
    Array<Transform> blended;
    blended.Resize(jointCount);
    for (std::uint32_t j = 0; j < jointCount; ++j) {
        const Transform& a = poseA[j];
        const Transform& b = poseB[j];
        blended[j].translation = {
                a.translation.x * wA + b.translation.x * w,
                a.translation.y * wA + b.translation.y * w,
                a.translation.z * wA + b.translation.z * w};
        blended[j].scale = {
                a.scale.x * wA + b.scale.x * w,
                a.scale.y * wA + b.scale.y * w,
                a.scale.z * wA + b.scale.z * w};
        blended[j].rotation = Quaternion::Slerp(a.rotation, b.rotation, w);
    }
    BuildPaletteFromPose(blended, outPalette, paletteMax);
}

void Skeleton::ComputePalette(
        std::uint32_t clipIndex,
        float timeSec,
        Matrix4* outPalette,
        std::uint32_t paletteMax) const {
    if (outPalette == nullptr || jointCount == 0 || paletteMax < jointCount) {
        return;
    }
    if (clipIndex >= clips.GetSize()) {
        return;
    }
    const AnimationClip& clip = clips[clipIndex];
    const float dur = clip.duration > 0.0F ? clip.duration : 1.0e-4F;
    float t = timeSec;
    if (t < 0.0F) {
        t = 0.0F;
    }
    if (t > dur) {
        t = std::fmod(t, dur);
        if (t < 0.0F) {
            t += dur;
        }
    }
    Array<Transform> pose;
    SampleClipPose(clipIndex, t, pose);
    BuildPaletteFromPose(pose, outPalette, paletteMax);
}

bool Skeleton::TryComputeJointWorldMatrix(
        const std::uint32_t clipIndex,
        const float timeSec,
        const std::uint32_t jointIndex,
        Matrix4& outJointWorld) const {
    if (jointIndex >= jointCount || clipIndex >= clips.GetSize()) {
        return false;
    }
    Array<Transform> pose;
    SampleClipPose(clipIndex, timeSec, pose);
    if (pose.GetSize() < jointCount) {
        return false;
    }
    Array<Matrix4> locals;
    locals.Resize(jointCount);
    for (std::uint32_t j = 0; j < jointCount; ++j) {
        locals[j] = pose[j].ToMatrix4();
    }
    Array<Matrix4> world;
    ComputeJointWorldMatrices(jointCount, jointParents, locals, world);
    const bool usePrefix = jointGlobalPrefix.GetSize() == jointCount;
    const Matrix4& pre = usePrefix ? jointGlobalPrefix[jointIndex] : Matrix4::Identity;
    outJointWorld = pre * world[jointIndex];
    return true;
}

}  // namespace Spark
