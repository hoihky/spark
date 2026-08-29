#pragma once

#include "spark/demo/DemoMode.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

enum class DemoCategory : std::uint8_t {
    Graphics3D,
    Gameplay3D,
    Gameplay2D,
    Tools,
    Arcade,
};

/** Stable id aligned with shell storage / legacy hotkey table (0…20). */
enum class DemoStorageId : std::uint8_t {
    Basic3D = 0,
    Sky,
    Particles,
    Terrain,
    Character,
    Platformer2D,
    BroadPhase2D,
    Maze3D,
    PhysicsBall3D,
    SceneEditor3D,
    Tetris2D,
    Connect3,
    SpaceInvaders2D,
    SteeringShowcase3D,
    ToonShading,
    MaterialShowcase3D,
    TimeOfDay,
    RenderLayers2D,
    TilemapShowcase2D,
    ImGuiShowcase,
    GltfSamples3D,
    ModelViewer3D,
#if SPARK_HAS_EDITOR
    SparkEditor,
#endif
    Count,
};

struct DemoCatalogEntry {
    DemoStorageId storageId = DemoStorageId::Basic3D;
    DemoMode mode = DemoMode::ThreeD;
    DemoCategory category = DemoCategory::Graphics3D;
    bool recommended = false;
    const char* title = "";
    const char* subtitle = "";
    /** Menu number shown in launcher (e.g. "17"). */
    const char* menuNumber = "";
    /**
     * Digit hotkey: 1–9 → keys 1–9, 10 → key 0, -1 = none.
     * Letter hotkey: uppercase GLFW key name char (e.g. 'Q'), 0 = none.
     */
    int digitHotkey = -1;
    char letterHotkey = '\0';
};

/** Read-only registry for SparkDemo launcher labels, categories, and hotkeys. */
class DemoCatalog {
public:
    [[nodiscard]] static constexpr std::size_t StorageCount() noexcept {
        return static_cast<std::size_t>(DemoStorageId::Count);
    }

    /** Launcher rows in demo-number order (1…21). */
    [[nodiscard]] static constexpr std::size_t LauncherRowCount() noexcept { return StorageCount(); }

    [[nodiscard]] static const DemoCatalogEntry& Entry(DemoStorageId id) noexcept;
    [[nodiscard]] static const DemoCatalogEntry& EntryByStorageIndex(std::size_t storageIndex) noexcept;

    [[nodiscard]] static constexpr bool IsSelectableRow(const std::size_t rowIndex) noexcept {
        return rowIndex < StorageCount();
    }

    [[nodiscard]] static DemoStorageId LauncherStorageId(const std::size_t rowIndex) noexcept;

    [[nodiscard]] static const char* CategoryLabel(DemoCategory category) noexcept;
    [[nodiscard]] static const char* LauncherRowLabel(std::size_t rowIndex) noexcept;

    [[nodiscard]] static bool TryResolveDigitHotkey(int glfwDigitKey, DemoStorageId& outId) noexcept;
    [[nodiscard]] static bool TryResolveLetterHotkey(int glfwKey, DemoStorageId& outId) noexcept;
};

}  // namespace Spark
