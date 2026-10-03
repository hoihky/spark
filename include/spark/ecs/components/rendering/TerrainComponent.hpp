#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/mesh/TerrainGeneratorSettings.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;

/**
 * Procedural / editable terrain: height samples drive an XZ heightfield mesh (Custom slot).
 * Use ApplyHeightBrush* after ray hits; add MaterialComponent for texturing.
 */
class TerrainComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Terrain;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit TerrainComponent(TerrainGeneratorSettings settings, Vector3 meshAlbedo = Vector3{0.42F, 0.55F, 0.36F});

    void OnAttach(GameObject& owner) override;

    [[nodiscard]] const TerrainGeneratorSettings& GetSettings() const noexcept { return settings; }
    [[nodiscard]] const Array<float>& GetHeightSamples() const noexcept { return heightSamples; }

    SPARK_SCRIPT_BIND(get_subdiv_x)
    [[nodiscard]] std::int32_t GetSubdivX() const noexcept { return settings.subdivX; }
    SPARK_SCRIPT_BIND(get_subdiv_z)
    [[nodiscard]] std::int32_t GetSubdivZ() const noexcept { return settings.subdivZ; }
    SPARK_SCRIPT_BIND(get_half_extent_x)
    [[nodiscard]] float GetHalfExtentX() const noexcept { return settings.halfExtentX; }
    SPARK_SCRIPT_BIND(get_half_extent_z)
    [[nodiscard]] float GetHalfExtentZ() const noexcept { return settings.halfExtentZ; }
    SPARK_SCRIPT_BIND(get_height_scale)
    [[nodiscard]] float GetHeightScale() const noexcept { return settings.heightScale; }
    SPARK_SCRIPT_BIND(get_noise_scale)
    [[nodiscard]] float GetNoiseScale() const noexcept { return settings.noiseScale; }
    SPARK_SCRIPT_BIND(get_octaves)
    [[nodiscard]] std::int32_t GetOctaves() const noexcept { return settings.octaves; }
    SPARK_SCRIPT_BIND(get_persistence)
    [[nodiscard]] float GetPersistence() const noexcept { return settings.persistence; }
    SPARK_SCRIPT_BIND(get_lacunarity)
    [[nodiscard]] float GetLacunarity() const noexcept { return settings.lacunarity; }
    SPARK_SCRIPT_BIND(get_seed)
    [[nodiscard]] std::uint32_t GetSeed() const noexcept { return settings.seed; }
    SPARK_SCRIPT_BIND(get_world_units_per_texture_repeat)
    [[nodiscard]] float GetWorldUnitsPerTextureRepeat() const noexcept {
        return settings.worldUnitsPerTextureRepeat;
    }
    SPARK_SCRIPT_BIND(get_mesh_albedo)
    [[nodiscard]] Vector3 GetMeshAlbedoRgb() const noexcept { return meshAlbedo; }

    SPARK_SCRIPT_BIND(set_height_scale)
    void SetHeightScale(float v) noexcept { settings.heightScale = v; }
    SPARK_SCRIPT_BIND(set_noise_scale)
    void SetNoiseScale(float v) noexcept { settings.noiseScale = v; }
    SPARK_SCRIPT_BIND(set_seed)
    void SetSeed(std::uint32_t v) noexcept { settings.seed = v; }

    /** Re-samples fBM into the height buffer and rebuilds the mesh. */
    void ResetHeightsToProcedural(GameObject& owner);

    SPARK_SCRIPT_BIND(reset_heights_to_procedural)
    void ResetHeightsToProceduralOn(GameObject* owner);

    /** Rebuilds mesh from the current height buffer (call after brush edits). */
    void RegenerateMesh(GameObject& owner);

    SPARK_SCRIPT_BIND(regenerate_mesh)
    void RegenerateMeshOn(GameObject* owner);

    /**
     * Ray vs heightfield triangles in object-local space (terrain lies in XZ; Y is up).
     * @param rayDirWorld should be a direction (any non-zero length; it is normalized).
     */
    [[nodiscard]] bool TryRaycastWorld(
            const GameObject& owner,
            Vector3 rayOriginWorld,
            Vector3 rayDirWorld,
            float maxDistance,
            Vector3& outHitWorld) const;

    /** Smooth additive brush on the height grid (local XZ plane; radius in local units). */
    void ApplyHeightBrushLocal(GameObject& owner, Vector2 centerXZ, float radiusXZ, float deltaY);

    SPARK_SCRIPT_BIND(apply_height_brush_local)
    void ApplyHeightBrushLocalOn(GameObject* owner, const Vector2& centerXZ, float radiusXZ, float deltaY);

    /** Transforms @p centerWorld to local XZ; scales radius from world using max horizontal scale of the object. */
    void ApplyHeightBrushWorld(GameObject& owner, Vector3 centerWorld, float radiusWorld, float deltaY);

    SPARK_SCRIPT_BIND(apply_height_brush_world)
    void ApplyHeightBrushWorldOn(
            GameObject* owner,
            const Vector3& centerWorld,
            float radiusWorld,
            float deltaY);

    SPARK_SCRIPT_BIND(try_sample_height_world)
    bool TrySampleHeightWorldOn(
            const GameObject* owner,
            float worldX,
            float worldZ,
            float* outWorldY) const;

    /**
     * Radial island mask: heights fade to @p submergedDepth outside @p beachRadiusWorld.
     * Call after procedural generation to carve shoreline around the origin.
     */
    void ApplyIslandFalloff(
            GameObject& owner,
            float coreRadiusWorld,
            float beachRadiusWorld,
            float submergedDepth);

    /**
     * Bilinear height sample on the terrain grid in world XZ (CPU debug / gameplay).
     * GPU shoreline foam uses scene-depth reconstruction instead (see water_foam.glsl).
     */
    [[nodiscard]] bool TrySampleHeightWorld(
            const GameObject& owner,
            float worldX,
            float worldZ,
            float& outWorldY) const;

    /**
     * Bilinear height + upward-facing normal in world space (for grass grounding / slope tests).
     */
    [[nodiscard]] bool TrySampleSurfaceWorld(
            const GameObject& owner,
            float worldX,
            float worldZ,
            float& outWorldY,
            Vector3& outNormalWorld) const;

private:
    void EnsureHeightBuffer(GameObject& owner);
    [[nodiscard]] static bool RayTriangle(
            Vector3 ro, Vector3 rd, Vector3 v0, Vector3 v1, Vector3 v2, float& outT) noexcept;

    TerrainGeneratorSettings settings;
    Vector3 meshAlbedo;
    Array<float> heightSamples{};
};

}  // namespace Spark
