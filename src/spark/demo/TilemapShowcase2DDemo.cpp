#include "spark/demo/TilemapShowcase2DDemo.hpp"

#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/scene/tilemap/TilemapGameplayGrid.hpp"
#include "spark/scene/tilemap/TilemapLayer.hpp"
#include "spark/ecs/components/tilemap/TilemapAutotileComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectGizmoComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp"
#include "spark/scene/tilemap/TilemapObjectQuery.hpp"
#include "spark/scene/tilemap/TilemapObjectSpawnRegistry.hpp"
#include "spark/scene/tilemap/TilemapFileResolve.hpp"
#include "spark/scene/tilemap/TilemapLayerSortMode.hpp"
#include "spark/scene/tilemap/Tileset.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Spark {
namespace Detail {

[[nodiscard]] Spark::SharedPtr<Spark::Texture2D> MakeShowcaseAtlas() {
    constexpr std::uint32_t tw = 16;
    constexpr std::uint32_t au = 4;
    constexpr std::uint32_t av = 4;
    constexpr std::uint32_t w = au * tw;
    constexpr std::uint32_t h = av * tw;
    Spark::Texture2D tex(Spark::Utf8String("TilemapShowcaseAtlas"));
    Spark::Array<std::uint8_t> px;
    px.Resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
    const Spark::Vector3 palette[8] = {
            {0.32F, 0.72F, 0.28F},
            {0.22F, 0.58F, 0.20F},
            {0.45F, 0.36F, 0.28F},
            {0.18F, 0.48F, 0.92F},
            {0.12F, 0.38F, 0.82F},
            {0.12F, 0.42F, 0.16F},
            {0.95F, 0.82F, 0.22F},
            {0.92F, 0.35F, 0.42F},
    };
    for (std::uint32_t ty = 0; ty < av; ++ty) {
        for (std::uint32_t tx = 0; tx < au; ++tx) {
            const std::uint32_t id = ty * au + tx;
            const Spark::Vector3 c = palette[std::min(id, 7U)];
            for (std::uint32_t py = 0; py < tw; ++py) {
                for (std::uint32_t px0 = 0; px0 < tw; ++px0) {
                    const std::uint32_t gx = tx * tw + px0;
                    const std::uint32_t gy = ty * tw + py;
                    const std::size_t di = (static_cast<std::size_t>(gy) * w + gx) * 4U;
                    px[di] = static_cast<std::uint8_t>(std::min(255.0F, c.x * 255.0F));
                    px[di + 1U] = static_cast<std::uint8_t>(std::min(255.0F, c.y * 255.0F));
                    px[di + 2U] = static_cast<std::uint8_t>(std::min(255.0F, c.z * 255.0F));
                    px[di + 3U] = 255;
                }
            }
        }
    }
    tex.SetPixels(w, h, Spark::MoveTemp(px));
    return Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tex));
}

[[nodiscard]] Spark::Vector4 TileUv(const std::uint16_t tileId) noexcept {
    constexpr std::uint32_t au = 4;
    constexpr std::uint32_t av = 4;
    const std::uint32_t tx = static_cast<std::uint32_t>(tileId) % au;
    const std::uint32_t ty = static_cast<std::uint32_t>(tileId) / au;
    const float du = 1.0F / static_cast<float>(au);
    const float dv = 1.0F / static_cast<float>(av);
    return {static_cast<float>(tx) * du, static_cast<float>(ty) * dv, static_cast<float>(tx + 1U) * du,
            static_cast<float>(ty + 1U) * dv};
}

