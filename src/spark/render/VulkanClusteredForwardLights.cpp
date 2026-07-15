#include "spark/render/VulkanClusteredForwardLights.hpp"

#include "spark/render/VulkanClusteredLightGpu.hpp"
#include "spark/render/VulkanRendererGpu.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace Spark {

namespace {

constexpr float kClusterDepthNearMin = 0.05F;

int FindDepthSlice(const float dist, const float nearPlane, const float farPlane, const int gridZ) {
    if (gridZ <= 1) {
        return 0;
    }
    if (dist <= nearPlane) {
        return 0;
    }
    if (dist >= farPlane) {
        return gridZ - 1;
    }
    const float ratio = farPlane / nearPlane;
    if (ratio <= 1.0F + 1e-6F) {
        const float t = (dist - nearPlane) / (farPlane - nearPlane);
        return std::clamp(static_cast<int>(std::floor(t * static_cast<float>(gridZ))), 0, gridZ - 1);
    }
    const float t = std::log(dist / nearPlane) / std::log(ratio);
    return std::clamp(static_cast<int>(std::floor(t * static_cast<float>(gridZ))), 0, gridZ - 1);
}

void NdcToScreenTile(
        const float ndcX,
        const float ndcY,
        const float ndcRadius,
        const int gridX,
        const int gridY,
        int& outMinX,
        int& outMaxX,
        int& outMinY,
        int& outMaxY) {
    // PerspectiveVulkan flips clip Y; match gl_FragCoord (y up) and sparkClusterIndex in GLSL.
    const float screenX = (ndcX + 1.0F) * 0.5F;
    const float screenY = (1.0F - ndcY) * 0.5F;
    const float screenR = ndcRadius * 0.5F;

    outMinX = std::clamp(static_cast<int>(std::floor((screenX - screenR) * static_cast<float>(gridX))), 0, gridX - 1);
    outMaxX = std::clamp(static_cast<int>(std::floor((screenX + screenR) * static_cast<float>(gridX))), 0, gridX - 1);
    outMinY = std::clamp(static_cast<int>(std::floor((screenY - screenR) * static_cast<float>(gridY))), 0, gridY - 1);
    outMaxY = std::clamp(static_cast<int>(std::floor((screenY + screenR) * static_cast<float>(gridY))), 0, gridY - 1);

    // Conservative halo so boundary pixels do not miss influencing lights.
    outMinX = std::max(0, outMinX - 1);
    outMaxX = std::min(gridX - 1, outMaxX + 1);
    outMinY = std::max(0, outMinY - 1);
    outMaxY = std::min(gridY - 1, outMaxY + 1);
}

std::uint32_t ClusterIndex(const int ix, const int iy, const int iz) {
    return static_cast<std::uint32_t>(ix + iy * static_cast<int>(kClusterGridX) +
                                      iz * static_cast<int>(kClusterGridX * kClusterGridY));
}

struct ClusterTileRange {
    int minX = 0;
    int maxX = 0;
    int minY = 0;
    int maxY = 0;
    int minZ = 0;
    int maxZ = 0;
};

bool ComputeClusterTileRange(
        const Matrix4& viewProj,
        const Vector3& camPos,
        const Vector3& pos,
        const float range,
        const float nearPlane,
        const float farPlane,
        ClusterTileRange& out) {
    const Vector4 clip0 = viewProj * Vector4{pos.x, pos.y, pos.z, 1.0F};
    if (clip0.w <= 1e-4F) {
        return false;
    }
    const float invW0 = 1.0F / clip0.w;
    const float ndcX = clip0.x * invW0;
    const float ndcY = clip0.y * invW0;

    Vector3 toLight = pos - camPos;
    const float dist = toLight.Length();
    if (dist < 1e-4F) {
        return false;
    }
    toLight = toLight * (1.0F / dist);

    Vector3 worldUp{0.0F, 1.0F, 0.0F};
    Vector3 right = Vector3::Cross(toLight, worldUp);
    if (right.LengthSquared() < 1e-6F) {
        right = Vector3{1.0F, 0.0F, 0.0F};
    } else {
        right = right.Normalized();
    }
    const Vector3 up = Vector3::Cross(right, toLight).Normalized();

    auto ndcDelta = [&](const Vector3& offset) {
        const Vector4 clip1 = viewProj * Vector4{pos.x + offset.x, pos.y + offset.y, pos.z + offset.z, 1.0F};
        if (clip1.w <= 1e-4F) {
            return Vector3{1.0F, 1.0F, 0.0F};
        }
        const float invW1 = 1.0F / clip1.w;
        return Vector3{
                std::abs(clip1.x * invW1 - ndcX),
                std::abs(clip1.y * invW1 - ndcY),
                0.0F};
    };

    const Vector3 dR = ndcDelta(right * range);
    const Vector3 dU = ndcDelta(up * range);
    const float ndcR = std::max({dR.x, dR.y, dU.x, dU.y, range / std::max(dist, 0.1F) * 0.25F});

    const int gridX = static_cast<int>(kClusterGridX);
    const int gridY = static_cast<int>(kClusterGridY);
    const int gridZ = static_cast<int>(kClusterGridZ);

    NdcToScreenTile(ndcX, ndcY, ndcR, gridX, gridY, out.minX, out.maxX, out.minY, out.maxY);

    const float depthInflate = range * 1.15F;
    out.minZ = FindDepthSlice(std::max(nearPlane, dist - depthInflate), nearPlane, farPlane, gridZ);
    out.maxZ = FindDepthSlice(dist + depthInflate, nearPlane, farPlane, gridZ);
    out.minZ = std::max(0, out.minZ - 1);
    out.maxZ = std::min(gridZ - 1, out.maxZ + 1);
    return true;
}

}  // namespace

