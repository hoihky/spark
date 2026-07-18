#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoProceduralSound.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/BlendModeComponent.hpp"
#include "spark/ecs/components/SpriteComponent.hpp"
#include "spark/ecs/components/TextOverlayComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace Spark {

namespace Detail {

[[nodiscard]] inline Spark::SharedPtr<Spark::Texture2D> MakeSpaceInvadersAtlas() {
    constexpr std::uint32_t tw = 16;
    constexpr std::uint32_t au = 2;
    constexpr std::uint32_t av = 2;
    constexpr std::uint32_t w = au * tw;
    constexpr std::uint32_t h = av * tw;
    Spark::Texture2D tex(Spark::Utf8String("SpaceInvadersAtlas"));
    Spark::Array<std::uint8_t> px;
    px.Resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
    const Spark::Vector3 palette[4] = {
            {0.35F, 0.92F, 0.42F},
            {0.78F, 0.38F, 0.95F},
            {0.35F, 0.88F, 0.98F},
            {0.98F, 0.92F, 0.28F},
    };
    for (std::uint32_t ty = 0; ty < av; ++ty) {
        for (std::uint32_t tx = 0; tx < au; ++tx) {
            const std::uint32_t id = ty * au + tx;
            const Spark::Vector3& c = palette[std::min<std::uint32_t>(id, 3U)];
            for (std::uint32_t py = 0; py < tw; ++py) {
                for (std::uint32_t px0 = 0; px0 < tw; ++px0) {
                    const float fx = (static_cast<float>(px0) + 0.5F) / static_cast<float>(tw) - 0.5F;
                    const float fy = (static_cast<float>(py) + 0.5F) / static_cast<float>(tw) - 0.5F;
                    const float r = std::sqrt(fx * fx + fy * fy);
                    Spark::Vector3 out = c;
                    if (r > 0.46F) {
                        out = out * 0.45F;
                    }
                    const std::uint32_t gx = tx * tw + px0;
                    const std::uint32_t gy = ty * tw + py;
                    const std::size_t di = (static_cast<std::size_t>(gy) * w + gx) * 4U;
                    px[di] = static_cast<std::uint8_t>(std::min(255.0F, out.x * 255.0F));
                    px[di + 1U] = static_cast<std::uint8_t>(std::min(255.0F, out.y * 255.0F));
                    px[di + 2U] = static_cast<std::uint8_t>(std::min(255.0F, out.z * 255.0F));
                    px[di + 3U] = 255;
                }
            }
        }
    }
    tex.SetPixels(w, h, Spark::MoveTemp(px));
    return Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tex));
}

[[nodiscard]] inline Spark::Vector4 AtlasUv(std::uint32_t cell) noexcept {
    const std::uint32_t c = cell % 4U;
    const float u0 = static_cast<float>(c % 2U) * 0.5F;
    const float v0 = static_cast<float>(c / 2U) * 0.5F;
    return {u0, v0, u0 + 0.5F, v0 + 0.5F};
}

}  // namespace Detail

/**
 * Minimal Space Invaders–style shooter: Camera2D + sprite atlas, fleet sidestep + descent, player shots,
 * random enemy return fire. World +Y is **up** on screen here (player uses **small** y at the bottom, fleet
 * uses **large** y at the top). Arrows move, Space shoots, R restarts. ESC returns to menu (handled by shell).
 */
class SpaceInvaders2DDemo {
public:
    static constexpr float kWorldW = 24.0F;
    static constexpr float kWorldH = 26.0F;
    static constexpr int kAlienCols = 10;
    static constexpr int kAlienRows = 4;
    static constexpr int kMaxPlayerBullets = 14;
    static constexpr int kMaxEnemyBullets = 10;

    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);


    void Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    struct AlienSlot {
        Spark::GameObject* go = nullptr;
        Spark::TransformComponent* tr = nullptr;
        bool alive = false;
        int gi = 0;
        int gj = 0;
    };

    struct BulletSlot {
        bool active = false;
        Spark::GameObject* go = nullptr;
        Spark::TransformComponent* tr = nullptr;
        Spark::SpriteComponent* spr = nullptr;
        float cx = 0.0F;
        float cy = 0.0F;
        float vx = 0.0F;
        float vy = 0.0F;
    };

    struct ExplosionSlot {
        bool active = false;
        Spark::GameObject* go = nullptr;
        Spark::TransformComponent* tr = nullptr;
        Spark::SpriteComponent* spr = nullptr;
        float timeLeft = 0.0F;
    };

    static std::uint32_t timingHackU32(Spark::IEngineContext& context);


    void RandSeed(std::uint32_t s) noexcept;

    [[nodiscard]] std::uint32_t RandU32() noexcept;


    void ResetRound() noexcept;


    void SyncAlienTransforms() noexcept;


    static void DeactivateBullet(BulletSlot& b) noexcept;


    [[nodiscard]] bool TrySpawnPlayerBullet() noexcept;


    void TrySpawnEnemyBullet() noexcept;


    static bool BoxOverlap(float ax, float ay, float ahx, float ahy, float bx, float by, float bhx, float bhy) noexcept;


    [[nodiscard]] static float AlienWorldY(float fleetYVal, int gj) noexcept;


    void ResolveCollisions(Spark::IEngineContext& context) noexcept;

    void UpdatePlayerShadow() noexcept;

    void SpawnExplosion(float worldX, float worldY) noexcept;

    void TickExplosions(float dt) noexcept;


    static constexpr float kStepX = 1.75F;
    static constexpr float kStepY = 1.15F;
    /** Aliens that reach this y or below (near the player) end the game. */
    static constexpr float kLoseLineY = 3.85F;

    Spark::Array<Spark::GameObject*> roots{};
    Spark::Camera2D camera{};
    Spark::SharedPtr<Spark::Texture2D> atlasTex{};
    Spark::GameObject* hudGo = nullptr;
    Spark::TextOverlayComponent* hudText = nullptr;
    Spark::GameObject* playerGo = nullptr;
    Spark::TransformComponent* playerTr = nullptr;
    Spark::GameObject* playerShadowGo = nullptr;
    Spark::Array<AlienSlot> aliens{};
    Spark::Array<BulletSlot> playerBullets{};
    Spark::Array<BulletSlot> enemyBullets{};
    Spark::Array<ExplosionSlot> explosions{};

    float playerX = 0.0F;
    float playerY = 2.45F;
    float fleetX = 0.0F;
    float fleetY = 0.0F;
    float fleetVelX = 4.2F;
    float fireCooldown = 0.0F;
    float enemyFireTimer = 0.4F;
    int score = 0;
    int lives = 3;
    int gamePhase = 0;
    float fpsSmoothed = 0.0F;
    std::uint32_t rng = 1;

};

}  // namespace Spark
