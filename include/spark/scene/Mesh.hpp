#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/MeshSubmesh.hpp"

#include <cstdint>

namespace Spark {

class Texture2D;
class GltfMaterial;
using GltfMaterialDesc = GltfMaterial;

/**
 * CPU-side mesh (vertices + indices). Shared across GameObjects / MeshComponents via SharedPtr.
 * GPU upload is renderer-specific and stays outside this type.
 */
class Mesh {
public:
    struct Vertex {
        Vector3 position{Vector3::Zero};
        Vector3 normal{Vector3::UnitZ};
        Vector2 texCoord{Vector2::Zero};
    };

    Mesh() = default;
    explicit Mesh(Utf8String meshName);

    [[nodiscard]] Utf8String& GetName() noexcept { return name; }
    [[nodiscard]] const Utf8String& GetName() const noexcept { return name; }

    [[nodiscard]] Array<Vertex>& GetVertices() noexcept { return vertices; }
    [[nodiscard]] const Array<Vertex>& GetVertices() const noexcept { return vertices; }

    [[nodiscard]] Array<std::uint32_t>& GetIndices() noexcept { return indices; }
    [[nodiscard]] const Array<std::uint32_t>& GetIndices() const noexcept { return indices; }

    [[nodiscard]] Array<MeshSubmesh>& GetSubmeshes() noexcept { return submeshes; }
    [[nodiscard]] const Array<MeshSubmesh>& GetSubmeshes() const noexcept { return submeshes; }

    void Clear() noexcept;

    void AddVertex(const Vertex& v);
    void AddTriangle(std::uint32_t i0, std::uint32_t i1, std::uint32_t i2);

    /** Axis-aligned quad in XY at z=0, w x h, two triangles; normals +Z. */
    static Mesh CreateQuad(float width, float height);

    /** Unit triangle in XY at z=0. */
    static Mesh CreateTriangle();

    /** Cube centered at origin, axis-aligned, 2 unit edge length (-1..1). */
    static Mesh CreateUnitCube();

    /** Horizontal XZ quad at y=0, normal +Y; corners at ±halfExtent in X and Z. */
    static Mesh CreateGroundPlane(float halfExtent, float worldUnitsPerTextureRepeat = -1.0F);

    /** Upper hemisphere (y >= 0), radius on XZ equator; outward normals (viewed from inside with cull off). */
    static Mesh CreateSkyDome(float radius, int latitudeSegments, int longitudeSegments);

    /** Full sphere; same conventions as CreateSkyDome — replaces a cube skybox to avoid face-edge flicker. */
    static Mesh CreateSkySphere(float radius, int latitudeSegments, int longitudeSegments);

    /** Unit-ish XY quad in z=0 plane, normal +Z — scale/rotate with transform (e.g. camera-facing billboard). */
    static Mesh CreateSkyBillboardPlane(float halfWidth, float halfHeight);

    /** Minimal “toy car” slab (one box, bottom on y=0) for demos without glTF assets. */
    static Mesh CreateSimpleCar();

    /**
     * Loads positions, normals, UVs and triangulated faces from a minimal OBJ subset.
     * Supports `v`, `vn`, `vt`, `f` (v, v/vt, v//vn, v/vt/vn). Returns false on I/O or parse failure.
     */
    [[nodiscard]] static bool TryLoadFromObj(const char* path, Mesh& outMesh);

    /**
     * Loads the default scene from a .glb/.gltf file into one indexed mesh (triangles only).
     * Bakes node transforms into vertices. When <c>outMaterials</c> is set, loads all glTF materials.
     * When only <c>outMaterial</c> is set, fills the primary (first) material for backward compatibility.
     */
    [[nodiscard]] static bool TryLoadFromGltf(
            const char* path,
            Mesh& outMesh,
            SharedPtr<Texture2D>* outBaseColor = nullptr,
            GltfMaterialDesc* outMaterial = nullptr,
            Array<GltfMaterialDesc>* outMaterials = nullptr);

    /** Axis-aligned bounds from vertex positions; false if there are no vertices. */
    [[nodiscard]] bool TryComputeAxisAlignedBounds(Vector3& outMin, Vector3& outMax) const noexcept;

private:
    Utf8String name;
    Array<Vertex> vertices;
    Array<std::uint32_t> indices;
    Array<MeshSubmesh> submeshes;
};

}  // namespace Spark
