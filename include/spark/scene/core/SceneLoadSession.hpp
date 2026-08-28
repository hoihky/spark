#pragma once

#include "spark/scene/core/SceneInstanceId.hpp"
#include "spark/scene/core/SceneManager.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"

namespace Spark {

/**
 * Facade over <c>SceneManager</c> async loads: keeps the pending document and
 * provides blocking <c>AwaitReady</c> for editor play mode.
 */
class SceneLoadSession final {
public:
    explicit SceneLoadSession(SceneManager& inManager) noexcept : manager(inManager) {}

    [[nodiscard]] SceneInstanceId LoadBlocking(const char* path, const SceneLoadOptions& options = {});
    [[nodiscard]] SceneInstanceId BeginAsync(const char* path, const SceneLoadOptions& options = {});
    [[nodiscard]] SceneInstanceId BeginAsync(
            const SceneDocument& document,
            const char* path,
            const SceneLoadOptions& options = {});

    void Pump() noexcept { manager.Pump(); }

    [[nodiscard]] bool AwaitReady(SceneInstanceId instanceId, int maxPumpIterations = 100000) noexcept;
    [[nodiscard]] bool IsReady(SceneInstanceId instanceId) const noexcept { return manager.IsSceneReady(instanceId); }
    [[nodiscard]] bool HasFailed(SceneInstanceId instanceId) const noexcept { return manager.HasSceneFailed(instanceId); }

    void SetPendingDocument(SceneDocument document) noexcept { pendingDocument = MoveTemp(document); }
    [[nodiscard]] const SceneDocument& GetPendingDocument() const noexcept { return pendingDocument; }
    void ClearPendingDocument() noexcept { pendingDocument = SceneDocument{}; }

    [[nodiscard]] SceneManager& GetManager() noexcept { return manager; }

private:
    SceneManager& manager;
    SceneDocument pendingDocument{};
};

}  // namespace Spark
