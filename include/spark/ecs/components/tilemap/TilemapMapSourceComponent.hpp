#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/scene/tilemap/TilemapDocumentApply.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class IEngineContext;

/**
 * Tooling entry point (Phase G): import a <c>.tmx</c> or cached <c>.sparkmap</c>, optional hot reload.
 * Sibling <c>TilemapComponent</c> is created/updated on import.
 */
class TilemapMapSourceComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::TilemapMapSource;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(get_tmx_path)
    [[nodiscard]] const Utf8String& GetTmxPath() const noexcept { return tmxPath; }
    SPARK_SCRIPT_BIND(set_tmx_path)
    void SetTmxPath(const char* path) noexcept;

    SPARK_SCRIPT_BIND(get_spark_map_path)
    [[nodiscard]] const Utf8String& GetSparkMapPath() const noexcept { return sparkMapPath; }
    SPARK_SCRIPT_BIND(set_spark_map_path)
    void SetSparkMapPath(const char* path) noexcept;

    SPARK_SCRIPT_BIND(get_pixels_per_world_unit)
    [[nodiscard]] float GetPixelsPerWorldUnit() const noexcept { return applyOptions.pixelsPerWorldUnit; }
    SPARK_SCRIPT_BIND(set_pixels_per_world_unit)
    void SetPixelsPerWorldUnit(const float value) noexcept { applyOptions.pixelsPerWorldUnit = value; }

    SPARK_SCRIPT_BIND(get_import_on_attach)
    [[nodiscard]] bool GetImportOnAttach() const noexcept { return importOnAttach; }
    SPARK_SCRIPT_BIND(set_import_on_attach)
    void SetImportOnAttach(const bool enabled) noexcept { importOnAttach = enabled; }

    SPARK_SCRIPT_BIND(get_hot_reload)
    [[nodiscard]] bool GetHotReload() const noexcept { return hotReload; }
    SPARK_SCRIPT_BIND(set_hot_reload)
    void SetHotReload(const bool enabled) noexcept { hotReload = enabled; }

    /** Re-reads TMX (or sparkmap if set) and reapplies to the owner. */
    bool ImportNow(GameObject& owner, GameWorld& world);

    SPARK_SCRIPT_BIND(get_last_error)
    [[nodiscard]] const Utf8String& GetLastError() const noexcept { return lastError; }
    SPARK_SCRIPT_BIND(get_last_validation_summary)
    [[nodiscard]] const Utf8String& GetLastValidationSummary() const noexcept { return lastValidationSummary; }

    void OnAttach(GameObject& owner) override;
    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

private:
    [[nodiscard]] bool ImportFromSources(GameObject& owner, GameWorld& world, Utf8String& outError);
    void TouchSourceTimestamp();

    Utf8String tmxPath{};
    Utf8String sparkMapPath{};
    TilemapDocumentApplier::Options applyOptions{};
    bool importOnAttach = true;
    bool hotReload = false;
    std::int64_t lastSourceTimestampNs = -1;
    Utf8String lastError{};
    Utf8String lastValidationSummary{};
};

}  // namespace Spark
