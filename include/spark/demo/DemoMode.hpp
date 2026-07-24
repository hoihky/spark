#pragma once

#include "spark/math/Constants.hpp"

#include <cstdint>

namespace Spark {

/** Uniform scale for unit cube [-1,1]³; local translation.y = kCubeScale places bottom on y=0. */
constexpr float kCubeScale = 1.0F;
constexpr float kCubeSpinRadPerSec = TwoPi * 0.35F;

enum class DemoMode : std::uint8_t {
    Menu,
    ThreeD,
    Sky,
    Particles,
    Terrain,
    Character,
    Platformer2D,
    BroadPhase2D,
    Maze3D,
    PhysicsBall3D,
    SteeringShowcase3D,
    SceneEditor3D,
    ToonShading,
    MaterialShowcase3D,
    TimeOfDay,
    Tetris2D,
    Connect3,
    SpaceInvaders2D,
    RenderLayers2D,
    TilemapShowcase2D,
    ImGuiShowcase,
};

}  // namespace Spark
