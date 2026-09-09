#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"

namespace Spark {

class Skeleton;
class SkinnedMesh;

/**
 * Collects non-fatal warnings for skinned glTF assets after a successful load.
 * Warnings are emitted to stderr by default; callers can also inspect @ref GetWarnings().
 */
class GltfSkinnedLoadValidator {
public:
    explicit GltfSkinnedLoadValidator(bool emitToStderrOnValidate = true) noexcept;

    void ValidateSkinnedLoad(const Skeleton& skeleton, const SkinnedMesh& mesh, const char* sourcePath);

    [[nodiscard]] const Array<Utf8String>& GetWarnings() const noexcept { return warnings; }
    void ClearWarnings() noexcept { warnings.Clear(); }
    void EmitWarnings() const;

private:
    void CheckJointCount(const Skeleton& skeleton, const char* sourcePath);
    void CheckAnimations(const Skeleton& skeleton, const char* sourcePath);
    void CheckClipDurations(const Skeleton& skeleton, const char* sourcePath);
    void CheckInverseBind(const Skeleton& skeleton, const char* sourcePath);
    void CheckMeshJointIndices(const SkinnedMesh& mesh, const Skeleton& skeleton, const char* sourcePath);
    void RecordWarning(const char* sourcePath, const char* message);

    Array<Utf8String> warnings;
    bool emitToStderrOnValidate = true;
};

}  // namespace Spark
