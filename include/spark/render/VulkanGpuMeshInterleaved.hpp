#pragma once

#include "spark/core/Array.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/SkinnedMesh.hpp"

namespace Spark {

namespace VulkanRendererGpu {

/**
 * CPU-side interleaved vertex packing for the scene vertex format (keeps mesh layout out of buffer code).
 */
class VulkanGpuMeshInterleaved {
public:
    static void AppendRigidMeshVertexToInterleaved(const Mesh::Vertex& v, Array<float>& interleaved);
    static void AppendSkinnedVertexToInterleaved(const SkinnedMesh::Vertex& v, Array<float>& interleaved);
};

}  // namespace VulkanRendererGpu
}  // namespace Spark
