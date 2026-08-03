#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/PhysicsWorld3D.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/colliders/DynamicBody3D.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"

namespace Spark {

/** Gravity, damping, swept CCD translation, and orientation integration for one substep. */
class RigidbodyIntegrator3D {
public:
    static void IntegrateSubstep(
            Array<DynamicBody3D>& bodies,
            float substepDt,
            const PhysicsWorld3DSettings& settings,
            const Array<Collider3D>& colliders,
            SpatialHashGrid3D& broadPhase,
            Array<std::uint32_t>& queryScratch) noexcept;
};

}  // namespace Spark
