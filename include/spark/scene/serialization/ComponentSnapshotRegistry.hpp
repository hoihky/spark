#pragma once

#include "spark/core/Array.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"

namespace Spark {

/** Owns component snapshot handlers keyed by ComponentKind (registry pattern). */
class ComponentSnapshotRegistry {
public:
    ComponentSnapshotRegistry();
    ~ComponentSnapshotRegistry();

    ComponentSnapshotRegistry(const ComponentSnapshotRegistry&) = delete;
    ComponentSnapshotRegistry& operator=(const ComponentSnapshotRegistry&) = delete;

    void Register(UniquePtr<IComponentSnapshotHandler> handler);
    [[nodiscard]] const IComponentSnapshotHandler* Find(ComponentKind kind) const noexcept;
    [[nodiscard]] const IComponentSnapshotHandler* FindByTag(const char* tag) const noexcept;
    [[nodiscard]] std::size_t GetHandlerCount() const noexcept { return handlers.GetSize(); }
    [[nodiscard]] const IComponentSnapshotHandler* GetHandlerAt(const std::size_t index) const noexcept;

    /** Process-wide default registry with built-in ECS handlers. */
    [[nodiscard]] static ComponentSnapshotRegistry& Default();

private:
    Array<UniquePtr<IComponentSnapshotHandler>> handlers;
};

}  // namespace Spark
