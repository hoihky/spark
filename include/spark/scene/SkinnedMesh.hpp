#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/MeshSubmesh.hpp"

#include <cstdint>

namespace Spark {

class Mesh;
class Texture2D;

/**
 * Mesh with up to 4 bone influences per vertex (glTF JOINTS_0 / WEIGHTS_0).
 * Vertices are stored in bind pose; GPU applies joint palette * inverse bind.
 */
class SkinnedMesh {
public:
    struct Vertex {
        Vector3 position{Vector3::Zero};
        Vector3 normal{Vector3::UnitZ};
        Vector2 texCoord{Vector2::Zero};
        /** xyz = tangent direction, w = bitangent handedness; zero xyz = derivative TBN fallback. */
        Vector4 tangent{};
        std::uint32_t joints[4]{0, 0, 0, 0};
        float weights[4]{1.0F, 0.0F, 0.0F, 0.0F};
    };

    SkinnedMesh() = default;
    explicit SkinnedMesh(Utf8String meshName);

    [[nodiscard]] Utf8String& GetName() noexcept { return name; }
    [[nodiscard]] const Utf8String& GetName() const noexcept { return name; }

    [[nodiscard]] Array<Vertex>& GetVertices() noexcept { return vertices; }
    [[nodiscard]] const Array<Vertex>& GetVertices() const noexcept { return vertices; }

    [[nodiscard]] Array<std::uint32_t>& GetIndices() noexcept { return indices; }
    [[nodiscard]] const Array<std::uint32_t>& GetIndices() const noexcept { return indices; }

    [[nodiscard]] Array<MeshSubmesh>& GetSubmeshes() noexcept { return submeshes; }
    [[nodiscard]] const Array<MeshSubmesh>& GetSubmeshes() const noexcept { return submeshes; }

    void Clear() noexcept;
    void AddTriangle(std::uint32_t i0, std::uint32_t i1, std::uint32_t i2);

    /** Rigid mesh as skinned (single bone 0, full weight) for shared vertex layout. */
    static void AppendRigidMeshAsSkinned(const Mesh& source, SkinnedMesh& outSkinned);

    /** Per-vertex smooth normals from indexed triangles (for glTF without NORMAL attributes). */
    static void RecomputeSmoothNormals(SkinnedMesh& mesh);
    /** MikkTSpace-style tangents from positions, normals, and UVs (when glTF omits TANGENT). */
    static void RecomputeTangentSpace(SkinnedMesh& mesh);

private:
    Utf8String name;
    Array<Vertex> vertices;
    Array<std::uint32_t> indices;
    Array<MeshSubmesh> submeshes;
};

}  // namespace Spark