void VulkanClusteredForwardLights::CreateBuffers(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        const std::uint32_t framesInFlight) {
    device_ = device;
    framesInFlight_ = std::min(framesInFlight, 2U);
    for (std::uint32_t i = 0; i < framesInFlight_; ++i) {
        Flight& f = flights_[i];
        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                kClusterLightsGpuBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                f.lightsBuffer,
                f.lightsMemory);
        if (vkMapMemory(device, f.lightsMemory, 0, kClusterLightsGpuBytes, 0, &f.lightsMapped) != VK_SUCCESS) {
            throw std::runtime_error("VulkanClusteredForwardLights: map lights SSBO failed");
        }

        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                kClusterGridGpuBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                f.clusterBuffer,
                f.clusterMemory);
        if (vkMapMemory(device, f.clusterMemory, 0, kClusterGridGpuBytes, 0, &f.clusterMapped) != VK_SUCCESS) {
            throw std::runtime_error("VulkanClusteredForwardLights: map cluster SSBO failed");
        }
    }
}

void VulkanClusteredForwardLights::DestroyBuffers(VkDevice device) {
    for (std::uint32_t i = 0; i < framesInFlight_; ++i) {
        Flight& f = flights_[i];
        if (f.lightsMapped != nullptr) {
            vkUnmapMemory(device, f.lightsMemory);
            f.lightsMapped = nullptr;
        }
        if (f.lightsBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, f.lightsBuffer, nullptr);
            f.lightsBuffer = VK_NULL_HANDLE;
        }
        if (f.lightsMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, f.lightsMemory, nullptr);
            f.lightsMemory = VK_NULL_HANDLE;
        }

        if (f.clusterMapped != nullptr) {
            vkUnmapMemory(device, f.clusterMemory);
            f.clusterMapped = nullptr;
        }
        if (f.clusterBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, f.clusterBuffer, nullptr);
            f.clusterBuffer = VK_NULL_HANDLE;
        }
        if (f.clusterMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, f.clusterMemory, nullptr);
            f.clusterMemory = VK_NULL_HANDLE;
        }
    }
    framesInFlight_ = 0;
    device_ = VK_NULL_HANDLE;
}

