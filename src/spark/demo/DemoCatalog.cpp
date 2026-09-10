#include "spark/demo/DemoCatalog.hpp"

#include <GLFW/glfw3.h>

#include <cstdio>

namespace Spark {

namespace {

constexpr DemoCatalogEntry kEntries[] = {
        {DemoStorageId::Basic3D,
         DemoMode::ThreeD,
         DemoCategory::Graphics3D,
         false,
         "Basic 3D",
         "glTF meshes, shadows, decals, skinned fox",
         "1",
         1,
         '\0'},
        {DemoStorageId::Sky,
         DemoMode::Sky,
         DemoCategory::Graphics3D,
         false,
         "Sky",
         "Box / dome / plane / HDR equirect",
         "2",
         2,
         '\0'},
        {DemoStorageId::Particles,
         DemoMode::Particles,
         DemoCategory::Graphics3D,
         false,
         "VFX Showcase",
         "Built-in RPG/FPS/action effects · tune · save .sparkvfx",
         "3",
         3,
         '\0'},
        {DemoStorageId::Terrain,
         DemoMode::Terrain,
         DemoCategory::Graphics3D,
         false,
         "Terrain",
         "Heightfield mesh, fly camera",
         "4",
         4,
         '\0'},
        {DemoStorageId::Character,
         DemoMode::Character,
         DemoCategory::Gameplay3D,
         true,
         "Character camera",
         "Skinned glTF, 1st/3rd person, animation FSM",
         "5",
         5,
         '\0'},
        {DemoStorageId::Platformer2D,
         DemoMode::Platformer2D,
         DemoCategory::Gameplay2D,
         true,
         "2D platformer",
         "Combat, gems, physics, sprite FSM, audio",
         "6",
         6,
         '\0'},
        {DemoStorageId::BroadPhase2D,
         DemoMode::BroadPhase2D,
         DemoCategory::Gameplay2D,
         false,
         "Broad-phase 2D",
         "Dungeon gems, hinge joint, WASD",
         "7",
         7,
         '\0'},
        {DemoStorageId::Maze3D,
         DemoMode::Maze3D,
         DemoCategory::Gameplay3D,
         false,
         "3D maze",
         "Procedural maze, guard AI, collectibles",
         "8",
         8,
         '\0'},
        {DemoStorageId::PhysicsBall3D,
         DemoMode::PhysicsBall3D,
         DemoCategory::Gameplay3D,
         false,
         "3D physics",
         "Throw balls, joints, Dear ImGui tuning",
         "9",
         9,
         '\0'},
        {DemoStorageId::SceneEditor3D,
         DemoMode::SceneEditor3D,
         DemoCategory::Tools,
         false,
         "Scene editor",
         "Retained UI, placement, fly / orbit camera",
         "10",
         10,
         '\0'},
        {DemoStorageId::Tetris2D,
         DemoMode::Tetris2D,
         DemoCategory::Arcade,
         false,
         "Tetris",
         "Additive line-clear FX",
         "11",
         -1,
         'T'},
        {DemoStorageId::Connect3,
         DemoMode::Connect3,
         DemoCategory::Arcade,
         false,
         "Match-3",
         "Grid puzzle prototype",
         "12",
         -1,
         'C'},
        {DemoStorageId::SpaceInvaders2D,
         DemoMode::SpaceInvaders2D,
         DemoCategory::Arcade,
         false,
         "Space Invaders",
         "Classic arcade clone",
         "13",
         -1,
         'I'},
        {DemoStorageId::SteeringShowcase3D,
         DemoMode::SteeringShowcase3D,
         DemoCategory::Gameplay3D,
         false,
         "3D steering",
         "Seek, flee, flock, obstacle avoidance",
         "14",
         -1,
         ';'},
        {DemoStorageId::ToonShading,
         DemoMode::ToonShading,
         DemoCategory::Graphics3D,
         false,
         "Toon / cel shading",
         "Lit PBR vs toon side-by-side",
         "15",
         -1,
         'Y'},
        {DemoStorageId::MaterialShowcase3D,
         DemoMode::MaterialShowcase3D,
         DemoCategory::Graphics3D,
         false,
         "Material ball",
         "PBR maps, emissive, live tuning",
         "16",
         -1,
         'B'},
        {DemoStorageId::TimeOfDay,
         DemoMode::TimeOfDay,
         DemoCategory::Graphics3D,
         true,
         "Time of day",
         "HDR sky, fog, post volumes, CSM shadows",
         "17",
         -1,
         'N'},
        {DemoStorageId::RenderLayers2D,
         DemoMode::RenderLayers2D,
         DemoCategory::Gameplay2D,
         false,
         "2D render layers",
         "Y-sort, layer masks, farming RPG style",
         "18",
         -1,
         '\0'},
        {DemoStorageId::TilemapShowcase2D,
         DemoMode::TilemapShowcase2D,
         DemoCategory::Gameplay2D,
         false,
         "Tilemap showcase",
         "TMX load, animation, pathfinding",
         "19",
         -1,
         '\0'},
        {DemoStorageId::ImGuiShowcase,
         DemoMode::ImGuiShowcase,
         DemoCategory::Tools,
         false,
         "Dear ImGui tools",
         "Docking, hierarchy, inspector, console",
         "20",
         -1,
         'G'},
        {DemoStorageId::GltfSamples3D,
         DemoMode::GltfSamples3D,
         DemoCategory::Graphics3D,
         true,
         "glTF PBR showcase",
         "Damaged Helmet + HDR studio IBL",
         "21",
         -1,
         'Q'},
        {DemoStorageId::ModelViewer3D,
         DemoMode::ModelViewer3D,
         DemoCategory::Tools,
         false,
         "3D model viewer",
         "Cycle bundled glTF samples with TAB",
         "22",
         -1,
         'V'},
        {DemoStorageId::WaterLake,
         DemoMode::WaterLake,
         DemoCategory::Graphics3D,
         true,
         "Water lake",
         "Island terrain · infinite ocean · fly camera · time scale",
         "23",
         -1,
         'W'},
#if SPARK_HAS_EDITOR
        {DemoStorageId::SparkEditor,
         DemoMode::SparkEditor,
         DemoCategory::Tools,
         true,
         "Spark editor",
         "Dock shell, hierarchy, inspector, undo",
         "24",
         -1,
         'E'},
#endif
};

static_assert(
        sizeof(kEntries) / sizeof(kEntries[0]) == static_cast<std::size_t>(DemoStorageId::Count),
        "kEntries must cover every DemoStorageId");

char gLauncherRowLabel[96]{};

}  // namespace

const DemoCatalogEntry& DemoCatalog::Entry(const DemoStorageId id) noexcept {
    return kEntries[static_cast<std::size_t>(id)];
}

const DemoCatalogEntry& DemoCatalog::EntryByStorageIndex(const std::size_t storageIndex) noexcept {
    return Entry(static_cast<DemoStorageId>(storageIndex));
}

DemoStorageId DemoCatalog::LauncherStorageId(const std::size_t rowIndex) noexcept {
    return static_cast<DemoStorageId>(rowIndex);
}

const char* DemoCatalog::CategoryLabel(const DemoCategory category) noexcept {
    switch (category) {
        case DemoCategory::Graphics3D:
            return "3D Graphics";
        case DemoCategory::Gameplay3D:
            return "3D Gameplay";
        case DemoCategory::Gameplay2D:
            return "2D Gameplay";
        case DemoCategory::Tools:
            return "Tools";
        case DemoCategory::Arcade:
            return "Arcade";
    }
    return "Other";
}

const char* DemoCatalog::LauncherRowLabel(const std::size_t rowIndex) noexcept {
    if (rowIndex >= StorageCount()) {
        return "";
    }
    const DemoCatalogEntry& e = EntryByStorageIndex(rowIndex);
    const char* mark = e.recommended ? "*" : " ";
    std::snprintf(
            gLauncherRowLabel,
            sizeof(gLauncherRowLabel),
            "%s %2s  %s",
            mark,
            e.menuNumber,
            e.title);
    return gLauncherRowLabel;
}

bool DemoCatalog::TryResolveDigitHotkey(const int glfwDigitKey, DemoStorageId& outId) noexcept {
    int digit = -1;
    if (glfwDigitKey >= GLFW_KEY_1 && glfwDigitKey <= GLFW_KEY_9) {
        digit = glfwDigitKey - GLFW_KEY_1 + 1;
    } else if (glfwDigitKey == GLFW_KEY_0) {
        digit = 10;
    } else {
        return false;
    }
    for (std::size_t i = 0; i < StorageCount(); ++i) {
        const DemoCatalogEntry& e = kEntries[i];
        if (e.digitHotkey == digit) {
            outId = e.storageId;
            return true;
        }
    }
    return false;
}

bool DemoCatalog::TryResolveLetterHotkey(const int glfwKey, DemoStorageId& outId) noexcept {
    char letter = '\0';
    if (glfwKey >= GLFW_KEY_A && glfwKey <= GLFW_KEY_Z) {
        letter = static_cast<char>('A' + (glfwKey - GLFW_KEY_A));
    } else if (glfwKey == GLFW_KEY_SEMICOLON) {
        letter = ';';
    } else {
        return false;
    }
    for (std::size_t i = 0; i < StorageCount(); ++i) {
        const DemoCatalogEntry& e = kEntries[i];
        if (e.letterHotkey == letter) {
            outId = e.storageId;
            return true;
        }
    }
    return false;
}

}  // namespace Spark