void ConfigureTileset(Spark::Tileset& tileset) {
    tileset.Definition(TilemapShowcase2DDemo::kTileGrassPaint).autotileGroup = 1;
    tileset.Definition(TilemapShowcase2DDemo::kTileGrassPaint).collisionShape = TileCollisionShape::None;

    tileset.Definition(TilemapShowcase2DDemo::kTileGrassEdge).autotileGroup = 1;
    tileset.Definition(TilemapShowcase2DDemo::kTileGrassEdge).collisionShape = TileCollisionShape::None;

    tileset.Definition(TilemapShowcase2DDemo::kTileWall).collisionShape = TileCollisionShape::FullCell;
    tileset.Definition(TilemapShowcase2DDemo::kTileWall).flags = TileDefinitionFlags::BlocksPathfinding;

    tileset.Definition(TilemapShowcase2DDemo::kTileWaterA).collisionShape = TileCollisionShape::None;
    tileset.Definition(TilemapShowcase2DDemo::kTileWaterA).flags = TileDefinitionFlags::BlocksPathfinding;
    tileset.Definition(TilemapShowcase2DDemo::kTileWaterB).collisionShape = TileCollisionShape::None;
    tileset.Definition(TilemapShowcase2DDemo::kTileWaterB).flags = TileDefinitionFlags::BlocksPathfinding;

    tileset.Definition(TilemapShowcase2DDemo::kTileProp).collisionShape = TileCollisionShape::None;

    TileAnimationClip waterClip{};
    waterClip.loop = true;
    TileAnimationFrame f0{};
    f0.tileId = TilemapShowcase2DDemo::kTileWaterA;
    f0.durationSeconds = 0.35F;
    TileAnimationFrame f1{};
    f1.tileId = TilemapShowcase2DDemo::kTileWaterB;
    f1.durationSeconds = 0.35F;
    waterClip.frames.PushBack(f0);
    waterClip.frames.PushBack(f1);
    tileset.GetAnimationClips().PushBack(waterClip);
    tileset.Definition(TilemapShowcase2DDemo::kTileWaterA).animationClipIndex = 0;

    TileAutotileRuleSet& grass = tileset.GetOrCreateAutotileRuleSet(1);
    for (std::uint8_t mask = 0; mask < 16; ++mask) {
        grass.SetVariant(mask, TilemapShowcase2DDemo::kTileGrassPaint);
    }
    grass.SetVariant(0, TilemapShowcase2DDemo::kTileGrassEdge);
    grass.SetVariant(1, TilemapShowcase2DDemo::kTileGrassEdge);
    grass.SetVariant(2, TilemapShowcase2DDemo::kTileGrassEdge);
    grass.SetVariant(4, TilemapShowcase2DDemo::kTileGrassEdge);
    grass.SetVariant(8, TilemapShowcase2DDemo::kTileGrassEdge);
    grass.SetVariant(15, TilemapShowcase2DDemo::kTileGrassPaint);
}

struct ShowcaseSpawnContext {
    SharedPtr<Texture2D> atlas{};
};

ShowcaseSpawnContext g_showcaseSpawn{};

GameObject* SpawnShowcaseChest(
        GameWorld& world,
        GameObject& /*mapOwner*/,
        const TilemapObjectMarker& marker,
        const TilemapGridFrame& frame) {
    if (!g_showcaseSpawn.atlas) {
        return nullptr;
    }
    GameObject* chest = world.CreateGameObject();
    chest->GetName() = marker.name.IsEmpty() ? Utf8String("Chest") : marker.name;
    const Vector3 pos = TilemapObjectMarkerWorldPosition(marker, frame);
    if (TransformComponent* tr = chest->AddComponent<TransformComponent>()) {
        tr->SetTranslation({pos.x, pos.y, 0.09F});
        tr->SetUniformScale(TilemapShowcase2DDemo::kTileWorld * 0.75F);
    }
    chest->AddComponent<SpriteComponent>(
            g_showcaseSpawn.atlas,
            Vector4{1.0F, 0.92F, 0.45F, 1.0F},
            TileUv(TilemapShowcase2DDemo::kTileProp),
            48);
    return chest;
}

void RegisterShowcaseSpawnHandlers() {
    TilemapObjectSpawnRegistry::Register("chest", &SpawnShowcaseChest);
}

void UnregisterShowcaseSpawnHandlers() {
    TilemapObjectSpawnRegistry::Unregister("chest");
}

[[nodiscard]] Utf8String ResolveSampleTmxPath() noexcept {
    static const char* kRelative = "sprites/kenney_tiny-dungeon/Tiled/sampleMap.tmx";
    return ResolveTilemapAssetPath(kRelative);
}

void PlacePlayerOnFirstWalkableCell(
        const Spark::TilemapGameplayGridComponent& gameplayGrid,
        Spark::Vector2& inOutPlayerPos) {
    const Spark::TilemapGameplayGrid& walk = gameplayGrid.GetGrid();
    const Spark::TilemapGridFrame& frame = gameplayGrid.GetGridFrame();
    for (std::int32_t y = 0; y < walk.Height(); ++y) {
        for (std::int32_t x = 0; x < walk.Width(); ++x) {
            if (!walk.IsWalkable(x, y)) {
                continue;
            }
            inOutPlayerPos = frame.CellCenterToWorldXY({x, y});
            return;
        }
    }
}

}  // namespace Detail

