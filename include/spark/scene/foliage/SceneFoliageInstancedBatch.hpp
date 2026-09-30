#pragma once

#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/**
 * One instanced foliage draw batch in <c>SceneRenderParams</c> (F1).
 * Instance records live in the shared <c>foliageInstances</c> array at [<c>instanceBegin</c>, …).
 */
class SceneFoliageInstancedBatch {
public:
    void SetSourceMesh(const SharedPtr<Mesh>& mesh) noexcept { sourceMesh = mesh; }
    [[nodiscard]] const SharedPtr<Mesh>& GetSourceMesh() const noexcept { return sourceMesh; }

    void SetTextureLayer(std::int32_t layer) noexcept { textureLayer = layer; }
    [[nodiscard]] std::int32_t GetTextureLayer() const noexcept { return textureLayer; }

    void SetAlbedoTint(const Vector3& rgb) noexcept { albedoTint = rgb; }
    [[nodiscard]] Vector3 GetAlbedoTint() const noexcept { return albedoTint; }

    void SetAlphaCutoff(float value) noexcept { alphaCutoff = value; }
    [[nodiscard]] float GetAlphaCutoff() const noexcept { return alphaCutoff; }

    void SetBladeHeight(float meters) noexcept { bladeHeight = meters; }
    [[nodiscard]] float GetBladeHeight() const noexcept { return bladeHeight; }

    void SetWindBendScale(float scale) noexcept { windBendScale = scale; }
    [[nodiscard]] float GetWindBendScale() const noexcept { return windBendScale; }

    void SetInstanceRange(std::uint32_t begin, std::uint32_t count) noexcept {
        instanceBegin = begin;
        instanceCount = count;
    }
    [[nodiscard]] std::uint32_t GetInstanceBegin() const noexcept { return instanceBegin; }
    [[nodiscard]] std::uint32_t GetInstanceCount() const noexcept { return instanceCount; }

private:
    SharedPtr<Mesh> sourceMesh{};
    std::int32_t textureLayer = -1;
    Vector3 albedoTint{0.35F, 0.62F, 0.28F};
    float alphaCutoff = 0.35F;
    float bladeHeight = 0.68F;
    float windBendScale = 0.28F;
    std::uint32_t instanceBegin = 0;
    std::uint32_t instanceCount = 0;
};

}  // namespace Spark
