#include "spark/scene/foliage/GrassTerrainSurfaceProbe.hpp"

#include "spark/ecs/components/rendering/TerrainComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Vector3.hpp"

#include <cmath>

namespace Spark {

float GrassTerrainSurfaceHit::SlopeAngleDegreesFromUp() const noexcept {
    if (!valid) {
        return 90.0F;
    }
    const float dot = normal.y;
    const float clamped = (dot < -1.0F) ? -1.0F : (dot > 1.0F ? 1.0F : dot);
    return RadiansToDegrees(std::acos(clamped));
}

bool GrassTerrainSurfaceProbe::ProbeWorld(
        const float worldX,
        const float worldZ,
        GrassTerrainSurfaceHit& outHit) const {
    outHit.Invalidate();
    if (terrainObject == nullptr) {
        return false;
    }
    const TerrainComponent* terrain = terrainObject->GetComponent<TerrainComponent>();
    if (terrain == nullptr) {
        return false;
    }
    float worldY = 0.0F;
    Vector3 normalWorld{};
    if (!terrain->TrySampleSurfaceWorld(*terrainObject, worldX, worldZ, worldY, normalWorld)) {
        return false;
    }
    outHit.SetSample(worldY, normalWorld);
    return true;
}

}  // namespace Spark
