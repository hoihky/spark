#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/TerrainGeneratorSettings.hpp"

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

    /** Re-samples fBM into the height buffer and rebuilds the mesh. */
    void ResetHeightsToProcedural(GameObject& owner);

    /** Rebuilds mesh from the current height buffer (call after brush edits). */
    void RegenerateMesh(GameObject& owner);

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

    /** Transforms @p centerWorld to local XZ; scales radius from world using max horizontal scale of the object. */
    void ApplyHeightBrushWorld(GameObject& owner, Vector3 centerWorld, float radiusWorld, float deltaY);

private:
    void EnsureHeightBuffer(GameObject& owner);
    [[nodiscard]] static bool RayTriangle(
            Vector3 ro, Vector3 rd, Vector3 v0, Vector3 v1, Vector3 v2, float& outT) noexcept;

    TerrainGeneratorSettings settings;
    Vector3 meshAlbedo;
    Array<float> heightSamples{};
};

}  // namespace Spark