void TilemapShowcase2DDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context) {
    roots.Clear();
    atlasTex = Detail::MakeShowcaseAtlas();
    w.RegisterTexture(atlasTex, "spark/tilemap_showcase/atlas");

    boardGo = w.CreateGameObject();
    boardGo->GetName() = Spark::Utf8String("TilemapShowcaseBoard");
    boardGo->AddComponent<Spark::TransformComponent>();
    tilemap = boardGo->AddComponent<Spark::TilemapComponent>(
            atlasTex,
            static_cast<std::uint32_t>(kCols),
            static_cast<std::uint32_t>(kRows),
            4,
            4,
            kTileWorld,
            0);
    Detail::ConfigureTileset(*tilemap->GetTileset());

    static_cast<void>(tilemap->AddLayer("Water"));
    static_cast<void>(tilemap->AddLayer("Props"));
    tilemap->GetLayer(1).orderInLayerOffset = 8;
    tilemap->GetLayer(1).contributeCollision = false;
    tilemap->GetLayer(2).orderInLayerOffset = 16;
    tilemap->GetLayer(2).contributeCollision = false;
    tilemap->GetLayer(2).contributeGameplayGrid = false;
    tilemap->GetLayer(2).sortMode = TilemapLayerSortMode::WorldY;

    boardGo->AddComponent<Spark::TilemapTileAnimatorComponent>();
    auto* autotile = boardGo->AddComponent<Spark::TilemapAutotileComponent>();
    autotile->SetLayerIndex(0);

    gameplayGrid = boardGo->AddComponent<Spark::TilemapGameplayGridComponent>();
    gameplayGrid->SetWalkRule(TilemapGameplayWalkRule::DefinitionAndFlags);
    gameplayGrid->SetAutoRebake(true);

    auto* objectLayer = boardGo->AddComponent<Spark::TilemapObjectLayerComponent>();
    const std::uint32_t objectLayerIndex = objectLayer->AddObjectLayer("GameplayObjects");

    if (auto* gizmo = boardGo->AddComponent<Spark::TilemapObjectGizmoComponent>()) {
        gizmo->SetGizmoTexture(atlasTex);
        gizmo->SetGizmoUvRect(Detail::TileUv(kTilePlayerIcon));
        gizmo->SetGizmoTint(Spark::Vector4{0.92F, 0.35F, 0.95F, 0.82F});
    }

    Detail::g_showcaseSpawn.atlas = atlasTex;
    Detail::RegisterShowcaseSpawnHandlers();

    boardGo->AddComponent<Spark::TilemapCollider2DComponent>();
    roots.PushBack(boardGo);

    playerGo = w.CreateGameObject();
    playerGo->GetName() = Spark::Utf8String("TilemapShowcasePlayer");
    {
        Spark::TransformComponent* tr = playerGo->AddComponent<Spark::TransformComponent>();
        tr->SetTranslation({playerPos.x, playerPos.y, 0.08F});
        tr->SetUniformScale(kTileWorld * 0.85F);
    }
    playerGo->AddComponent<Spark::SpriteComponent>(
            atlasTex, Spark::Vector4{1, 1, 1, 1}, Detail::TileUv(kTilePlayerIcon), 50);
    roots.PushBack(playerGo);

    pathMarkers.Reserve(48);
    for (int i = 0; i < 48; ++i) {
        Spark::GameObject* m = w.CreateGameObject();
        m->GetName() = Spark::Utf8String("PathMarker");
        m->AddComponent<Spark::TransformComponent>()->SetTranslation({-50.0F, -50.0F, 0.0F});
        m->AddComponent<Spark::SpriteComponent>(
                atlasTex,
                Spark::Vector4{0.95F, 0.9F, 0.35F, 0.55F},
                Detail::TileUv(kTilePlayerIcon),
                40);
        pathMarkers.PushBack(m);
        roots.PushBack(m);
    }

    hudGo = w.CreateGameObject();
    hudGo->GetName() = Spark::Utf8String("TilemapShowcaseHud");
    hudText = hudGo->AddComponent<Spark::TextOverlayComponent>();
    hudText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
    DemoHud::Apply(*hudText, false);
    roots.PushBack(hudGo);

    camera.position = {static_cast<float>(kCols) * 0.5F * kTileWorld, static_cast<float>(kRows) * 0.5F * kTileWorld, 0.0F};
    camera.halfExtentY = static_cast<float>(kRows) * 0.55F * kTileWorld;

    BuildLevel();
    if (objectLayer != nullptr) {
        BuildObjectMarkers(*objectLayer, objectLayerIndex);
        boardGo->AddComponent<Spark::TilemapObjectSpawnComponent>();
        if (auto* gizmo = boardGo->GetComponent<Spark::TilemapObjectGizmoComponent>()) {
            gizmo->RebuildVisuals(*boardGo, w);
        }
    }
    pathTarget = playerPos;
    context.GetInput().SetCursorCaptured(false);
}

