#pragma once

#include "spark/ecs/GameComponent.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Runtime tag linking an expanded glTF instance node back to its source node index.
 * Used to persist per-node mesh/material overrides without serializing full child entities.
 */
class GltfInstanceNodeComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::GltfInstanceNode;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(get_node_index)
    [[nodiscard]] std::uint32_t GetNodeIndex() const noexcept { return nodeIndex; }
    SPARK_SCRIPT_BIND(set_node_index)
    void SetNodeIndex(const std::uint32_t index) noexcept { nodeIndex = index; }

private:
    std::uint32_t nodeIndex = 0U;
};

}  // namespace Spark
