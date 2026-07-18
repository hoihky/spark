#include "spark/ecs/components/physics/2d/PolygonCollider2DComponent.hpp"

namespace Spark {

void PolygonCollider2DComponent::SetVertices(const Array<Vector2>& verts) {
    vertices.Clear();
    const std::size_t n = verts.GetSize() < MaxVertices ? verts.GetSize() : MaxVertices;
    for (std::size_t i = 0; i < n; ++i) {
        vertices.PushBack(verts[i]);
    }
}

}  // namespace Spark