void TilemapShowcase2DDemo::Unload(Spark::GameWorld& w) {
    Detail::UnregisterShowcaseSpawnHandlers();
    Detail::g_showcaseSpawn.atlas.Reset();
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            w.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    pathMarkers.Clear();
    atlasTex.Reset();
    boardGo = nullptr;
    tilemap = nullptr;
    gameplayGrid = nullptr;
    playerGo = nullptr;
    hudGo = nullptr;
    hudText = nullptr;
    pathCells.Clear();
    pathStep = 0;
}

void TilemapShowcase2DDemo::RestoreProceduralTilemap() {
    if (tilemap == nullptr || !atlasTex) {
        return;
    }

    tilemap->Resize(static_cast<std::uint32_t>(kCols), static_cast<std::uint32_t>(kRows));
    tilemap->SetTileWorldSize(kTileWorld);

    SharedPtr<Tileset> showcaseTileset = CreateTilesetFromAtlas(atlasTex, 4U, 4U);
    Detail::ConfigureTileset(*showcaseTileset);
    tilemap->SetTileset(showcaseTileset);

    while (tilemap->GetLayerCount() > 3U) {
        tilemap->RemoveLayer(tilemap->GetLayerCount() - 1U);
    }
    while (tilemap->GetLayerCount() < 3U) {
        const char* layerName = tilemap->GetLayerCount() == 1U ? "Water" : "Props";
        static_cast<void>(tilemap->AddLayer(layerName));
    }

    TilemapLayer& terrain = tilemap->GetLayer(0U);
    terrain.name = Utf8String("Terrain");
    terrain.visible = true;
    terrain.orderInLayerOffset = 0;
    terrain.contributeCollision = true;
    terrain.contributeGameplayGrid = true;
    terrain.sortMode = TilemapLayerSortMode::GridOrder;

    TilemapLayer& water = tilemap->GetLayer(1U);
    water.name = Utf8String("Water");
    water.visible = true;
    water.orderInLayerOffset = 8;
    water.contributeCollision = false;
    water.contributeGameplayGrid = true;
    water.sortMode = TilemapLayerSortMode::GridOrder;

    TilemapLayer& props = tilemap->GetLayer(2U);
    props.name = Utf8String("Props");
    props.visible = true;
    props.orderInLayerOffset = 16;
    props.contributeCollision = false;
    props.contributeGameplayGrid = false;
    props.sortMode = TilemapLayerSortMode::WorldY;

    if (boardGo != nullptr) {
        if (auto* source = boardGo->GetComponent<TilemapMapSourceComponent>()) {
            source->SetHotReload(false);
            source->SetTmxPath("");
            source->SetSparkMapPath("");
        }
        if (auto* gizmo = boardGo->GetComponent<TilemapObjectGizmoComponent>()) {
            gizmo->SetGizmoTexture(atlasTex);
            gizmo->SetGizmoUvRect(Detail::TileUv(kTilePlayerIcon));
        }
    }

    tmxStatus.Clear();
    camera.position = {static_cast<float>(kCols) * 0.5F * kTileWorld, static_cast<float>(kRows) * 0.5F * kTileWorld,
            0.0F};
    camera.halfExtentY = static_cast<float>(kRows) * 0.55F * kTileWorld;
}

