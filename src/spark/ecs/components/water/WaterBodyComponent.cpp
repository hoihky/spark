#include "spark/ecs/components/water/WaterBodyComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/scene/camera/Camera.hpp"
#include "spark/scene/water/WaterWavePreset.hpp"

#include <algorithm>

namespace Spark {

WaterBodyComponent::WaterBodyComponent(
        WaterBodyMode modeIn,
        float waterLevelYIn,
        WaterBodyExtent extentIn,
        WaterWavePresetId wavePresetIn,
        WaterSurfaceMeshSettings meshSettingsIn,
        Vector3 meshAlbedoIn)
        : mode(modeIn),
          waterLevelY(waterLevelYIn),
          extent(extentIn),
          wavePresetId(wavePresetIn),
          surfaceMesh(meshSettingsIn),
          meshAlbedo(meshAlbedoIn) {}

void WaterBodyComponent::OnAttach(GameObject& owner) {
    surfaceDirty = true;
    RegenerateSurface(owner);
}

void WaterBodyComponent::OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) {
    (void)context;

    if (surfaceDirty) {
        RegenerateSurface(owner);
        return;
    }

    waveTimeSeconds += timing.deltaTimeSeconds * waveTimeScale;

    if (mode == WaterBodyMode::InfiniteOcean) {
        Vector3 cameraWorld{};
        if (TryResolveClipmapCameraWorld(owner.GetWorld(), cameraWorld)) {
            UpdateInfiniteOceanClipmap(owner, cameraWorld);
        }
    }
}

void WaterBodyComponent::SetClipmapCameraWorld(const Vector3& worldPosition) noexcept {
    clipmapCameraOverride = worldPosition;
}

void WaterBodyComponent::ClearClipmapCameraOverride() noexcept {
    clipmapCameraOverride.reset();
}

bool WaterBodyComponent::TryResolveClipmapCameraWorld(const GameWorld& world, Vector3& outWorld) const noexcept {
    if (clipmapCameraOverride.has_value()) {
        outWorld = *clipmapCameraOverride;
        return true;
    }
    ResolvedCamera camera{};
    if (TryResolveMainCamera(world, camera) && camera.object != nullptr) {
        outWorld = camera.object->GetWorldMatrix().TranslationVector();
        return true;
    }
    return false;
}

void WaterBodyComponent::SetMode(WaterBodyMode value) noexcept {
    if (mode == value) {
        return;
    }
    mode = value;
    surfaceDirty = true;
}

void WaterBodyComponent::SetWaterLevelY(float value) noexcept {
    if (waterLevelY == value) {
        return;
    }
    waterLevelY = value;
    surfaceDirty = true;
}

void WaterBodyComponent::SetExtent(const WaterBodyExtent& value) noexcept {
    extent = value;
    surfaceDirty = true;
}

void WaterBodyComponent::SetWavePresetId(WaterWavePresetId value) noexcept {
    wavePresetId = value;
}

void WaterBodyComponent::SetSurfaceMeshSettings(WaterSurfaceMeshSettings value) noexcept {
    surfaceMesh.SetSettings(value);
    surfaceDirty = true;
}

void WaterBodyComponent::SetMeshAlbedo(const Vector3& value) noexcept {
    meshAlbedo = value;
}

WaterWaveSettings WaterBodyComponent::GetResolvedWaveSettings() const noexcept {
    WaterWaveSettings settings = WaterWavePreset(wavePresetId).ToSettings();
    if (waveAmplitudeScale == 1.0F && waveSpeedScale == 1.0F) {
        return settings;
    }

    const std::uint32_t count = settings.GetActiveWaveCount();
    for (std::uint32_t wi = 0; wi < count; ++wi) {
        GerstnerWave& wave = settings.GetWave(wi);
        if (waveAmplitudeScale != 1.0F) {
            wave.amplitude *= waveAmplitudeScale;
            wave.steepness = (std::min)(wave.steepness * waveAmplitudeScale, 0.95F);
        }
        if (waveSpeedScale != 1.0F) {
            wave.speed *= waveSpeedScale;
        }
    }
    return settings;
}

