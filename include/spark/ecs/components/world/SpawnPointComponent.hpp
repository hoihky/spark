#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Named spawn location for scene transitions and play-in-editor.
 * Optional default prefab path is relative to the scene/assets root.
 */
class SpawnPointComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SpawnPoint;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(get_spawn_name)
    [[nodiscard]] const Utf8String& GetSpawnName() const noexcept { return spawnName; }
    SPARK_SCRIPT_BIND(get_default_prefab_path)
    [[nodiscard]] const Utf8String& GetDefaultPrefabPath() const noexcept { return defaultPrefabPath; }

    SPARK_SCRIPT_BIND(set_spawn_name)
    void SetSpawnName(const char* name) noexcept { spawnName = Utf8String(name != nullptr ? name : ""); }
    void SetDefaultPrefabPath(const char* path) noexcept {
        defaultPrefabPath = Utf8String(path != nullptr ? path : "");
    }

private:
    Utf8String spawnName{"Player"};
    Utf8String defaultPrefabPath{};
};

}  // namespace Spark