void TilemapShowcase2DDemo::BuildLevel() {
    if (tilemap == nullptr || boardGo == nullptr) {
        return;
    }
    RestoreProceduralTilemap();
    for (int y = 0; y < kRows; ++y) {
        for (int x = 0; x < kCols; ++x) {
            const bool border = x == 0 || y == 0 || x == kCols - 1 || y == kRows - 1;
            tilemap->SetPaintTile(0, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y),
                    border ? kTileWall : kTileGrassPaint);
            tilemap->SetTileCell(1, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), TileCell::Empty());
            tilemap->SetTileCell(2, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), TileCell::Empty());
        }
    }

    const auto pond = [&](int cx, int cy, int rw, int rh) {
        for (int y = cy; y < cy + rh; ++y) {
            for (int x = cx; x < cx + rw; ++x) {
                if (x <= 0 || y <= 0 || x >= kCols - 1 || y >= kRows - 1) {
                    continue;
                }
                TileCell cell = TileCell::FromTileId(kTileWaterA);
                cell.paintTileId = kTileWaterA;
                tilemap->SetTileCell(1, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), cell);
            }
        }
    };
    pond(8, 4, 5, 3);
    pond(14, 8, 4, 4);

    const auto prop = [&](int x, int y) {
        if (x <= 0 || y <= 0 || x >= kCols - 1 || y >= kRows - 1) {
            return;
        }
        TileCell cell = TileCell::FromTileId(kTileProp);
        cell.paintTileId = kTileProp;
        tilemap->SetTileCell(2, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), cell);
    };
    prop(5, 6);
    prop(6, 9);
    prop(17, 5);
    prop(18, 10);
    prop(11, 11);

    if (Spark::TilemapAutotileComponent* autotile = boardGo->GetComponent<Spark::TilemapAutotileComponent>()) {
        autotile->RequestRebuild();
        autotile->RebuildIfNeeded(*boardGo);
    }
    if (gameplayGrid != nullptr) {
        gameplayGrid->RequestRebake();
        gameplayGrid->RebakeIfNeeded(*boardGo);
    }
}

void TilemapShowcase2DDemo::BuildObjectMarkers(
        Spark::TilemapObjectLayerComponent& objects,
        const std::uint32_t layerIndex) {
    objects.ClearMarkers(layerIndex);

    TilemapObjectMarker quest{};
    quest.name = Utf8String("QuestMarker");
    quest.typeId = Utf8String("quest");
    quest.cellX = 16;
    quest.cellY = 3;
    quest.mode = TilemapObjectMarkerMode::GizmoOnly;
    static_cast<void>(objects.AddMarker(layerIndex, quest));

    TilemapObjectMarker chest{};
    chest.name = Utf8String("LootChest");
    chest.typeId = Utf8String("chest");
    chest.cellX = 10;
    chest.cellY = 10;
    static_cast<void>(objects.AddMarker(layerIndex, chest));

    TilemapObjectMarker chest2 = chest;
    chest2.name = Utf8String("LootChestEast");
    chest2.cellX = 19;
    chest2.cellY = 7;
    static_cast<void>(objects.AddMarker(layerIndex, chest2));
}

void TilemapShowcase2DDemo::ClearPathMarkers() {
    for (std::size_t i = 0; i < pathMarkers.GetSize(); ++i) {
        if (pathMarkers[i] == nullptr) {
            continue;
        }
        if (Spark::TransformComponent* tr = pathMarkers[i]->GetComponent<Spark::TransformComponent>()) {
            tr->SetTranslation({-50.0F, -50.0F, 0.0F});
        }
    }
}

void TilemapShowcase2DDemo::ShowPath(const Array<GridPathfinder::Cell>& cells) {
    ClearPathMarkers();
    const TilemapGridFrame& frame =
            gameplayGrid != nullptr ? gameplayGrid->GetGridFrame() : TilemapGridFrame{};
    const float cellWorld = tilemap != nullptr ? tilemap->GetTileWorldSize() : kTileWorld;
    for (std::size_t i = 0; i < cells.GetSize() && i < pathMarkers.GetSize(); ++i) {
        const Vector2 wp = frame.CellCenterToWorldXY(cells[i]);
        if (Spark::TransformComponent* tr = pathMarkers[i]->GetComponent<Spark::TransformComponent>()) {
            tr->SetTranslation({wp.x, wp.y, 0.04F});
            tr->SetUniformScale(cellWorld * 0.35F);
        }
    }
}