Vector3 WaterBodyComponent::GetResolvedSurfaceAlbedo(const MaterialComponent* material) const noexcept {
    Vector3 albedo = meshAlbedo;
    if (material != nullptr) {
        const Vector3& tint = material->GetTint();
        albedo = {albedo.x * tint.x, albedo.y * tint.y, albedo.z * tint.z};
    }
    return albedo;
}

void WaterBodyComponent::RegenerateSurface(GameObject& owner) {
    const float tileHalfX = ResolveTileHalfExtentX();
    const float tileHalfZ = ResolveTileHalfExtentZ();
    Vector2 centerXZ{};

    if (mode == WaterBodyMode::InfiniteOcean) {
        Vector3 cameraWorld{};
        if (TryResolveClipmapCameraWorld(owner.GetWorld(), cameraWorld)) {
            centerXZ = surfaceMesh.ComputeSnappedAnchorXZ(cameraWorld);
        } else if (const TransformComponent* transform = owner.GetComponent<TransformComponent>()) {
            const Vector3 translation = transform->GetLocalTransform().translation;
            centerXZ = {translation.x, translation.z};
        }
    } else {
        centerXZ = ResolveFiniteCenterXZ(owner);
    }

    surfaceMesh.RebuildTile(tileHalfX, tileHalfZ, centerXZ);
    SyncTransformToSurface(owner, centerXZ);
    ApplyMeshToOwner(owner);
    surfaceDirty = false;
}

void WaterBodyComponent::UpdateInfiniteOceanClipmap(GameObject& owner, const Vector3& cameraWorld) {
    if (mode != WaterBodyMode::InfiniteOcean) {
        return;
    }

    if (!surfaceDirty && !surfaceMesh.ShouldRebuildForCamera(cameraWorld)) {
        SyncTransformToSurface(owner, surfaceMesh.GetAnchorXZ());
        return;
    }

    const Vector2 centerXZ = surfaceMesh.ComputeSnappedAnchorXZ(cameraWorld);
    const float tileHalf = surfaceMesh.GetSettings().GetTileHalfExtent();
    surfaceMesh.RebuildTile(tileHalf, tileHalf, centerXZ);
    SyncTransformToSurface(owner, centerXZ);
    ApplyMeshToOwner(owner);
    surfaceDirty = false;
}

float WaterBodyComponent::ResolveTileHalfExtentX() const noexcept {
    if (mode == WaterBodyMode::InfiniteOcean) {
        return surfaceMesh.GetSettings().GetTileHalfExtent();
    }
    return extent.GetHalfExtentX();
}

float WaterBodyComponent::ResolveTileHalfExtentZ() const noexcept {
    if (mode == WaterBodyMode::InfiniteOcean) {
        return surfaceMesh.GetSettings().GetTileHalfExtent();
    }
    return extent.GetHalfExtentZ();
}

Vector2 WaterBodyComponent::ResolveFiniteCenterXZ(const GameObject& owner) const noexcept {
    if (const TransformComponent* transform = owner.GetComponent<TransformComponent>()) {
        const Vector3 translation = transform->GetLocalTransform().translation;
        return {translation.x, translation.z};
    }
    return {};
}

void WaterBodyComponent::SyncTransformToSurface(GameObject& owner, Vector2 centerXZ) const {
    TransformComponent* transform = owner.GetComponent<TransformComponent>();
    if (transform == nullptr) {
        transform = owner.AddComponent<TransformComponent>();
    }
    transform->SetTranslation({centerXZ.x, waterLevelY, centerXZ.y});
}

void WaterBodyComponent::ApplyMeshToOwner(GameObject& owner) {
    MeshComponent* meshComponent = owner.GetComponent<MeshComponent>();
    if (meshComponent == nullptr) {
        owner.AddComponent<MeshComponent>(surfaceMesh.GetMesh(), SceneMeshSlot::Custom, meshAlbedo);
        return;
    }
    meshComponent->SetMesh(surfaceMesh.GetMesh());
    meshComponent->SetAlbedo(meshAlbedo);
}

}  // namespace Spark
