#include "spark/demo/platformer2d/Platformer2DSpawnHost.hpp"

#include "spark/demo/DemoFoundation.hpp"
#include "spark/demo/Platformer2DDemo.hpp"
#include "spark/demo/platformer2d/Platformer2DConfig.hpp"
#include "spark/ecs/components/gameplay/GameFlowTriggerComponent.hpp"
#include "spark/ecs/components/gameplay/PickupComponent.hpp"
#include "spark/ecs/components/physics/2d/TriggerVolume2DComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/scene/tilemap/TilemapObjectSpawnRegistry.hpp"

#include <cmath>

namespace Spark {

namespace {

Platformer2DSpawnHostContext g_context{};

Spark::Vector2 MarkerWorldXY(const GameObject& mapOwner, const TilemapObjectMarker& marker, const TilemapGridFrame& frame)
{
    const GridPathfinder::Cell cell{marker.cellX, marker.cellY};
    Spark::Vector2 world = frame.CellCenterToWorldXY(cell);
    world.x += (marker.offsetX - 0.5F) * frame.cellSize;
    world.y += (marker.offsetY - 0.5F) * frame.cellSize;
    return world;
}

int FindEnemySpawnIndex(const float worldX, const float worldY) noexcept {
    for (int ei = 0; ei < Platformer2D::Config::kEnemyCount; ++ei) {
        const float ex = Platformer2D::Config::kEnemySpawns[static_cast<std::size_t>(ei)][0];
        const float ey = Platformer2D::Config::kEnemySpawns[static_cast<std::size_t>(ei)][1];
        if (std::abs(ex - worldX) < 0.05F && std::abs(ey - worldY) < 0.05F) {
            return ei;
        }
    }
    return -1;
}

}  // namespace

void Platformer2DSpawnHost::SetContext(const Platformer2DSpawnHostContext& ctx) noexcept {
    g_context = ctx;
}

GameObject* Platformer2DSpawnHost::SpawnGem(
        GameWorld& world,
        GameObject& mapOwner,
        const TilemapObjectMarker& marker,
        const TilemapGridFrame& frame) {
    Platformer2DDemo* demo = g_context.demo;
    if (demo == nullptr || !g_context.gemTexture) {
        return nullptr;
    }
    const Spark::Vector2 pos = MarkerWorldXY(mapOwner, marker, frame);
    const std::size_t gi = demo->gemObjects.GetSize();
    GameObject* gem = world.CreateGameObject();
    gem->GetName() = Utf8String("PlatGem");
    TransformComponent* gtr = gem->AddComponent<TransformComponent>();
    gtr->SetTranslation({pos.x, pos.y, 0.05F + 0.0003F * static_cast<float>(gi)});
    gtr->SetScale({Platformer2DDemo::kGemDrawScale, Platformer2DDemo::kGemDrawScale, 1.0F});
    gem->AddComponent<SpriteComponent>(
            g_context.gemTexture,
            Vector4{1.0F, 1.0F, 1.0F, 1.0F},
            Vector4{0.0F, 0.0F, 1.0F, 1.0F},
            620 + static_cast<int>(gi));
    gem->AddComponent<TriggerVolume2DComponent>(
            TriggerVolume2DShape::Circle,
            Vector2{0.5F, 0.5F},
            Vector2::Zero);
    if (TriggerVolume2DComponent* trigger = gem->GetComponent<TriggerVolume2DComponent>()) {
        trigger->SetRadius(0.55F);
    }
    auto* pickup = gem->AddComponent<PickupComponent>();
    pickup->SetItemId("gem");
    pickup->SetOnCollected([demo, gem](GameObject& /*collector*/, const char*, int) {
        if (gem != nullptr && demo != nullptr) {
            if (TransformComponent* gtrLocal = gem->GetComponent<TransformComponent>()) {
                const Vector3 gpos = gtrLocal->GetLocalTransform().translation;
                demo->explosions.SpawnGemPickup(gpos.x, gpos.y);
            }
            for (std::size_t idx = 0; idx < demo->gemObjects.GetSize(); ++idx) {
                if (demo->gemObjects[idx] == gem) {
                    demo->gemObjects[idx] = nullptr;
                    break;
                }
            }
        }
        if (demo != nullptr) {
            ++demo->gemsCollected;
            if (demo->playerObject != nullptr && demo->sfxCoin.Get() != nullptr) {
                DemoAudio::QueueCue(*demo->playerObject, demo->sfxCoin, 0.95F);
            } else if (demo->engineContext != nullptr) {
                DemoPlayProceduralClip(*demo->engineContext, DemoSfx::ClipGemCollect(), 0.88F);
            }
        }
    });
    demo->gemObjects.PushBack(gem);
    demo->gemBasePositions.PushBack({pos.x, pos.y});
    demo->roots.Track(gem);
    return gem;
}

GameObject* Platformer2DSpawnHost::SpawnEnemy(
        GameWorld& world,
        GameObject& mapOwner,
        const TilemapObjectMarker& marker,
        const TilemapGridFrame& frame) {
    Platformer2DDemo* demo = g_context.demo;
    if (demo == nullptr || !demo->enemyAtlasTex) {
        return nullptr;
    }
    const Spark::Vector2 pos = MarkerWorldXY(mapOwner, marker, frame);
    const int ei = FindEnemySpawnIndex(pos.x, pos.y);
    float pMin = pos.x - 2.0F;
    float pMax = pos.x + 2.0F;
    if (ei >= 0) {
        pMin = Platformer2D::Config::kEnemySpawns[static_cast<std::size_t>(ei)][2];
        pMax = Platformer2D::Config::kEnemySpawns[static_cast<std::size_t>(ei)][3];
    }
    GameObject* ego = demo->enemySquad.SpawnAt(
            world,
            demo->enemyAtlasTex,
            pos.x,
            pos.y,
            pMin,
            pMax,
            710 + ei);
    if (ego != nullptr) {
        demo->roots.Track(ego);
    }
    return ego;
}

GameObject* Platformer2DSpawnHost::SpawnGoal(
        GameWorld& world,
        GameObject& mapOwner,
        const TilemapObjectMarker& marker,
        const TilemapGridFrame& frame) {
    Platformer2DDemo* demo = g_context.demo;
    if (demo == nullptr || g_context.gameFlowObject == nullptr) {
        return nullptr;
    }
    const Spark::Vector2 pos = MarkerWorldXY(mapOwner, marker, frame);
    if (demo->goalTriggerGo != nullptr) {
        world.DestroyGameObject(demo->goalTriggerGo);
        demo->goalTriggerGo = nullptr;
    }
    demo->goalTriggerGo = world.CreateGameObject();
    demo->goalTriggerGo->GetName() = Utf8String("PlatGoalTrigger");
    TransformComponent* goalTriggerTr = demo->goalTriggerGo->AddComponent<TransformComponent>();
    goalTriggerTr->SetTranslation({pos.x, pos.y, 0.0F});
    demo->goalTriggerGo->AddComponent<TriggerVolume2DComponent>(
            TriggerVolume2DShape::Box,
            Vector2{g_context.goalHalfW, g_context.goalHalfH});
    auto* goalFlow = demo->goalTriggerGo->AddComponent<GameFlowTriggerComponent>();
    goalFlow->SetSource(GameFlowTriggerSource::TriggerEnter);
    goalFlow->SetTargetState(GameFlowState::Victory);
    goalFlow->SetInstigatorNameFilter("Player");
    goalFlow->SetStateOwner(g_context.gameFlowObject);
    demo->roots.Track(demo->goalTriggerGo);
    return demo->goalTriggerGo;
}

void Platformer2DSpawnHost::RegisterHandlers() noexcept {
    TilemapObjectSpawnRegistry::Default().Register("gem", &Platformer2DSpawnHost::SpawnGem);
    TilemapObjectSpawnRegistry::Default().Register("enemy", &Platformer2DSpawnHost::SpawnEnemy);
    TilemapObjectSpawnRegistry::Default().Register("goal", &Platformer2DSpawnHost::SpawnGoal);
}

void Platformer2DSpawnHost::UnregisterHandlers() noexcept {
    TilemapObjectSpawnRegistry::Default().Unregister("gem");
    TilemapObjectSpawnRegistry::Default().Unregister("enemy");
    TilemapObjectSpawnRegistry::Default().Unregister("goal");
}

}  // namespace Spark
