#pragma once

#include <cstdint>

#ifndef SPARK_SCENE_DRAW_ITEM_COMPLETE
#include "spark/engine/SceneRenderParams.hpp"
#else

/**
 * One instanced opaque lit draw in <c>SceneRenderParams::meshInstancedBatches</c>.
 * Per-instance transforms live in <c>meshInstances</c> at [<c>instanceBegin</c>, …).
 */
class SceneMeshInstancedBatch {
public:
    void SetDrawTemplate(const SceneDrawItem& item) noexcept { drawTemplate = item; }
    [[nodiscard]] const SceneDrawItem& GetDrawTemplate() const noexcept { return drawTemplate; }
    [[nodiscard]] SceneDrawItem& GetDrawTemplate() noexcept { return drawTemplate; }

    void SetInstanceRange(const std::uint32_t begin, const std::uint32_t count) noexcept {
        instanceBegin = begin;
        instanceCount = count;
    }
    [[nodiscard]] std::uint32_t GetInstanceBegin() const noexcept { return instanceBegin; }
    [[nodiscard]] std::uint32_t GetInstanceCount() const noexcept { return instanceCount; }

private:
    SceneDrawItem drawTemplate{};
    std::uint32_t instanceBegin = 0;
    std::uint32_t instanceCount = 0;
};

#endif  // SPARK_SCENE_DRAW_ITEM_COMPLETE