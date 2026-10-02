#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

class GameObject;
class TerrainComponent;

/** Result of sampling terrain height + normal at a world XZ point (F2-03). */
class GrassTerrainSurfaceHit {
public:
    void Invalidate() noexcept { valid = false; }

    void SetSample(float worldY, const Vector3& normalWorld) noexcept {
        worldHeight = worldY;
        normal = normalWorld;
        valid = true;
    }

    [[nodiscard]] bool IsValid() const noexcept { return valid; }
    [[nodiscard]] float GetWorldHeight() const noexcept { return worldHeight; }
    [[nodiscard]] const Vector3& GetNormalWorld() const noexcept { return normal; }

    [[nodiscard]] float SlopeAngleDegreesFromUp() const noexcept;

private:
    bool valid = false;
    float worldHeight = 0.0F;
    Vector3 normal{0.0F, 1.0F, 0.0F};
};

/** CPU heightfield probe bound to a terrain <c>GameObject</c>. */
class GrassTerrainSurfaceProbe {
public:
    void BindTerrainObject(GameObject* terrainObjectIn) noexcept { terrainObject = terrainObjectIn; }
    [[nodiscard]] GameObject* GetTerrainObject() const noexcept { return terrainObject; }

    [[nodiscard]] bool ProbeWorld(float worldX, float worldZ, GrassTerrainSurfaceHit& outHit) const;

private:
    GameObject* terrainObject = nullptr;
};

}  // namespace Spark
