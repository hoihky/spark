#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/core/SceneInstanceTracker.hpp"
#include "spark/scene/editor/SceneEditorContentModel.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/prefab/PrefabCatalog.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class SceneManager;

/** Context passed to placement commands (Command pattern). */
struct ScenePlacementContext {
    GameWorld& world;
    SceneManager& sceneManager;
    SceneEditorContentModel& content;
    SceneInstanceTracker& instances;
    PrefabCatalog& prefabCatalog;
    Vector3 groundHit{Vector3::Zero};
    GameObject* selected = nullptr;
    SharedPtr<Mesh> unitCubeAsset{};
    void (*syncLightGizmoEmissive)(GameObject* lightObject) noexcept = nullptr;
    void (*setStatus)(const char* message, void* userData) noexcept = nullptr;
    void* statusUserData = nullptr;
};

/** Extensible editor placement action (Command). */
class IScenePlacementAction {
public:
    virtual ~IScenePlacementAction() = default;

    [[nodiscard]] virtual Utf8String GetMenuLabel() const = 0;
    [[nodiscard]] virtual bool IsAvailable(const ScenePlacementContext& ctx) const { (void)ctx; return true; }
    virtual bool Execute(ScenePlacementContext& ctx) = 0;
};

/** Ordered registry of placement commands for context menus. */
class ScenePlacementActionRegistry final {
public:
    void Register(UniquePtr<IScenePlacementAction> action);
    void Clear() noexcept;

    void BuildMenu(Array<Utf8String>& outLabels, const ScenePlacementContext& ctx);
    [[nodiscard]] bool Execute(std::size_t menuIndex, ScenePlacementContext& ctx);

    /** Registers demo mesh presets, spawn point, and lights. Prefabs are registered separately. */
    static void RegisterDemoDefaults(ScenePlacementActionRegistry& registry);
    static void RegisterPrefabActions(ScenePlacementActionRegistry& registry, const PrefabCatalog& catalog);
    /** Registers one prefab placement command (e.g. after runtime glTF import). */
    static void RegisterPrefabAction(ScenePlacementActionRegistry& registry, const PrefabCatalogEntry& entry);

private:
    Array<UniquePtr<IScenePlacementAction>> actions{};
    Array<std::size_t> visibleActionIndices{};
};

/** Factory for demo-specific menu commands (save/load/delete). */
[[nodiscard]] UniquePtr<IScenePlacementAction> MakeLambdaPlacementAction(
        Utf8String label,
        bool (*executeFn)(ScenePlacementContext&),
        bool (*availableFn)(const ScenePlacementContext&) = nullptr);

}  // namespace Spark
