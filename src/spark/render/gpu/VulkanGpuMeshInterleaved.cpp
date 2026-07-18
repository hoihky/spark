#include "spark/render/gpu/VulkanGpuMeshInterleaved.hpp"

#include "spark/render/scene/VulkanSceneVertexLayout.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/SkinnedMesh.hpp"

#include <bit>

namespace Spark {
namespace VulkanRendererGpu {

void VulkanGpuMeshInterleaved::AppendRigidMeshVertexToInterleaved(const Mesh::Vertex& v, Array<float>& interleaved) {
    interleaved.PushBack(v.position.x);
    interleaved.PushBack(v.position.y);
    interleaved.PushBack(v.position.z);
    interleaved.PushBack(v.normal.x);
    interleaved.PushBack(v.normal.y);
    interleaved.PushBack(v.normal.z);
    interleaved.PushBack(v.texCoord.x);
    interleaved.PushBack(v.texCoord.y);
    interleaved.PushBack(0.0F);
    interleaved.PushBack(0.0F);
    interleaved.PushBack(0.0F);
    interleaved.PushBack(0.0F);
    const float z = std::bit_cast<float>(0u);
    interleaved.PushBack(z);
    interleaved.PushBack(z);
    interleaved.PushBack(z);
    interleaved.PushBack(z);
    interleaved.PushBack(1.0F);
    interleaved.PushBack(0.0F);
    interleaved.PushBack(0.0F);
    interleaved.PushBack(0.0F);
}

void VulkanGpuMeshInterleaved::AppendSkinnedVertexToInterleaved(
        const SkinnedMesh::Vertex& v,
        Array<float>& interleaved) {
    interleaved.PushBack(v.position.x);
    interleaved.PushBack(v.position.y);
    interleaved.PushBack(v.position.z);
    interleaved.PushBack(v.normal.x);
    interleaved.PushBack(v.normal.y);
    interleaved.PushBack(v.normal.z);
    interleaved.PushBack(v.texCoord.x);
    interleaved.PushBack(v.texCoord.y);
    interleaved.PushBack(v.tangent.x);
    interleaved.PushBack(v.tangent.y);
    interleaved.PushBack(v.tangent.z);
    interleaved.PushBack(v.tangent.w);
    interleaved.PushBack(std::bit_cast<float>(v.joints[0]));
    interleaved.PushBack(std::bit_cast<float>(v.joints[1]));
    interleaved.PushBack(std::bit_cast<float>(v.joints[2]));
    interleaved.PushBack(std::bit_cast<float>(v.joints[3]));
    interleaved.PushBack(v.weights[0]);
    interleaved.PushBack(v.weights[1]);
    interleaved.PushBack(v.weights[2]);
    interleaved.PushBack(v.weights[3]);
}

}  // namespace VulkanRendererGpu
}  // namespace Spark
