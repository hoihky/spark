#pragma once

#include "spark/core/Array.hpp"
#include "spark/scene/core/SceneInstanceId.hpp"

namespace Spark {

class SceneManager;

/** Tracks additive scene instances loaded into one world (Registry of instance ids). */
class SceneInstanceTracker final {
public:
    void Register(SceneInstanceId instanceId) noexcept;
    void UnloadAll(SceneManager& manager) noexcept;
    void Clear() noexcept;

    [[nodiscard]] bool Contains(SceneInstanceId instanceId) const noexcept;
    [[nodiscard]] std::size_t GetCount() const noexcept { return instanceIds.GetSize(); }

private:
    Array<SceneInstanceId> instanceIds{};
};

}  // namespace Spark
