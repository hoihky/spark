#include "spark/scene/SkinnedMesh.hpp"

#include "spark/core/Utility.hpp"
#include "spark/scene/Mesh.hpp"

#include <cmath>

namespace Spark {

SkinnedMesh::SkinnedMesh(Utf8String meshName) : name(MoveTemp(meshName)) {}

void SkinnedMesh::Clear() noexcept {
    vertices.Clear();
    indices.Clear();
    submeshes.Clear();
}

void SkinnedMesh::AddTriangle(std::uint32_t i0, std::uint32_t i1, std::uint32_t i2) {
    indices.PushBack(i0);
    indices.PushBack(i1);
    indices.PushBack(i2);
}

void SkinnedMesh::AppendRigidMeshAsSkinned(const Mesh& source, SkinnedMesh& outSkinned) {
    const std::uint32_t base = static_cast<std::uint32_t>(outSkinned.GetVertices().GetSize());
    for (std::size_t i = 0; i < source.GetVertices().GetSize(); ++i) {
        const Mesh::Vertex& v = source.GetVertices()[i];
        SkinnedMesh::Vertex sv{};
        sv.position = v.position;
        sv.normal = v.normal;
        sv.texCoord = v.texCoord;
        sv.joints[0] = 0;
        sv.joints[1] = 0;
        sv.joints[2] = 0;
        sv.joints[3] = 0;
        sv.weights[0] = 1.0F;
        sv.weights[1] = 0.0F;
        sv.weights[2] = 0.0F;
        sv.weights[3] = 0.0F;
        outSkinned.GetVertices().PushBack(sv);
    }
    for (std::size_t t = 0; t + 2 < source.GetIndices().GetSize(); t += 3) {
        outSkinned.AddTriangle(
                base + source.GetIndices()[t],
                base + source.GetIndices()[t + 1],
                base + source.GetIndices()[t + 2]);
    }
}

void SkinnedMesh::RecomputeSmoothNormals(SkinnedMesh& mesh) {
    Array<Vertex>& verts = mesh.GetVertices();
    const Array<std::uint32_t>& inds = mesh.GetIndices();
    if (verts.IsEmpty() || inds.IsEmpty() || inds.GetSize() % 3U != 0U) {
        return;
    }
    for (std::size_t i = 0; i < verts.GetSize(); ++i) {
        verts[i].normal = Vector3::Zero;
    }
    for (std::size_t t = 0; t + 2 < inds.GetSize(); t += 3) {
        const std::uint32_t i0 = inds[t];
        const std::uint32_t i1 = inds[t + 1];
        const std::uint32_t i2 = inds[t + 2];
        if (i0 >= verts.GetSize() || i1 >= verts.GetSize() || i2 >= verts.GetSize()) {
            continue;
        }
        const Vector3& p0 = verts[i0].position;
        const Vector3& p1 = verts[i1].position;
        const Vector3& p2 = verts[i2].position;
        const Vector3 e1 = p1 - p0;
        const Vector3 e2 = p2 - p0;
        Vector3 fn = Vector3::Cross(e1, e2);
        const float lenSq = fn.LengthSquared();
        if (lenSq < 1.0e-20F) {
            continue;
        }
        fn = fn * (1.0F / std::sqrt(lenSq));
        verts[i0].normal = verts[i0].normal + fn;
        verts[i1].normal = verts[i1].normal + fn;
        verts[i2].normal = verts[i2].normal + fn;
    }
    for (std::size_t i = 0; i < verts.GetSize(); ++i) {
        Vector3& n = verts[i].normal;
        const float lenSq = n.LengthSquared();
        if (lenSq < 1.0e-20F) {
            n = Vector3{0.0F, 1.0F, 0.0F};
        } else {
            n = n * (1.0F / std::sqrt(lenSq));
        }
    }
}

void SkinnedMesh::RecomputeTangentSpace(SkinnedMesh& mesh) {
    Array<Vertex>& verts = mesh.GetVertices();
    const Array<std::uint32_t>& inds = mesh.GetIndices();
    if (verts.IsEmpty() || inds.IsEmpty() || inds.GetSize() % 3U != 0U) {
        return;
    }

    Array<Vector3> tan1;
    Array<Vector3> tan2;
    tan1.Resize(verts.GetSize());
    tan2.Resize(verts.GetSize());
    for (std::size_t i = 0; i < verts.GetSize(); ++i) {
        tan1[i] = Vector3::Zero;
        tan2[i] = Vector3::Zero;
    }

    for (std::size_t t = 0; t + 2 < inds.GetSize(); t += 3) {
        const std::uint32_t i0 = inds[t];
        const std::uint32_t i1 = inds[t + 1];
        const std::uint32_t i2 = inds[t + 2];
        if (i0 >= verts.GetSize() || i1 >= verts.GetSize() || i2 >= verts.GetSize()) {
            continue;
        }
        const Vector3& p0 = verts[i0].position;
        const Vector3& p1 = verts[i1].position;
        const Vector3& p2 = verts[i2].position;
        const Vector2& w0 = verts[i0].texCoord;
        const Vector2& w1 = verts[i1].texCoord;
        const Vector2& w2 = verts[i2].texCoord;

        const float x1 = p1.x - p0.x;
        const float x2 = p2.x - p0.x;
        const float y1 = p1.y - p0.y;
        const float y2 = p2.y - p0.y;
        const float z1 = p1.z - p0.z;
        const float z2 = p2.z - p0.z;
        const float s1 = w1.x - w0.x;
        const float s2 = w2.x - w0.x;
        const float t1 = w1.y - w0.y;
        const float t2 = w2.y - w0.y;

        const float denom = s1 * t2 - s2 * t1;
        if (std::abs(denom) < 1.0e-10F) {
            continue;
        }
        const float r = 1.0F / denom;
        const Vector3 sdir{
                (t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r};
        const Vector3 tdir{
                (s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r};

        tan1[i0] = tan1[i0] + sdir;
        tan1[i1] = tan1[i1] + sdir;
        tan1[i2] = tan1[i2] + sdir;
        tan2[i0] = tan2[i0] + tdir;
        tan2[i1] = tan2[i1] + tdir;
        tan2[i2] = tan2[i2] + tdir;
    }

    for (std::size_t i = 0; i < verts.GetSize(); ++i) {
        const Vector3& n = verts[i].normal;
        Vector3 t = tan1[i];
        if (t.LengthSquared() < 1.0e-20F) {
            verts[i].tangent = {};
            continue;
        }
        t = t - n * Vector3::Dot(n, t);
        const float tLenSq = t.LengthSquared();
        if (tLenSq < 1.0e-20F) {
            verts[i].tangent = {};
            continue;
        }
        t = t * (1.0F / std::sqrt(tLenSq));
        const float w = (Vector3::Dot(Vector3::Cross(n, t), tan2[i]) < 0.0F) ? -1.0F : 1.0F;
        verts[i].tangent = {t.x, t.y, t.z, w};
    }
}

}  // namespace Spark
