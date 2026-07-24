#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/scene/tilemap/TilemapDocumentApply.hpp"

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

    [[nodiscard]] const Utf8String& GetTmxPath() const noexcept { return tmxPath; }
    void SetTmxPath(const char* path) noexcept;

    [[nodiscard]] const Utf8String& GetSparkMapPath() const noexcept { return sparkMapPath; }
    void SetSparkMapPath(const char* path) noexcept;

    [[nodiscard]] float GetPixelsPerWorldUnit() const noexcept { return applyOptions.pixelsPerWorldUnit; }
    void SetPixelsPerWorldUnit(const float value) noexcept { applyOptions.pixelsPerWorldUnit = value; }

    [[nodiscard]] bool GetImportOnAttach() const noexcept { return importOnAttach; }
    void SetImportOnAttach(const bool enabled) noexcept { importOnAttach = enabled; }

    [[nodiscard]] bool GetHotReload() const noexcept { return hotReload; }
    void SetHotReload(const bool enabled) noexcept { hotReload = enabled; }

    /** Re-reads TMX (or sparkmap if set) and reapplies to the owner. */
    bool ImportNow(GameObject& owner, GameWorld& world);

    [[nodiscard]] const Utf8String& GetLastError() const noexcept { return lastError_; }

    void OnAttach(GameObject& owner) override;
    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

private:
    [[nodiscard]] bool ImportFromSources(GameObject& owner, GameWorld& world, Utf8String& outError);
    void TouchSourceTimestamp();

    Utf8String tmxPath{};
    Utf8String sparkMapPath{};
    TilemapDocumentApplyOptions applyOptions{};
    bool importOnAttach = true;
    bool hotReload = false;
    std::int64_t lastSourceTimestampNs_ = -1;
    Utf8String lastError_{};
};

}  // namespace Spark
