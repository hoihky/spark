#include "spark/scene/foliage/GrassBladeMesh.hpp"

#include "spark/math/Vector3.hpp"

namespace Spark {

namespace {

void AddCrossedQuad(
        Array<Mesh::Vertex>& vertices,
        Array<std::uint32_t>& indices,
        const float axisX,
        const float axisZ,
        const float halfWidth,
        const float height) {
    const std::uint32_t base = static_cast<std::uint32_t>(vertices.GetSize());
    const Vector3 normal = Vector3{axisX, 0.0F, axisZ}.Normalized();

    auto corner = [&](float along, float y, float u, float v) {
        Mesh::Vertex vertex{};
        vertex.position = {axisX * along, y, axisZ * along};
        vertex.normal = normal;
        vertex.texCoord = {u, v};
        vertices.PushBack(vertex);
    };

    corner(-halfWidth, 0.0F, 0.0F, 1.0F);
    corner(halfWidth, 0.0F, 1.0F, 1.0F);
    corner(halfWidth, height, 1.0F, 0.0F);
    corner(-halfWidth, height, 0.0F, 0.0F);

    indices.PushBack(base + 0U);
    indices.PushBack(base + 1U);
    indices.PushBack(base + 2U);
    indices.PushBack(base + 0U);
    indices.PushBack(base + 2U);
    indices.PushBack(base + 3U);
}

}  // namespace

SharedPtr<Mesh> GrassBladeMesh::CreateSharedBladeMesh() {
    SharedPtr<Mesh> mesh = MakeShared<Mesh>(Utf8String("GrassBladeCross"));
    Array<Mesh::Vertex>& vertices = mesh->GetVertices();
    Array<std::uint32_t>& indices = mesh->GetIndices();
    vertices.Clear();
    indices.Clear();

    const float halfWidth = kDefaultBladeWidth * 0.5F;
    const float height = kDefaultBladeHeight;
    const float sink = 0.02F;

    AddCrossedQuad(vertices, indices, 1.0F, 0.0F, halfWidth, height);
    AddCrossedQuad(vertices, indices, 0.0F, 1.0F, halfWidth, height);

    for (std::size_t i = 0; i < vertices.GetSize(); ++i) {
        if (vertices[i].position.y <= 0.0F) {
            vertices[i].position.y = -sink;
        }
    }

    mesh->NotifyGeometryChanged();
    return mesh;
}

}  // namespace Spark