bool TilemapShowcase2DDemo::PickCell(
        Spark::IEngineContext& context,
        const float mx,
        const float my,
        int& outX,
        int& outY) const {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0 || fbH <= 0 || gameplayGrid == nullptr) {
        return false;
    }
    const Spark::Matrix4 vp = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
    Spark::Matrix4 invVp{};
    if (!vp.TryInvert(invVp)) {
        return false;
    }
    Spark::Vector3 ro{};
    Spark::Vector3 rd{};
    if (!TerrainScreenToWorldRay(fbW, fbH, mx, my, invVp, ro, rd)) {
        return false;
    }
    if (std::fabs(rd.z) < 1.0e-5F) {
        return false;
    }
    const float t = -ro.z / rd.z;
    const Spark::Vector2 world{ro.x + rd.x * t, ro.y + rd.y * t};
    const GridPathfinder::Cell cell = gameplayGrid->GetGridFrame().WorldXYToCell(world);
    if (!gameplayGrid->GetGridFrame().IsCellInBounds(cell)) {
        return false;
    }
    outX = cell.x;
    outY = cell.y;
    return true;
}

void TilemapShowcase2DDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context) {
    Spark::IInput& in = context.GetInput();
    const float dt = timing.deltaTimeSeconds;

    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);

    if (in.IsKeyPressedThisFrame(GLFW_KEY_L)) {
        if (boardGo != nullptr) {
            const Utf8String tmxPath = Detail::ResolveSampleTmxPath();
            if (tmxPath.IsEmpty()) {
                tmxStatus = Utf8String("TMX not found (Kenney sampleMap under assets/)");
            } else {
                TilemapMapSourceComponent* source = boardGo->GetComponent<TilemapMapSourceComponent>();
                if (source == nullptr) {
                    source = boardGo->AddComponent<TilemapMapSourceComponent>();
                    source->SetImportOnAttach(false);
                }
                source->SetTmxPath(tmxPath.CStr());
                source->SetSparkMapPath("");
                source->SetPixelsPerWorldUnit(16.0F);
                source->SetHotReload(true);
                if (source->ImportNow(*boardGo, boardGo->GetWorld())) {
                    tilemap = boardGo->GetComponent<TilemapComponent>();
                    if (auto* autotile = boardGo->GetComponent<TilemapAutotileComponent>()) {
                        autotile->SetRebuildOnUpdate(false);
                    }
                    if (auto* gizmo = boardGo->GetComponent<TilemapObjectGizmoComponent>()) {
                        if (tilemap != nullptr && tilemap->GetAtlas()) {
                            gizmo->SetGizmoTexture(tilemap->GetAtlas());
                        }
                    }
                    if (gameplayGrid != nullptr) {
                        gameplayGrid->RequestRebake();
                        gameplayGrid->RebakeIfNeeded(*boardGo);
                        Detail::PlacePlayerOnFirstWalkableCell(*gameplayGrid, playerPos);
                    } else {
                        const float cell = tilemap != nullptr ? tilemap->GetTileWorldSize() : kTileWorld;
                        playerPos = {cell * 2.0F, cell * 2.0F};
                    }
                    {
                        const float cell = tilemap != nullptr ? tilemap->GetTileWorldSize() : kTileWorld;
                        const float mw = static_cast<float>(tilemap != nullptr ? tilemap->GetMapWidth() : kCols);
                        const float mh = static_cast<float>(tilemap != nullptr ? tilemap->GetMapHeight() : kRows);
                        camera.position = {mw * 0.5F * cell, mh * 0.5F * cell, 0.0F};
                        camera.halfExtentY = mh * 0.55F * cell;
                    }
                    if (playerGo != nullptr) {
                        if (Spark::TransformComponent* tr = playerGo->GetComponent<Spark::TransformComponent>()) {
                            tr->SetTranslation({playerPos.x, playerPos.y, 0.08F});
                        }
                    }
                    pathCells.Clear();
                    pathStep = 0;
                    ClearPathMarkers();
                    tmxStatus = Utf8String("Loaded ");
                    tmxStatus.AppendUtf8(tmxPath.CStr());
                } else {
                    tmxStatus = source->GetLastError();
                    if (tmxStatus.IsEmpty()) {
                        tmxStatus = Utf8String("TMX import failed");
                    }
                }
            }
        }
    }

    if (in.IsKeyPressedThisFrame(GLFW_KEY_R)) {
        playerPos = {3.5F, 3.5F};
        pathCells.Clear();
        pathStep = 0;
        ClearPathMarkers();
        BuildLevel();
        if (boardGo != nullptr) {
            Spark::GameWorld& world = boardGo->GetWorld();
            if (auto* autotile = boardGo->GetComponent<Spark::TilemapAutotileComponent>()) {
                autotile->SetRebuildOnUpdate(true);
                autotile->RequestRebuild();
            }
            if (auto* objectLayer = boardGo->GetComponent<Spark::TilemapObjectLayerComponent>()) {
                BuildObjectMarkers(*objectLayer, 0);
            }
            if (auto* spawn = boardGo->GetComponent<Spark::TilemapObjectSpawnComponent>()) {
                spawn->RespawnAll(*boardGo, world);
            }
            if (auto* gizmo = boardGo->GetComponent<Spark::TilemapObjectGizmoComponent>()) {
                gizmo->RebuildVisuals(*boardGo, world);
            }
        }
    }

    if (in.IsMouseButtonPressedThisFrame(GLFW_MOUSE_BUTTON_LEFT) && gameplayGrid != nullptr) {
        float mx = 0.0F;
        float my = 0.0F;
        in.GetCursorFramebufferPixels(mx, my, fbW, fbH);
        int gx = 0;
        int gy = 0;
        if (PickCell(context, mx, my, gx, gy)) {
            const TilemapGridFrame& frame = gameplayGrid->GetGridFrame();
            GridPathfinder::Cell start = frame.WorldXYToCell(playerPos);
            GridPathfinder::Cell goal{};
            goal.x = gx;
            goal.y = gy;
            Array<GridPathfinder::Cell> cells{};
            if (GridPathfinder::FindPath4(gameplayGrid->GetWalkability(), start, goal, cells)) {
                pathCells = cells;
                pathStep = pathCells.GetSize() > 1 ? 1 : 0;
                pathTarget = frame.CellCenterToWorldXY(goal);
                ShowPath(pathCells);
            }
        }
    }

    if (!pathCells.IsEmpty() && pathStep < pathCells.GetSize() && gameplayGrid != nullptr) {
        const Vector2 waypoint = gameplayGrid->GetGridFrame().CellCenterToWorldXY(pathCells[pathStep]);
        Spark::Vector2 delta{waypoint.x - playerPos.x, waypoint.y - playerPos.y};
        const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        if (dist < 0.05F) {
            playerPos = waypoint;
            ++pathStep;
        } else {
            const float step = moveSpeed * dt;
            const float t = std::min(1.0F, step / std::max(dist, 1.0e-4F));
            playerPos.x += delta.x * t;
            playerPos.y += delta.y * t;
        }
    }

    if (playerGo != nullptr) {
        if (Spark::TransformComponent* tr = playerGo->GetComponent<Spark::TransformComponent>()) {
            tr->SetTranslation({playerPos.x, playerPos.y, 0.08F});
        }
    }

    if (hudText != nullptr) {
        Utf8String hud{};
        hud.AppendUtf8("Tilemap showcase — layers, animation, autotile, path grid, object markers\n");
        hud.AppendUtf8("Left-click: pathfind   R: reset   L: load Kenney sampleMap.tmx\n");
        if (!tmxStatus.IsEmpty()) {
            hud.AppendUtf8(tmxStatus.CStr());
            hud.AppendUtf8("\n");
        }
        hud.AppendUtf8("Magenta gizmo = quest marker. Gold = spawned chests.");
        hudText->SetText(hud);
    }
}

void TilemapShowcase2DDemo::Render(
        Spark::Scene& /*scene*/,
        Spark::GameWorld& world,
        Spark::IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) {
        fbW = 1;
    }
    if (fbH <= 0) {
        fbH = 1;
    }
    const Spark::Matrix4 viewProj = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
    Spark::Vector3 pr{};
    Spark::Vector3 pu{};
    camera.BillboardBasisWorld(pr, pu);
    Spark::SubmitStandardLitSceneFromWorld(
            world,
            context,
            viewProj,
            camera.position,
            Spark::Vector3{0.42F, 0.78F, 0.38F}.Normalized(),
            Spark::Vector3{0.94F, 0.97F, 1.0F},
            0.75F,
            Spark::Vector3{0.06F, 0.09F, 0.12F},
            false,
            pr,
            pu,
            0.0F,
            SceneSpriteSortMode::SortOrderThenWorldY);
}

}  // namespace Spark
