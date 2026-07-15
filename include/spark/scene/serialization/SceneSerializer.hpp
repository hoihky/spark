#pragma once

#include "spark/scene/GameWorld.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"

#include <functional>

namespace Spark {

class GameObject;

/** Captures ECS worlds into SceneDocument and writes text files. */
class SceneSerializer {
public:
    explicit SceneSerializer(const ComponentSnapshotRegistry& registry = ComponentSnapshotRegistry::Default());

    [[nodiscard]] SceneDocument Capture(
            const GameWorld& world,
            const SceneCaptureContext& ctx,
            const std::function<bool(const GameObject*)>& includeEntity) const;

    [[nodiscard]] bool WriteToFile(const SceneDocument& document, const char* path) const;
    [[nodiscard]] bool WriteToString(const SceneDocument& document, Utf8String& out) const;

private:
    const ComponentSnapshotRegistry& registry;
};

/** Reads SceneDocument files and instantiates ECS entities. */
class SceneDeserializer {
public:
    explicit SceneDeserializer(const ComponentSnapshotRegistry& registry = ComponentSnapshotRegistry::Default());

    [[nodiscard]] bool ReadFromFile(const char* path, SceneDocument& out) const;
    [[nodiscard]] bool ReadFromString(const char* text, SceneDocument& out) const;

    /**
     * Creates GameObjects, restores components, then links parents.
     * Returns false if any entity failed; successfully created objects remain in the world.
     */
    [[nodiscard]] bool Apply(
            const SceneDocument& document,
            GameWorld& world,
            const SceneApplyContext& ctx,
            HashMap<std::uint64_t, GameObject*>* outIdToObject = nullptr) const;

private:
    const ComponentSnapshotRegistry& registry;
};

}  // namespace Spark
