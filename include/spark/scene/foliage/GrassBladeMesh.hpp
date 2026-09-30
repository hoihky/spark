#pragma once

#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/mesh/Mesh.hpp"

namespace Spark {

/** Procedural crossed-blade grass clump for foliage instancing (Y-up, base at y = 0). */
class GrassBladeMesh {
public:
    static constexpr float kDefaultBladeHeight = 0.68F;
    static constexpr float kDefaultBladeWidth = 0.09F;

    [[nodiscard]] static SharedPtr<Mesh> CreateSharedBladeMesh();
    [[nodiscard]] static float DefaultBladeHeight() noexcept { return kDefaultBladeHeight; }
};

}  // namespace Spark
