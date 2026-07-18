#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoProceduralSound.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

#include <algorithm>
#include <cstdint>

namespace Spark {

namespace Detail {

/**
 * Cell offsets (ox, oy) from anchor; grid row 0 is the bottom of the well, oy increases toward the top.
 * Order: I, O, T, S, Z, J, L — four clockwise rotations each.
 */
constexpr std::int8_t kPieceCells[7][4][4][2] = {
        // I
        { {{0, 0}, {1, 0}, {2, 0}, {3, 0}},
          {{1, 0}, {1, 1}, {1, 2}, {1, 3}},
          {{0, 0}, {1, 0}, {2, 0}, {3, 0}},
          {{2, 0}, {2, 1}, {2, 2}, {2, 3}} },
        // O
        { {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
          {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
          {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
          {{0, 0}, {1, 0}, {0, 1}, {1, 1}} },
        // T
        { {{1, 0}, {0, 1}, {1, 1}, {2, 1}},
          {{0, 0}, {1, 0}, {1, 1}, {1, 2}},
          {{0, 1}, {1, 1}, {2, 1}, {1, 2}},
          {{1, 0}, {1, 1}, {2, 1}, {1, 2}} },
        // S
        { {{1, 0}, {2, 0}, {0, 1}, {1, 1}},
          {{1, 0}, {1, 1}, {2, 1}, {2, 2}},
          {{1, 1}, {2, 1}, {0, 2}, {1, 2}},
          {{0, 0}, {0, 1}, {1, 1}, {1, 2}} },
        // Z
        { {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
          {{2, 0}, {1, 1}, {2, 1}, {1, 2}},
          {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
          {{1, 0}, {0, 1}, {1, 1}, {0, 2}} },
        // J
        { {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
          {{1, 0}, {2, 0}, {1, 1}, {1, 2}},
          {{0, 1}, {1, 1}, {2, 1}, {2, 2}},
          {{1, 0}, {1, 1}, {0, 2}, {1, 2}} },
        // L
        { {{2, 0}, {0, 1}, {1, 1}, {2, 1}},
          {{1, 0}, {1, 1}, {1, 2}, {2, 2}},
          {{0, 1}, {1, 1}, {2, 1}, {0, 2}},
          {{0, 0}, {1, 0}, {1, 1}, {1, 2}} },
};
[[nodiscard]] Spark::SharedPtr<Spark::Texture2D> MakeTetrisBlockAtlas();

}  // namespace Detail

/**
 * Classic-style Tetris using Camera2D + TilemapComponent (colored atlas tiles) and TextOverlay HUD.
 * Controls: arrows move/soft drop, Up / X rotate CW, Space hard drop, R restart when game over.
 */
class Tetris2DDemo {
public:
    static constexpr int kCols = 10;
    static constexpr int kRows = 22;
    static constexpr float kTileWorld = 1.0F;

    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);


    void Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    static std::uint32_t timingHackU32(Spark::IEngineContext& context);


    void RandSeed(std::uint32_t s) noexcept;

    [[nodiscard]] std::uint32_t RandU32() noexcept;

    [[nodiscard]] int RandPieceKind() noexcept;


    void ResetGame();


    void SpawnPiece();


    [[nodiscard]] bool Fits(int kind, int rot, int ax, int ay) const noexcept;


    [[nodiscard]] bool TryMove(int dx, int dy) noexcept;


    void TryRotate(Spark::IEngineContext& context) noexcept;


    void LockPiece(Spark::IEngineContext& context);


    [[nodiscard]] int ClearLines();


    void PushBoardToTilemap();

    void UpdateGhostSprites();

    void UpdateBoardPad();

    [[nodiscard]] static Spark::Vector4 TileUv(std::uint16_t tileId) noexcept;


    Spark::Array<Spark::GameObject*> roots{};
    Spark::Camera2D camera{};
    Spark::SharedPtr<Spark::Texture2D> atlasTex{};
    Spark::GameObject* boardGo = nullptr;
    Spark::TilemapComponent* tilemap = nullptr;
    Spark::GameObject* fpsHudObject = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    Spark::GameObject* boardPadGo = nullptr;
    Spark::Array<Spark::GameObject*> ghostCells{};
    Spark::GameObject* lineFlashGo = nullptr;
    float lineFlashT = 0.0F;
    float fpsSmoothed = 0.0F;

    Spark::Array<std::uint8_t> grid{};
    int anchorX = 0;
    int anchorY = 0;
    int curKind = 0;
    int curRot = 0;
    float fallAccum = 0.0F;
    int score = 0;
    int linesCleared = 0;
    int level = 1;
    bool gameOver = false;
    std::uint32_t rng = 1;

};

}  // namespace Spark
