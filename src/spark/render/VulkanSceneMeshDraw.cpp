#include "spark/render/VulkanSceneMeshDraw.hpp"

namespace Spark {

SceneMeshDrawRange ResolveSceneMeshDrawRange(
        const SceneDrawItem& draw,
        const std::size_t drawIndex,
        const SceneMeshDrawBindings& bindings) noexcept {
    SceneMeshDrawRange out{};
    if (draw.mesh == SceneMeshSlot::UnitCube || draw.mesh == SceneMeshSlot::GroundPlane) {
        out.binding = SceneMeshGeometryBinding::StaticScene;
        switch (draw.mesh) {
        case SceneMeshSlot::UnitCube:
            out.indexCount = bindings.cubeIndexCount;
            out.firstIndex = 0;
            out.vertexOffset = bindings.cubeVertexOffset;
            break;
        case SceneMeshSlot::GroundPlane:
            out.indexCount = bindings.planeIndexCount;
            out.firstIndex = bindings.planeFirstIndex;
            out.vertexOffset = bindings.planeVertexOffset;
            break;
        case SceneMeshSlot::Custom:
        default:
            return out;
        }
        out.drawable = out.indexCount > 0 && bindings.staticVertexBuffer != VK_NULL_HANDLE &&
                       bindings.staticIndexBuffer != VK_NULL_HANDLE;
        return out;
    }

    if (draw.mesh != SceneMeshSlot::Custom) {
        return out;
    }
    if (bindings.customDrawPacked == nullptr || drawIndex >= bindings.customDrawPacked->GetSize()) {
        return out;
    }
    const CustomMeshGpuSlice& pk = (*bindings.customDrawPacked)[drawIndex];
    if (pk.indexCount == 0 || bindings.customVertexBuffer == VK_NULL_HANDLE ||
        bindings.customIndexBuffer == VK_NULL_HANDLE) {
        return out;
    }
    out.binding = SceneMeshGeometryBinding::CustomDynamic;
    out.indexCount = pk.indexCount;
    out.firstIndex = pk.firstIndex;
    out.vertexOffset = pk.vertexOffset;
    out.drawable = true;
    return out;
}

}  // namespace Spark