void VulkanClusteredForwardLights::BuildAndUpload(
        const std::uint32_t frameIndex,
        const SceneRenderParams& scene,
        const ResolvedSceneLighting& lighting,
        const VkExtent2D extent) {
    (void)extent;
    if (frameIndex >= framesInFlight_ || flights_[frameIndex].lightsMapped == nullptr ||
        flights_[frameIndex].clusterMapped == nullptr) {
        return;
    }

    auto* lights = static_cast<ClusterLightsGpu*>(flights_[frameIndex].lightsMapped);
    auto* grid = static_cast<ClusterGridGpu*>(flights_[frameIndex].clusterMapped);
    std::memset(lights, 0, sizeof(ClusterLightsGpu));
    std::memset(grid, 0, sizeof(ClusterGridGpu));

    const std::uint32_t npl = static_cast<std::uint32_t>(std::min(
            scene.pointLights.GetSize(),
            static_cast<std::size_t>(kMaxClusteredPointLights)));
    const std::uint32_t nsl = static_cast<std::uint32_t>(std::min(
            scene.spotLights.GetSize(),
            static_cast<std::size_t>(kMaxClusteredSpotLights)));
    lights->header.numPointLights = npl;
    lights->header.numSpotLights = nsl;

    for (std::uint32_t i = 0; i < npl; ++i) {
        const ScenePointLight& pl = scene.pointLights[static_cast<std::size_t>(i)];
        lights->pointPositionRange[i][0] = pl.positionWorld.x;
        lights->pointPositionRange[i][1] = pl.positionWorld.y;
        lights->pointPositionRange[i][2] = pl.positionWorld.z;
        lights->pointPositionRange[i][3] = pl.range;
        lights->pointColorIntensity[i][0] = pl.color.x;
        lights->pointColorIntensity[i][1] = pl.color.y;
        lights->pointColorIntensity[i][2] = pl.color.z;
        lights->pointColorIntensity[i][3] = pl.intensity;
    }

    for (std::uint32_t si = 0; si < nsl; ++si) {
        const SceneSpotLight& sl = scene.spotLights[static_cast<std::size_t>(si)];
        Vector3 dir = sl.directionWorld.Normalized();
        const float halfInner = 0.5F * sl.innerConeRadians;
        const float halfOuter = 0.5F * sl.outerConeRadians;
        const float cosInner = std::cos(halfInner);
        const float cosOuter = std::cos(halfOuter);
        lights->spotPositionRange[si][0] = sl.positionWorld.x;
        lights->spotPositionRange[si][1] = sl.positionWorld.y;
        lights->spotPositionRange[si][2] = sl.positionWorld.z;
        lights->spotPositionRange[si][3] = sl.range;
        lights->spotDirectionCosOuter[si][0] = dir.x;
        lights->spotDirectionCosOuter[si][1] = dir.y;
        lights->spotDirectionCosOuter[si][2] = dir.z;
        lights->spotDirectionCosOuter[si][3] = cosOuter;
        lights->spotColorIntensity[si][0] = sl.color.x;
        lights->spotColorIntensity[si][1] = sl.color.y;
        lights->spotColorIntensity[si][2] = sl.color.z;
        lights->spotColorIntensity[si][3] = sl.intensity;
        lights->spotCosInner[si][0] = cosInner;
    }

    if (npl == 0 && nsl == 0) {
        return;
    }

    const float nearPlane = std::max(lighting.shadowCascadeNear, kClusterDepthNearMin);
    const float farPlane = std::max(lighting.shadowCascadeFar, nearPlane + 1.0F);
    const Vector3 camPos = scene.cameraPositionWorld;
    const Matrix4& viewProj = scene.viewProjection;

    std::uint32_t perClusterCounts[kClusterCount]{};

    auto countLight = [&](const Vector3& pos, const float range) {
        ClusterTileRange tiles{};
        if (!ComputeClusterTileRange(viewProj, camPos, pos, range, nearPlane, farPlane, tiles)) {
            return;
        }
        for (int iz = tiles.minZ; iz <= tiles.maxZ; ++iz) {
            for (int iy = tiles.minY; iy <= tiles.maxY; ++iy) {
                for (int ix = tiles.minX; ix <= tiles.maxX; ++ix) {
                    const std::uint32_t ci = ClusterIndex(ix, iy, iz);
                    if (perClusterCounts[ci] < kMaxLightsPerCluster) {
                        ++perClusterCounts[ci];
                    }
                }
            }
        }
    };

    for (std::uint32_t i = 0; i < npl; ++i) {
        const ScenePointLight& pl = scene.pointLights[static_cast<std::size_t>(i)];
        countLight(pl.positionWorld, pl.range);
    }
    for (std::uint32_t si = 0; si < nsl; ++si) {
        const SceneSpotLight& sl = scene.spotLights[static_cast<std::size_t>(si)];
        countLight(sl.positionWorld, sl.range);
    }

    std::uint32_t running = 0;
    for (std::uint32_t ci = 0; ci < kClusterCount; ++ci) {
        grid->offsets[ci] = running;
        grid->counts[ci] = perClusterCounts[ci];
        running += perClusterCounts[ci];
        if (running > kMaxClusterLightIndices) {
            running = kMaxClusterLightIndices;
        }
    }

    std::uint32_t cursors[kClusterCount]{};
    for (std::uint32_t ci = 0; ci < kClusterCount; ++ci) {
        cursors[ci] = grid->offsets[ci];
    }

    auto assignLight = [&](const Vector3& pos, const float range, const std::uint32_t globalIndex) {
        ClusterTileRange tiles{};
        if (!ComputeClusterTileRange(viewProj, camPos, pos, range, nearPlane, farPlane, tiles)) {
            return;
        }
        for (int iz = tiles.minZ; iz <= tiles.maxZ; ++iz) {
            for (int iy = tiles.minY; iy <= tiles.maxY; ++iy) {
                for (int ix = tiles.minX; ix <= tiles.maxX; ++ix) {
                    const std::uint32_t ci = ClusterIndex(ix, iy, iz);
                    if (cursors[ci] >= grid->offsets[ci] + grid->counts[ci]) {
                        continue;
                    }
                    if (cursors[ci] >= kMaxClusterLightIndices) {
                        continue;
                    }
                    grid->indices[cursors[ci]] = globalIndex;
                    ++cursors[ci];
                }
            }
        }
    };

    for (std::uint32_t i = 0; i < npl; ++i) {
        const ScenePointLight& pl = scene.pointLights[static_cast<std::size_t>(i)];
        assignLight(pl.positionWorld, pl.range, i);
    }
    for (std::uint32_t si = 0; si < nsl; ++si) {
        const SceneSpotLight& sl = scene.spotLights[static_cast<std::size_t>(si)];
        assignLight(sl.positionWorld, sl.range, npl + si);
    }
}

VkBuffer VulkanClusteredForwardLights::LightsBuffer(const std::uint32_t frameIndex) const noexcept {
    if (frameIndex >= framesInFlight_) {
        return VK_NULL_HANDLE;
    }
    return flights_[frameIndex].lightsBuffer;
}

VkBuffer VulkanClusteredForwardLights::ClusterBuffer(const std::uint32_t frameIndex) const noexcept {
    if (frameIndex >= framesInFlight_) {
        return VK_NULL_HANDLE;
    }
    return flights_[frameIndex].clusterBuffer;
}

}  // namespace Spark
