#include "spark/demo/BroadPhase2DDemo.hpp"
#include "spark/demo/DemoAssetLoad.hpp"
#include "spark/demo/DemoFoundation.hpp"
#include "spark/demo/platformer2d/Platformer2DCombatMath.hpp"
#include "spark/ecs/components/audio/SoundCueComponent.hpp"
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"
#include "spark/scene/vfx/VfxSubsystemProcess.hpp"

#include <cstdio>

namespace Spark {
namespace {

constexpr std::uint32_t kTinyDungeonPlayerTile = 24U;
constexpr std::uint32_t kTinyDungeonGemTile = 29U;
constexpr std::uint32_t kEnemyTypeTiles[] = {96U, 97U, 108U, 109U, 120U, 121U, 122U, 123U, 124U, 125U, 126U, 127U};
constexpr std::uint32_t kEnemyTypeCount = static_cast<std::uint32_t>(sizeof(kEnemyTypeTiles) / sizeof(kEnemyTypeTiles[0]));
constexpr std::uint32_t kTinyDungeonFloorTiles[4] = {72U, 73U, 74U, 75U};

[[nodiscard]] bool IsEnemyTile(const std::uint32_t tile) noexcept {
    for (std::uint32_t i = 0; i < kEnemyTypeCount; ++i) {
        if (kEnemyTypeTiles[i] == tile) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] Spark::Vector3 EnemyFallbackRgb(const std::uint32_t tile, const float nx, const float ny) noexcept {
    const float cx = nx - 0.5F;
    const float cy = ny - 0.5F;
    const float r = std::sqrt(cx * cx + cy * cy);
    switch (tile) {
    case 96U:
        return r < 0.42F ? Spark::Vector3{0.28F, 0.82F, 0.34F} : Spark::Vector3{0.0F, 0.0F, 0.0F};
    case 97U:
        return r < 0.42F ? Spark::Vector3{0.22F, 0.48F, 0.92F} : Spark::Vector3{0.0F, 0.0F, 0.0F};
    case 108U:
        return (std::fabs(cx) < 0.28F && cy > -0.2F && cy < 0.35F) || (std::fabs(cx) < 0.18F && cy > 0.35F)
                ? Spark::Vector3{0.72F, 0.74F, 0.82F}
                : Spark::Vector3{0.0F, 0.0F, 0.0F};
    case 109U:
        return (std::fabs(cx) < 0.30F && cy > -0.15F) ? Spark::Vector3{0.58F, 0.42F, 0.28F} : Spark::Vector3{0.0F, 0.0F, 0.0F};
    case 120U:
        return (r < 0.34F && cy > -0.05F) || (std::fabs(cx) < 0.10F && cy > 0.25F)
                ? Spark::Vector3{0.90F, 0.90F, 0.84F}
                : Spark::Vector3{0.0F, 0.0F, 0.0F};
    case 121U:
        return r < 0.40F ? Spark::Vector3{0.82F, 0.88F, 0.98F} : Spark::Vector3{0.0F, 0.0F, 0.0F};
    case 122U:
        return (std::fabs(cx) < 0.22F && cy > -0.25F) ? Spark::Vector3{0.42F, 0.12F, 0.18F} : Spark::Vector3{0.0F, 0.0F, 0.0F};
    case 123U:
        return r < 0.38F ? Spark::Vector3{0.92F, 0.52F, 0.18F} : Spark::Vector3{0.0F, 0.0F, 0.0F};
    case 124U:
        return (std::fabs(cx) < 0.34F && std::fabs(cy) < 0.34F) ? Spark::Vector3{0.18F, 0.72F, 0.62F}
                                                                  : Spark::Vector3{0.0F, 0.0F, 0.0F};
    case 125U:
        return r < 0.36F ? Spark::Vector3{0.55F, 0.22F, 0.72F} : Spark::Vector3{0.0F, 0.0F, 0.0F};
    case 126U:
        return (std::fabs(cx) < 0.30F && cy > -0.10F) ? Spark::Vector3{0.88F, 0.22F, 0.22F}
                                                        : Spark::Vector3{0.0F, 0.0F, 0.0F};
    case 127U:
        return r < 0.40F ? Spark::Vector3{0.32F, 0.32F, 0.38F} : Spark::Vector3{0.0F, 0.0F, 0.0F};
    default:
        return r < 0.40F ? Spark::Vector3{0.88F, 0.90F, 0.98F} : Spark::Vector3{0.0F, 0.0F, 0.0F};
    }
}

[[nodiscard]] Spark::Vector4 EnemyTypeTint(const std::size_t typeIndex) noexcept {
    static constexpr Spark::Vector4 kTints[] = {
            {0.82F, 1.0F, 0.78F, 1.0F},
            {0.78F, 0.88F, 1.0F, 1.0F},
            {0.95F, 0.95F, 1.0F, 1.0F},
            {0.88F, 0.78F, 0.62F, 1.0F},
            {0.92F, 0.94F, 1.0F, 1.0F},
            {0.92F, 0.72F, 0.82F, 1.0F},
            {1.0F, 0.82F, 0.42F, 1.0F},
            {0.72F, 1.0F, 0.92F, 1.0F},
            {0.82F, 0.62F, 1.0F, 1.0F},
            {1.0F, 0.62F, 0.62F, 1.0F},
            {0.78F, 0.78F, 0.86F, 1.0F},
            {0.68F, 0.72F, 0.82F, 1.0F},
    };
    return kTints[typeIndex % (sizeof(kTints) / sizeof(kTints[0]))];
}

[[nodiscard]] Spark::Vector4 EnemyTypeEmission(const std::size_t typeIndex) noexcept {
    static constexpr Spark::Vector4 kEmits[] = {
            {0.35F, 0.95F, 0.42F, 1.4F},
            {0.35F, 0.55F, 0.98F, 1.5F},
            {0.85F, 0.88F, 1.0F, 1.2F},
            {0.95F, 0.62F, 0.28F, 1.3F},
            {0.72F, 0.82F, 1.0F, 1.7F},
            {0.92F, 0.35F, 0.55F, 1.6F},
            {1.0F, 0.72F, 0.22F, 1.5F},
            {0.28F, 0.92F, 0.78F, 1.4F},
            {0.68F, 0.42F, 0.98F, 1.5F},
            {1.0F, 0.35F, 0.35F, 1.6F},
            {0.55F, 0.55F, 0.65F, 1.1F},
            {0.45F, 0.48F, 0.58F, 1.0F},
    };
    return kEmits[typeIndex % (sizeof(kEmits) / sizeof(kEmits[0]))];
}

Spark::SharedPtr<Spark::Texture2D> MakeHudWhitePixelTexture()
{
    Spark::Texture2D tex(Spark::Utf8String("MazeHudWhitePixel"));
    Spark::Array<std::uint8_t> px;
    px.Resize(4U);
    px[0] = 255;
    px[1] = 255;
    px[2] = 255;
    px[3] = 255;
    tex.SetPixels(1U, 1U, Spark::MoveTemp(px));
    return Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tex));
}

[[nodiscard]] bool MazeCellIsSolidWall(
        const Array<std::uint8_t>& mazeCells, int mazeW, int mazeH, int x, int y) noexcept {
    if (x < 0 || y < 0 || x >= mazeW || y >= mazeH) {
        return true;
    }
    return mazeCells[static_cast<std::size_t>(y * mazeW + x)] != 0;
}

[[nodiscard]] std::uint32_t TinyDungeonWallAutotileSpark(
        const Array<std::uint8_t>& mazeCells, int mazeW, int mazeH, int x, int y) noexcept {
    const bool wallN = MazeCellIsSolidWall(mazeCells, mazeW, mazeH, x, y + 1);
    const bool wallE = MazeCellIsSolidWall(mazeCells, mazeW, mazeH, x + 1, y);
    const bool wallS = MazeCellIsSolidWall(mazeCells, mazeW, mazeH, x, y - 1);
    const bool wallW = MazeCellIsSolidWall(mazeCells, mazeW, mazeH, x - 1, y);
    const unsigned openN = wallN ? 0U : 1U;
    const unsigned openE = wallE ? 0U : 2U;
    const unsigned openS = wallS ? 0U : 4U;
    const unsigned openW = wallW ? 0U : 8U;
    const unsigned m = openN | openE | openS | openW;
    static constexpr std::uint32_t kByOpenMask[16] = {
            118U, 106U, 117U, 107U, 130U, 118U, 131U, 117U, 119U, 105U, 118U, 130U,
            129U, 119U, 106U, 118U,
    };
    return kByOpenMask[m];
}

[[nodiscard]] Spark::Vector4 TinyDungeonTileUv(const std::uint32_t linearTileIndex) noexcept {
    return DemoAssets::KenneyTinyDungeonTileUv(linearTileIndex);
}

[[nodiscard]] Spark::Texture2D MakeTinyDungeonAtlasFallback() {
    constexpr std::uint32_t kCols = DemoAssets::kKenneyTinyDungeonAtlasCols;
    constexpr std::uint32_t kRows = DemoAssets::kKenneyTinyDungeonAtlasRows;
    constexpr std::uint32_t kCell = 16U;
    constexpr std::uint32_t kW = kCols * kCell;
    constexpr std::uint32_t kH = kRows * kCell;
    Spark::Texture2D tex(Spark::Utf8String("Maze2DAtlasFallback"));
    Spark::Array<std::uint8_t> px;
    px.Resize(static_cast<std::size_t>(kW) * static_cast<std::size_t>(kH) * 4U);
    for (std::uint32_t ty = 0; ty < kRows; ++ty) {
        for (std::uint32_t tx = 0; tx < kCols; ++tx) {
            const std::uint32_t tile = ty * kCols + tx;
            const bool isFloor = tile == 72U || tile == 73U || tile == 74U || tile == 75U;
            const bool isWall = tile >= 105U && tile <= 131U;
            for (std::uint32_t py = 0; py < kCell; ++py) {
                for (std::uint32_t px0 = 0; px0 < kCell; ++px0) {
                    const float nx = static_cast<float>(px0) / static_cast<float>(kCell);
                    const float ny = static_cast<float>(py) / static_cast<float>(kCell);
                    Spark::Vector3 rgb{0.12F, 0.14F, 0.18F};
                    if (isFloor) {
                        const float noise = 0.85F + 0.15F * std::sin(nx * 11.0F + ny * 9.0F + static_cast<float>(tile));
                        rgb = {0.28F * noise, 0.30F * noise, 0.34F * noise};
                    } else if (isWall) {
                        const float brick = std::fmod(ny * 4.0F, 1.0F) < 0.08F ? 0.72F : 1.0F;
                        const float shade = brick * (0.78F + 0.22F * std::sin(nx * 8.0F + ny * 6.0F));
                        rgb = {0.34F * shade, 0.36F * shade, 0.48F * shade};
                    } else if (tile == kTinyDungeonPlayerTile) {
                        rgb = {0.22F, 0.58F, 0.95F};
                    } else if (tile == kTinyDungeonGemTile) {
                        rgb = {0.95F, 0.82F, 0.18F};
                    } else if (IsEnemyTile(tile)) {
                        rgb = EnemyFallbackRgb(tile, nx, ny);
                        if (rgb.x + rgb.y + rgb.z < 0.01F) {
                            rgb = {0.0F, 0.0F, 0.0F};
                        }
                    }
                    const std::uint32_t gx = tx * kCell + px0;
                    const std::uint32_t gy = ty * kCell + py;
                    const std::size_t di = (static_cast<std::size_t>(gy) * kW + gx) * 4U;
                    const float alpha = (IsEnemyTile(tile) && rgb.x + rgb.y + rgb.z < 0.01F) ? 0.0F : 1.0F;
                    px[di] = static_cast<std::uint8_t>(std::clamp(rgb.x * 255.0F, 0.0F, 255.0F));
                    px[di + 1U] = static_cast<std::uint8_t>(std::clamp(rgb.y * 255.0F, 0.0F, 255.0F));
                    px[di + 2U] = static_cast<std::uint8_t>(std::clamp(rgb.z * 255.0F, 0.0F, 255.0F));
                    px[di + 3U] = static_cast<std::uint8_t>(std::clamp(alpha * 255.0F, 0.0F, 255.0F));
                }
            }
        }
    }
    tex.SetPixels(kW, kH, Spark::MoveTemp(px));
    tex.SetSceneUploadNearest(true);
    return tex;
}

struct MazeIJ {
    int i = 0;
    int j = 0;
};

void ShuffleMazeCells(Array<MazeIJ>& cells, unsigned seed) noexcept {
    for (std::size_t n = cells.GetSize(); n > 1U; --n) {
        seed = seed * 1664525U + 1013904223U;
        const std::size_t k = static_cast<std::size_t>(seed % static_cast<unsigned>(n));
        Swap(cells[k], cells[n - 1U]);
    }
}

void GenerateMazeOddGrid(int w, int h, Array<std::uint8_t>& outCells) {
    outCells.Clear();
    const std::size_t n = static_cast<std::size_t>(w * h);
    outCells.Resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        outCells[i] = 1;
    }
    static constexpr int kDirs[4][2] = {
            {0, -2},
            {0, 2},
            {-2, 0},
            {2, 0},
    };

    Array<MazeIJ> stack;
    stack.PushBack(MazeIJ{1, 1});
    outCells[static_cast<std::size_t>(1 * w + 1)] = 0;

    while (!stack.IsEmpty()) {
        const MazeIJ cur = stack[stack.GetSize() - 1];
        const int x = cur.i;
        const int y = cur.j;
        int ord[4] = {0, 1, 2, 3};
        unsigned seed = static_cast<unsigned>(x * 1103515245 + y * 12345 + 7);
        for (int a = 3; a > 0; --a) {
            const int b = static_cast<int>((seed >> (a * 3)) % static_cast<unsigned>(a + 1));
            const int tmp = ord[a];
            ord[a] = ord[b];
            ord[b] = tmp;
        }

        bool advanced = false;
        for (int k = 0; k < 4; ++k) {
            const int* d = kDirs[ord[k]];
            const int nx = x + d[0];
            const int ny = y + d[1];
            if (nx < 1 || ny < 1 || nx >= w - 1 || ny >= h - 1) {
                continue;
            }
            if (outCells[static_cast<std::size_t>(ny * w + nx)] == 0) {
                continue;
            }
            const int mx = x + d[0] / 2;
            const int my = y + d[1] / 2;
            outCells[static_cast<std::size_t>(my * w + mx)] = 0;
            outCells[static_cast<std::size_t>(ny * w + nx)] = 0;
            stack.PushBack(MazeIJ{nx, ny});
            advanced = true;
            break;
        }
        if (!advanced) {
            stack.PopBack();
        }
    }
}

void BuildPhysicalMazeWideFloorsThinWalls(
        const Array<std::uint8_t>& logic,
        int lw,
        int lh,
        int k,
        Array<std::uint8_t>& phys) noexcept {
    const int stride = k + 1;
    const int pw = lw * stride - 1;
    const int ph = lh * stride - 1;
    phys.Resize(static_cast<std::size_t>(pw * ph));
    for (std::size_t i = 0; i < phys.GetSize(); ++i) {
        phys[i] = 1;
    }
    for (int ly = 0; ly < lh; ++ly) {
        for (int lx = 0; lx < lw; ++lx) {
            if (logic[static_cast<std::size_t>(ly * lw + lx)] != 0) {
                continue;
            }
            const int bx = lx * stride;
            const int by = ly * stride;
            for (int dy = 0; dy < k; ++dy) {
                for (int dx = 0; dx < k; ++dx) {
                    phys[static_cast<std::size_t>((by + dy) * pw + (bx + dx))] = 0;
                }
            }
        }
    }
    for (int ly = 0; ly < lh; ++ly) {
        for (int lx = 0; lx < lw; ++lx) {
            if (logic[static_cast<std::size_t>(ly * lw + lx)] != 0) {
                continue;
            }
            if (lx + 1 < lw && logic[static_cast<std::size_t>(ly * lw + lx + 1)] == 0) {
                const int px = lx * stride + k;
                for (int py = ly * stride; py < ly * stride + k; ++py) {
                    phys[static_cast<std::size_t>(py * pw + px)] = 0;
                }
            }
            if (ly + 1 < lh && logic[static_cast<std::size_t>((ly + 1) * lw + lx)] == 0) {
                const int py = ly * stride + k;
                for (int px = lx * stride; px < lx * stride + k; ++px) {
                    phys[static_cast<std::size_t>(py * pw + px)] = 0;
                }
            }
        }
    }
}

[[nodiscard]] bool PhysicalCellReservedForSpawn(
        int px,
        int py,
        int lw,
        int lh,
        int k,
        int stride,
        const Array<std::uint8_t>& logic) noexcept {
    static constexpr int rx[3] = {1, 1, 2};
    static constexpr int ry[3] = {1, 2, 1};
    for (int i = 0; i < 3; ++i) {
        const int lx = rx[i];
        const int ly = ry[i];
        if (px >= lx * stride && px < lx * stride + k && py >= ly * stride && py < ly * stride + k) {
            return true;
        }
    }
    if (lw > 2 && logic[static_cast<std::size_t>(1 * lw + 1)] == 0 && logic[static_cast<std::size_t>(1 * lw + 2)] == 0) {
        const int pxConn = 1 * stride + k;
        if (px == pxConn && py >= 1 * stride && py < 1 * stride + k) {
            return true;
        }
    }
    if (lh > 2 && logic[static_cast<std::size_t>(1 * lw + 1)] == 0 && logic[static_cast<std::size_t>(2 * lw + 1)] == 0) {
        const int pyConn = 1 * stride + k;
        if (py == pyConn && px >= 1 * stride && px < 1 * stride + k) {
            return true;
        }
    }
    return false;
}

}  // namespace

Platformer2D::BulletProfile BroadPhase2DDemo::MakePlayerBulletProfile() const noexcept
{
    Platformer2D::BulletProfile profile{};
    profile.speed = 18.0F * kCellWorld;
    profile.halfW = 0.045F * kCellWorld;
    profile.halfH = 0.030F * kCellWorld;
    profile.drawScale = 0.36F * kCellWorld;
    profile.lifetime = 1.8F;
    profile.baseTint = {0.35F, 0.92F, 1.0F, 0.94F};
    profile.additiveBlend = true;
    return profile;
}

Platformer2D::BulletProfile BroadPhase2DDemo::MakeEnemyBulletProfile() const noexcept
{
    Platformer2D::BulletProfile profile{};
    profile.speed = 10.5F * kCellWorld;
    profile.halfW = 0.050F * kCellWorld;
    profile.halfH = 0.032F * kCellWorld;
    profile.drawScale = 0.38F * kCellWorld;
    profile.lifetime = 2.4F;
    profile.baseTint = {1.0F, 0.42F, 0.55F, 0.90F};
    profile.additiveBlend = true;
    return profile;
}

void BroadPhase2DDemo::SpawnEnemies(Spark::GameWorld& world, const Spark::Array<Spark::Vector2>& spawnPoints)
{
    enemies.Clear();
    enemySpriteTextures.Clear();
    const int count = static_cast<int>((std::min)(spawnPoints.GetSize(), static_cast<std::size_t>(kEnemyCount)));
    enemies.Resize(static_cast<std::size_t>(count));
    for (int ei = 0; ei < count; ++ei) {
        const Spark::Vector2 pos = spawnPoints[static_cast<std::size_t>(ei)];
        const std::size_t typeIdx = static_cast<std::size_t>(ei) % static_cast<std::size_t>(kEnemyTypeCount);
        const std::uint32_t tile = kEnemyTypeTiles[typeIdx];
        MazeEnemy& enemy = enemies[static_cast<std::size_t>(ei)];
        enemy = {};
        enemy.alive = true;
        enemy.homePos = pos;
        enemy.spriteTile = tile;
        enemy.wanderPhase = static_cast<float>(ei) * 1.37F;
        enemy.shootCooldown = 0.8F + 0.35F * static_cast<float>(ei);

        Spark::SharedPtr<Spark::Texture2D> spriteTex = dungeonAtlasTex;
        Spark::Vector4 spriteUv = TinyDungeonTileUv(tile);
        Spark::Texture2D tileCpu;
        if (DemoAssets::TryLoadKenneyTinyDungeonTileTexture(tileCpu, tile)) {
            Spark::Utf8String texName("MazeEnemyTile");
            texName.AppendUtf8("_");
            char suffix[12];
            std::snprintf(suffix, sizeof(suffix), "%u", tile);
            texName.AppendUtf8(suffix);
            tileCpu.GetName() = Spark::MoveTemp(texName);
            spriteTex = Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tileCpu));
            enemySpriteTextures.PushBack(spriteTex);
            char regKey[64];
            std::snprintf(regKey, sizeof(regKey), "spark/maze2d/enemy_%d_%u", ei, tile);
            world.RegisterTexture(spriteTex, regKey);
            spriteUv = Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F};
        }

        const Spark::Vector4 tint = EnemyTypeTint(typeIdx);
        const Spark::Vector4 emit = EnemyTypeEmission(typeIdx);

        Spark::GameObject* ego = world.CreateGameObject();
        ego->GetName() = Spark::Utf8String("MazeEnemy");
        enemy.go = ego;
        enemy.tr = ego->AddComponent<Spark::TransformComponent>();
        enemy.tr->SetTranslation({pos.x, pos.y, 0.058F + 0.0001F * static_cast<float>(ei)});
        enemy.tr->SetScale({0.62F * kCellWorld, 0.62F * kCellWorld, 1.0F});
        ego->AddComponent<Spark::SpriteComponent>(spriteTex, tint, spriteUv, 6000);
        ego->AddComponent<Spark::SpriteLighting2DComponent>(
                SpriteLighting2DMode::PulseEmission,
                emit,
                Spark::Vector4{1.15F, 0.42F, 0.0F, 0.0F});
        ego->AddComponent<Spark::BoxCollider2DComponent>();
        enemy.rb = ego->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Dynamic, 1.0F);
        enemy.rb->SetGravityScale(0.0F);
        roots.PushBack(ego);
    }
}

void BroadPhase2DDemo::TickEnemies(const float deltaSeconds, const float playerX, const float playerY)
{
    constexpr float kChaseRange = 12.0F * kCellWorld;
    constexpr float kShootRange = 16.0F * kCellWorld;
    constexpr float kEnemySpeed = 4.0F * kCellWorld;

    for (std::size_t ei = 0; ei < enemies.GetSize(); ++ei) {
        MazeEnemy& enemy = enemies[ei];
        if (!enemy.alive || enemy.tr == nullptr || enemy.rb == nullptr) {
            continue;
        }
        enemy.shootCooldown = std::max(0.0F, enemy.shootCooldown - deltaSeconds);
        const Spark::Vector3 pos = enemy.tr->GetLocalTransform().translation;
        float dx = playerX - pos.x;
        float dy = playerY - pos.y;
        const float dist = std::sqrt(dx * dx + dy * dy);
        float moveX = 0.0F;
        float moveY = 0.0F;
        if (dist < kChaseRange && dist > 0.25F * kCellWorld) {
            Platformer2D::CombatMath::NormalizeOrDefault(dx, dy, 1.0F, 0.0F, moveX, moveY);
        } else {
            enemy.wanderPhase += deltaSeconds * 0.9F;
            moveX = std::cos(enemy.wanderPhase) * 0.40F;
            moveY = std::sin(enemy.wanderPhase * 0.8F) * 0.40F;
        }
        enemy.rb->SetVelocity({moveX * kEnemySpeed, moveY * kEnemySpeed});

        if (dist < kShootRange && enemy.shootCooldown <= 0.0F) {
            float dirX = 0.0F;
            float dirY = 0.0F;
            Platformer2D::CombatMath::NormalizeOrDefault(dx, dy, 1.0F, 0.0F, dirX, dirY);
            if (enemyBullets.TrySpawn(
                        pos.x + dirX * kEnemyHalfW * 0.9F,
                        pos.y + dirY * kEnemyHalfW * 0.9F,
                        dirX,
                        dirY,
                        MakeEnemyBulletProfile())) {
                enemy.shootCooldown = 1.45F + 0.25F * static_cast<float>(ei % 3U);
            } else {
                enemy.shootCooldown = 0.35F;
            }
        }
    }
}

void BroadPhase2DDemo::ResolveCombat(
        Spark::GameWorld& world,
        Spark::IEngineContext& context,
        const float playerX,
        const float playerY)
{
    for (std::size_t bi = 0; bi < playerBullets.Slots().GetSize(); ++bi) {
        Platformer2D::BulletPool::Slot& bullet = playerBullets.Slots()[bi];
        if (!bullet.active) {
            continue;
        }
        for (std::size_t ei = 0; ei < enemies.GetSize(); ++ei) {
            MazeEnemy& enemy = enemies[ei];
            if (!enemy.alive || enemy.tr == nullptr) {
                continue;
            }
            const Spark::Vector3 epos = enemy.tr->GetLocalTransform().translation;
            if (!Platformer2D::CombatMath::BoxOverlap(
                        bullet.cx,
                        bullet.cy,
                        bullet.profile.halfW,
                        bullet.profile.halfH,
                        epos.x,
                        epos.y,
                        kEnemyHalfW,
                        kEnemyHalfH)) {
                continue;
            }
            Platformer2D::BulletPool::DeactivateSlot(bullet);
            explosions.SpawnEnemyDefeat(epos.x, epos.y);
            enemy.alive = false;
            world.DestroyGameObject(enemy.go);
            enemy.go = nullptr;
            enemy.tr = nullptr;
            enemy.rb = nullptr;
            ++enemiesDefeated;
            if (playerGo != nullptr && sfxExplosion.Get() != nullptr) {
                DemoAudio::QueueCue(*playerGo, sfxExplosion, 0.88F);
            } else {
                DemoPlayProceduralClip(context, DemoSfx::ClipInvadersHit(), 0.85F);
            }
            break;
        }
    }

    if (playerHurtCooldown > 0.0F) {
        return;
    }
    for (std::size_t bi = 0; bi < enemyBullets.Slots().GetSize(); ++bi) {
        Platformer2D::BulletPool::Slot& bullet = enemyBullets.Slots()[bi];
        if (!bullet.active) {
            continue;
        }
        if (!Platformer2D::CombatMath::BoxOverlap(
                    bullet.cx,
                    bullet.cy,
                    bullet.profile.halfW,
                    bullet.profile.halfH,
                    playerX,
                    playerY,
                    kPlayerHalfW,
                    kPlayerHalfH)) {
            continue;
        }
        Platformer2D::BulletPool::DeactivateSlot(bullet);
        playerHurtCooldown = kPlayerHurtCooldownSeconds;
        if (playerDamageable != nullptr) {
            playerDamageable->ApplyDamage(kEnemyBulletDamage, nullptr);
        } else if (playerHealth != nullptr) {
            playerHealth->ApplyDamage(kEnemyBulletDamage, nullptr);
        }
        explosions.SpawnPlayerHurt(playerX, playerY);
        if (playerGo != nullptr && sfxHurt.Get() != nullptr) {
            DemoAudio::QueueCue(*playerGo, sfxHurt, 0.92F);
        }
        break;
    }
}

void BroadPhase2DDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
    Unload(w);

    roots.Clear();
    gemObjects.Clear();
    gemBasePositions.Clear();
    enemies.Clear();
    staticColliders.Clear();
    broadGrid.Clear();
    queryScratch.Clear();
    if (mazeAudioEngine != nullptr) {
        mazeAudioEngine->ClearBackgroundMusic();
        mazeAudioEngine = nullptr;
    }
    playerBullets.Shutdown(w);
    enemyBullets.Shutdown(w);
    explosions.Shutdown();
    healthHud.Shutdown(w);

    wallCount = 0;
    lastBroadCandidates = 0;
    lastNarrowHits = 0;
    gemsCollected = 0;
    gemsTotal = 0;
    enemiesDefeated = 0;
    sceneTime = 0.0F;
    playerHurtCooldown = 0.0F;
    aimX = 1.0F;
    aimY = 0.0F;

    Array<std::uint8_t> logicCells;
    GenerateMazeOddGrid(kMazeLogicalW, kMazeLogicalH, logicCells);
    Array<std::uint8_t> cells;
    BuildPhysicalMazeWideFloorsThinWalls(
            logicCells, kMazeLogicalW, kMazeLogicalH, kCorridorFloorCells, cells);

    Spark::Texture2D dungeonAtlasCpu{};
    if (!DemoAssets::TryLoadKenneyTinyDungeonAtlas(dungeonAtlasCpu)) {
        dungeonAtlasCpu = MakeTinyDungeonAtlasFallback();
    }
    dungeonAtlasTex = Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(dungeonAtlasCpu));
    w.RegisterTexture(dungeonAtlasTex, "spark/maze2d/kenney_tiny_dungeon");

    bulletTex = Spark::MakeShared<Spark::Texture2D>(DemoAssets::MakePlayerBulletTextureFallback());
    enemyBulletTex = Spark::MakeShared<Spark::Texture2D>(DemoAssets::MakeEnemyBulletTextureFallback());
    w.RegisterTexture(bulletTex, "spark/maze2d/bullet");
    w.RegisterTexture(enemyBulletTex, "spark/maze2d/enemy_bullet");

    hudWhiteTex = MakeHudWhitePixelTexture();
    w.RegisterTexture(hudWhiteTex, "spark/maze2d/hud_white");

    sfxCoin = TryLoadSoundClipFromBundledAsset("assets/audio/coin.wav");
    sfxHurt = TryLoadSoundClipFromBundledAsset("assets/audio/hurt.wav");
    sfxExplosion = TryLoadSoundClipFromBundledAsset("assets/audio/explosion.wav");

    mazeOriginX = -0.5F * static_cast<float>(kMazeW) * kCellWorld;
    mazeOriginY = -0.5F * static_cast<float>(kMazeH) * kCellWorld;

    for (int y = 0; y < kMazeH; ++y) {
        for (int x = 0; x < kMazeW; ++x) {
            if (cells[static_cast<std::size_t>(y * kMazeW + x)] != 0) {
                continue;
            }
            Spark::GameObject* floorGo = w.CreateGameObject();
            floorGo->GetName() = Spark::Utf8String("MazeFloor");
            Spark::TransformComponent* floorTr = floorGo->AddComponent<Spark::TransformComponent>();
            const float cx = mazeOriginX + (static_cast<float>(x) + 0.5F) * kCellWorld;
            const float cy = mazeOriginY + (static_cast<float>(y) + 0.5F) * kCellWorld;
            floorTr->SetTranslation({cx, cy, -0.02F});
            floorTr->SetScale({kCellWorld, kCellWorld, 1.0F});
            const std::uint32_t floorTile = kTinyDungeonFloorTiles[static_cast<std::size_t>((x + y) % 4)];
            const float shade = 0.88F + 0.10F * std::sin(static_cast<float>(x) * 0.41F + static_cast<float>(y) * 0.33F);
            floorGo->AddComponent<Spark::SpriteComponent>(
                    dungeonAtlasTex,
                    Spark::Vector4{0.62F * shade, 0.66F * shade, 0.72F * shade, 1.0F},
                    TinyDungeonTileUv(floorTile),
                    2);
            roots.PushBack(floorGo);
        }
    }

    for (int y = 0; y < kMazeH; ++y) {
        for (int x = 0; x < kMazeW; ++x) {
            if (cells[static_cast<std::size_t>(y * kMazeW + x)] == 0) {
                continue;
            }
            Spark::GameObject* g = w.CreateGameObject();
            g->GetName() = Spark::Utf8String("MazeWall");
            Spark::TransformComponent* tr = g->AddComponent<Spark::TransformComponent>();
            const float cx = mazeOriginX + (static_cast<float>(x) + 0.5F) * kCellWorld;
            const float cy = mazeOriginY + (static_cast<float>(y) + 0.5F) * kCellWorld;
            tr->SetTranslation({cx, cy, 0.01F + 0.0002F * static_cast<float>(x + y)});
            tr->SetScale({kCellWorld, kCellWorld, 1.0F});
            const float wallShade = 0.80F + 0.14F * std::sin(static_cast<float>(x) * 0.27F + static_cast<float>(y) * 0.31F);
            g->AddComponent<Spark::SpriteComponent>(
                    dungeonAtlasTex,
                    Spark::Vector4{0.52F * wallShade, 0.55F * wallShade, 0.68F * wallShade, 1.0F},
                    TinyDungeonTileUv(TinyDungeonWallAutotileSpark(cells, kMazeW, kMazeH, x, y)),
                    4 + ((x + y * 3) % 40));
            g->AddComponent<Spark::SpriteLighting2DComponent>(
                    SpriteLighting2DMode::DirectionalLambert,
                    Spark::Vector4{0.42F, 0.68F, 0.0F, 0.0F},
                    Spark::Vector4{0.0F, 0.0F, 0.0F, 0.0F});
            g->AddComponent<Spark::BoxCollider2DComponent>();
            roots.PushBack(g);
            ++wallCount;
        }
    }

    playerGo = w.CreateGameObject();
    playerGo->GetName() = Spark::Utf8String("MazePlayer");
    playerTr = playerGo->AddComponent<Spark::TransformComponent>();
    static constexpr int kSpawnLogicalX = 1;
    static constexpr int kSpawnLogicalY = 1;
    const float spawnCellCx =
            static_cast<float>(kSpawnLogicalX * kMazeStride) + 0.5F * static_cast<float>(kCorridorFloorCells);
    const float spawnCellCy =
            static_cast<float>(kSpawnLogicalY * kMazeStride) + 0.5F * static_cast<float>(kCorridorFloorCells);
    const float px = mazeOriginX + spawnCellCx * kCellWorld;
    const float py = mazeOriginY + spawnCellCy * kCellWorld;
    playerTr->SetTranslation({px, py, 0.06F});
    playerTr->SetScale({0.74F * kCellWorld, 0.74F * kCellWorld, 1.0F});
    playerGo->AddComponent<Spark::SpriteComponent>(
            dungeonAtlasTex,
            Spark::Vector4{1.0F, 1.0F, 1.0F, 1.0F},
            TinyDungeonTileUv(kTinyDungeonPlayerTile),
            6000);
    playerGo->AddComponent<Spark::SpriteLighting2DComponent>(
            SpriteLighting2DMode::Rim,
            Spark::Vector4{0.55F, 0.82F, 1.0F, 2.2F},
            Spark::Vector4{0.55F, 0.0F, 0.0F, 0.0F});
    playerGo->AddComponent<Spark::BoxCollider2DComponent>();
    playerRb = playerGo->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Dynamic, 1.0F);
    playerRb->SetVelocity(Spark::Vector2::Zero);
    playerHealth = playerGo->AddComponent<Spark::HealthComponent>(kPlayerMaxHealth);
    playerDamageable = playerGo->AddComponent<Spark::DamageableComponent>();
    playerGo->AddComponent<Spark::SoundCueComponent>();

    Array<MazeIJ> floorCells;
    Spark::Array<Spark::Vector2> enemySpawnPoints;
    for (int y = 0; y < kMazeH; ++y) {
        for (int x = 0; x < kMazeW; ++x) {
            if (cells[static_cast<std::size_t>(y * kMazeW + x)] != 0) {
                continue;
            }
            if (PhysicalCellReservedForSpawn(
                        x,
                        y,
                        kMazeLogicalW,
                        kMazeLogicalH,
                        kCorridorFloorCells,
                        kMazeStride,
                        logicCells)) {
                continue;
            }
            floorCells.PushBack(MazeIJ{x, y});
        }
    }
    ShuffleMazeCells(floorCells, static_cast<unsigned>(kMazeW * 49999 + kMazeH * 131U + 17U));

    constexpr int kMaxGems = 42;
    gemsTotal = static_cast<int>(floorCells.GetSize());
    if (gemsTotal > kMaxGems) {
        gemsTotal = kMaxGems;
    }
    for (int gi = 0; gi < gemsTotal; ++gi) {
        const MazeIJ c = floorCells[static_cast<std::size_t>(gi)];
        Spark::GameObject* gem = w.CreateGameObject();
        gem->GetName() = Spark::Utf8String("MazeGem");
        Spark::TransformComponent* gtr = gem->AddComponent<Spark::TransformComponent>();
        const float gx = mazeOriginX + (static_cast<float>(c.i) + 0.5F) * kCellWorld;
        const float gy = mazeOriginY + (static_cast<float>(c.j) + 0.5F) * kCellWorld;
        gtr->SetTranslation({gx, gy, 0.055F + 0.0001F * static_cast<float>(gi)});
        gtr->SetScale({0.34F * kCellWorld, 0.34F * kCellWorld, 1.0F});
        const float hue = static_cast<float>(gi) * 0.37F;
        const Spark::Vector3 rgb{
                0.35F + 0.45F * std::fabs(std::sin(hue)),
                0.55F + 0.35F * std::fabs(std::sin(hue + 2.1F)),
                0.85F + 0.15F * std::fabs(std::sin(hue + 4.2F))};
        gem->AddComponent<Spark::SpriteComponent>(
                dungeonAtlasTex,
                Spark::Vector4{1.0F, 1.0F, 1.0F, 1.0F},
                TinyDungeonTileUv(kTinyDungeonGemTile),
                6000);
        const float pulseHz = 1.1F + 0.11F * static_cast<float>(gi % 7);
        const float emitStr = 1.35F + 0.08F * static_cast<float>(gi % 5);
        gem->AddComponent<Spark::SpriteLighting2DComponent>(
                SpriteLighting2DMode::PulseEmission,
                Spark::Vector4{rgb.x * 1.2F, rgb.y * 1.2F, rgb.z * 1.15F, pulseHz},
                Spark::Vector4{emitStr, 0.42F, 0.0F, 0.0F});
        gemObjects.PushBack(gem);
        gemBasePositions.PushBack({gx, gy});
    }

    for (int ei = 0; ei < kEnemyCount; ++ei) {
        const std::size_t idx = static_cast<std::size_t>(gemsTotal + ei);
        if (idx >= floorCells.GetSize()) {
            break;
        }
        const MazeIJ c = floorCells[idx];
        enemySpawnPoints.PushBack(
                {mazeOriginX + (static_cast<float>(c.i) + 0.5F) * kCellWorld,
                 mazeOriginY + (static_cast<float>(c.j) + 0.5F) * kCellWorld});
    }

    fpsHudObject = w.CreateGameObject();
    fpsHudObject->GetName() = Spark::Utf8String("MazeFpsHud");
    fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
    fpsText->SetScreenPosition(Spark::DemoHud::kScreenMargin, 86.0F);
    DemoHud::Apply(*fpsText, false);
    fpsText->SetText(Spark::Utf8String("maze — WASD move | J attack | ESC menu"));
    roots.PushBack(playerGo);
    roots.PushBack(fpsHudObject);

    mainCameraGo = w.CreateGameObject();
    mainCameraGo->GetName() = Spark::Utf8String("MazeCamera");
    Spark::TransformComponent* camTr = mainCameraGo->AddComponent<Spark::TransformComponent>();
    camTr->SetTranslation({px, py, 0.0F});
    cameraComp = mainCameraGo->AddComponent<Spark::Camera2DComponent>();
    cameraComp->SetHalfExtentY(kCameraHalfExtentInCells * kCellWorld);
    cameraComp->SetPriority(100);
    roots.PushBack(mainCameraGo);

    {
        DemoRootCollection hudRoots{};
        healthHud.Initialize(w, hudWhiteTex, hudRoots);
        for (std::size_t hi = 0; hi < hudRoots.GetRoots().GetSize(); ++hi) {
            roots.PushBack(hudRoots.GetRoots()[hi]);
        }
        healthHud.SetHealth(kPlayerMaxHealth, kPlayerMaxHealth);
    }

    {
        const float hingeX = px + 2.8F * kCellWorld;
        const float hingeY = py;
        Spark::GameObject* hingeAnchor = w.CreateGameObject();
        hingeAnchor->GetName() = Spark::Utf8String("Maze2DHingeAnchor");
        Spark::TransformComponent* anchorTr = hingeAnchor->AddComponent<Spark::TransformComponent>();
        anchorTr->SetTranslation({hingeX, hingeY, 0.04F});
        anchorTr->SetScale({0.35F * kCellWorld, 0.35F * kCellWorld, 1.0F});
        hingeAnchor->AddComponent<Spark::SpriteComponent>(
                dungeonAtlasTex,
                Spark::Vector4{0.72F, 0.72F, 0.78F, 1.0F},
                TinyDungeonTileUv(118U),
                5000);
        hingeAnchor->AddComponent<Spark::BoxCollider2DComponent>();
        hingeAnchor->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Static, 1.0F);
        roots.PushBack(hingeAnchor);

        Spark::GameObject* swingPlat = w.CreateGameObject();
        swingPlat->GetName() = Spark::Utf8String("Maze2DSwingPlat");
        Spark::TransformComponent* swingTr = swingPlat->AddComponent<Spark::TransformComponent>();
        swingTr->SetTranslation({hingeX, hingeY - 1.6F * kCellWorld, 0.05F});
        swingTr->SetScale({1.4F * kCellWorld, 0.28F * kCellWorld, 1.0F});
        swingPlat->AddComponent<Spark::SpriteComponent>(
                dungeonAtlasTex,
                Spark::Vector4{0.55F, 0.82F, 0.95F, 1.0F},
                TinyDungeonTileUv(kTinyDungeonGemTile),
                5100);
        swingPlat->AddComponent<Spark::BoxCollider2DComponent>();
        Spark::Rigidbody2DComponent* swingRb =
                swingPlat->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Dynamic, 1.0F);
        swingRb->SetGravityScale(0.0F);
        Spark::HingeJoint2DComponent* hinge = swingPlat->AddComponent<Spark::HingeJoint2DComponent>(hingeAnchor);
        hinge->SetLocalAnchorA({0.0F, 0.5F * kCellWorld});
        hinge->SetLocalAnchorB({0.0F, -0.5F * kCellWorld});
        hinge->SetStiffness(0.72F);
        roots.PushBack(swingPlat);
    }

    DemoRootCollection bulletRoots{};
    playerBullets.Initialize(w, bulletTex, 8, 7000, Spark::Utf8String("MazePlayerBullet"), bulletRoots);
    enemyBullets.Initialize(w, enemyBulletTex, 10, 7100, Spark::Utf8String("MazeEnemyBullet"), bulletRoots);
    for (std::size_t ri = 0; ri < bulletRoots.GetRoots().GetSize(); ++ri) {
        roots.PushBack(bulletRoots.GetRoots()[ri]);
    }
    explosions.Initialize(w);
    SpawnEnemies(w, enemySpawnPoints);

    const float broadCell = std::max(16.0F, kCellWorld * 4.0F);
    Spark::ColliderBakePipeline2D::GetDefault().Rebuild(w, broadCell, staticColliders, broadGrid);

    PhysicsWorld2DSettings& phys = physics.GetWorld2D().GetSettings();
    phys.gravityY = 0.0F;
    phys.maxFallSpeed = 500.0F;
    phys.resolveDynamicVsDynamic = true;
    phys.jointIterations = 6;

    camera.position = {px, py, 0.0F};
    camera.halfExtentY = kCameraHalfExtentInCells * kCellWorld;
    camera.rotationRad = 0.0F;

    context.GetInput().SetCursorCaptured(false);

    mazeAudioEngine = context.TryGetSoundEngine();
    if (mazeAudioEngine != nullptr && mazeAudioEngine->IsRunning()) {
        auto clip = TryLoadSoundClipFromBundledAsset("assets/audio/castle.wav");
        if (!clip) {
            clip = SoundClip::CreateSimpleAmbienceLoop();
        }
        mazeAudioEngine->SetBackgroundMusic(clip, 0.26F, true);
    }
}

void BroadPhase2DDemo::Unload(Spark::GameWorld& w)
{
    if (mazeAudioEngine != nullptr) {
        mazeAudioEngine->ClearBackgroundMusic();
        mazeAudioEngine = nullptr;
    }
    for (std::size_t i = 0; i < gemObjects.GetSize(); ++i) {
        if (gemObjects[i] != nullptr) {
            w.DestroyGameObject(gemObjects[i]);
        }
    }
    gemObjects.Clear();
    gemBasePositions.Clear();
    enemies.Clear();
    playerBullets.Shutdown(w);
    enemyBullets.Shutdown(w);
    explosions.Shutdown();
    healthHud.Shutdown(w);
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            w.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    dungeonAtlasTex.Reset();
    bulletTex.Reset();
    enemyBulletTex.Reset();
    hudWhiteTex.Reset();
    enemySpriteTextures.Clear();
    sfxCoin.Reset();
    sfxHurt.Reset();
    sfxExplosion.Reset();
    staticColliders.Clear();
    broadGrid.Clear();
    queryScratch.Clear();
    playerGo = nullptr;
    playerTr = nullptr;
    playerRb = nullptr;
    playerHealth = nullptr;
    playerDamageable = nullptr;
    mainCameraGo = nullptr;
    cameraComp = nullptr;
    fpsHudObject = nullptr;
    fpsText = nullptr;
    enemiesDefeated = 0;
    playerHurtCooldown = 0.0F;
}

void BroadPhase2DDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world)
{
    sceneTime = timing.totalTimeSeconds;
    Spark::IInput& in = context.GetInput();
    const float dt = timing.deltaTimeSeconds;
    Spark::ProcessVfx(world);

    if (playerHurtCooldown > 0.0F) {
        playerHurtCooldown = std::max(0.0F, playerHurtCooldown - dt);
    }

    if (playerRb != nullptr && playerTr != nullptr) {
        float mx = 0.0F;
        float my = 0.0F;
        if (in.IsKeyDown(GLFW_KEY_A) || in.IsKeyDown(GLFW_KEY_LEFT)) {
            mx -= 1.0F;
        }
        if (in.IsKeyDown(GLFW_KEY_D) || in.IsKeyDown(GLFW_KEY_RIGHT)) {
            mx += 1.0F;
        }
        if (in.IsKeyDown(GLFW_KEY_W) || in.IsKeyDown(GLFW_KEY_UP)) {
            my += 1.0F;
        }
        if (in.IsKeyDown(GLFW_KEY_S) || in.IsKeyDown(GLFW_KEY_DOWN)) {
            my -= 1.0F;
        }
        const float lenSq = mx * mx + my * my;
        if (lenSq > 1.0e-6F) {
            const float inv = 1.0F / std::sqrt(lenSq);
            mx *= inv;
            my *= inv;
            aimX = mx;
            aimY = my;
        }

        constexpr float kMoveSpeedPerCell = 11.0F;
        const float moveSpeed = kMoveSpeedPerCell * kCellWorld;
        playerRb->SetVelocity({mx * moveSpeed, my * moveSpeed});

        const bool attackPressed = in.IsKeyPressedThisFrame(GLFW_KEY_J);
        if (attackPressed) {
            const Spark::Vector3 p0 = playerTr->GetLocalTransform().translation;
            if (playerBullets.TrySpawn(
                        p0.x + aimX * kPlayerHalfW * 0.95F,
                        p0.y + aimY * kPlayerHalfW * 0.95F,
                        aimX,
                        aimY,
                        MakePlayerBulletProfile())) {
                explosions.SpawnMuzzleFlash(
                        p0.x + aimX * kPlayerHalfW * 1.1F, p0.y + aimY * kPlayerHalfW * 1.1F);
                DemoPlayProceduralClip(context, DemoSfx::ClipPlatformerShoot(), 0.78F);
            }
        }

        physics.Simulate2D(world, timing);

        const Spark::Vector3 p = playerTr->GetLocalTransform().translation;
        TickEnemies(dt, p.x, p.y);

        const float cullPad = 6.0F * kCellWorld;
        const float cullMinX = mazeOriginX - cullPad;
        const float cullMaxX = -mazeOriginX + cullPad;
        const float cullMinY = mazeOriginY - cullPad;
        const float cullMaxY = -mazeOriginY + cullPad;
        playerBullets.Tick(dt, cullMinX, cullMaxX, cullMinY, cullMaxY);
        enemyBullets.Tick(dt, cullMinX, cullMaxX, cullMinY, cullMaxY);
        ResolveCombat(world, context, p.x, p.y);

        if (playerHealth != nullptr && !playerHealth->IsAlive()) {
            playerHealth->ResetToFull();
            static constexpr int kSpawnLogicalX = 1;
            static constexpr int kSpawnLogicalY = 1;
            const float spawnCellCx =
                    static_cast<float>(kSpawnLogicalX * kMazeStride) + 0.5F * static_cast<float>(kCorridorFloorCells);
            const float spawnCellCy =
                    static_cast<float>(kSpawnLogicalY * kMazeStride) + 0.5F * static_cast<float>(kCorridorFloorCells);
            playerTr->SetTranslation(
                    {mazeOriginX + spawnCellCx * kCellWorld, mazeOriginY + spawnCellCy * kCellWorld, p.z});
            playerRb->SetVelocity(Spark::Vector2::Zero);
            enemyBullets.DeactivateAll();
            playerHurtCooldown = 0.0F;
        }

        const Spark::BoxCollider2DComponent* pCol = playerGo->GetComponent<Spark::BoxCollider2DComponent>();
        if (pCol != nullptr) {
            CollisionAabb2 playerBox{};
            ComputeBoxCollider2WorldAabb(*playerGo, *pCol, playerBox);
            broadGrid.QueryUniquePayloadIndices(playerBox, queryScratch);
            lastBroadCandidates = static_cast<std::uint32_t>(queryScratch.GetSize());
            std::uint32_t narrowHits = 0;
            for (std::size_t i = 0; i < queryScratch.GetSize(); ++i) {
                const std::uint32_t idx = queryScratch[i];
                if (idx < staticColliders.GetSize() && staticColliders[idx].OverlapsAabb(playerBox)) {
                    ++narrowHits;
                }
            }
            lastNarrowHits = narrowHits;
        }

        const float follow = std::min(1.0F, 8.0F * dt);
        camera.position.x += (p.x - camera.position.x) * follow;
        camera.position.y += (p.y - camera.position.y) * follow;

        if (mainCameraGo != nullptr && cameraComp != nullptr) {
            if (Spark::TransformComponent* camTr = mainCameraGo->GetComponent<Spark::TransformComponent>()) {
                camTr->SetTranslation({camera.position.x, camera.position.y, 0.0F});
            }
            int fbW = 0;
            int fbH = 0;
            context.GetFramebufferSize(fbW, fbH);
            healthHud.SyncToCamera(*cameraComp, *mainCameraGo, static_cast<float>(fbW), static_cast<float>(fbH));
        }
        if (playerHealth != nullptr) {
            healthHud.SetHealth(playerHealth->GetCurrent(), playerHealth->GetMaximum());
        }

        const float collectRadius = 0.52F * kCellWorld;
        const float magnetRadius = 1.15F * collectRadius;
        const float cr2 = collectRadius * collectRadius;
        const float mr2 = magnetRadius * magnetRadius;
        for (std::size_t gi = 0; gi < gemObjects.GetSize();) {
            Spark::GameObject* gem = gemObjects[gi];
            if (gem == nullptr) {
                gemObjects.RemoveAt(gi);
                gemBasePositions.RemoveAt(gi);
                continue;
            }
            Spark::TransformComponent* gtr = gem->GetComponent<Spark::TransformComponent>();
            if (gtr == nullptr) {
                world.DestroyGameObject(gem);
                gemObjects.RemoveAt(gi);
                gemBasePositions.RemoveAt(gi);
                continue;
            }
            const Spark::Vector2 base = gemBasePositions[gi];
            const float bob = std::sin(sceneTime * 4.5F + static_cast<float>(gi) * 0.61F) * 0.06F * kCellWorld;
            const float pulse = (0.34F + 0.04F * std::sin(sceneTime * 5.2F + static_cast<float>(gi))) * kCellWorld;
            Spark::Vector2 gpos2 = base;
            gpos2.y += bob;
            const float dx = gpos2.x - p.x;
            const float dy = gpos2.y - p.y;
            const float d2 = dx * dx + dy * dy;
            if (d2 < mr2 && d2 > 1.0e-4F) {
                const float pull = std::min(1.0F, (mr2 - d2) / mr2) * 9.0F * dt;
                gpos2.x -= dx * pull;
                gpos2.y -= dy * pull;
            }
            gtr->SetTranslation({gpos2.x, gpos2.y, 0.055F + 0.0001F * static_cast<float>(gi)});
            gtr->SetScale({pulse, pulse, 1.0F});

            if (d2 <= cr2) {
                explosions.SpawnGemPickup(gpos2.x, gpos2.y);
                world.DestroyGameObject(gem);
                gemObjects.RemoveAt(gi);
                gemBasePositions.RemoveAt(gi);
                ++gemsCollected;
                if (playerGo != nullptr && sfxCoin.Get() != nullptr) {
                    DemoAudio::QueueCue(*playerGo, sfxCoin, 0.92F);
                } else {
                    DemoPlayProceduralClip(context, DemoSfx::ClipGemCollect(), 0.9F);
                }
                continue;
            }
            ++gi;
        }
    }

    if (fpsText != nullptr) {
        const std::string hud = std::format(
                "gems {}/{}  ghosts {}/{}  hash {}/{}",
                gemsCollected,
                gemsTotal,
                enemiesDefeated,
                kEnemyCount,
                lastBroadCandidates,
                lastNarrowHits);
        fpsText->SetText(Spark::Utf8String(hud.c_str()));
    }
}

void BroadPhase2DDemo::Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context)
{
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
            Spark::Vector3{0.18F, 0.62F, 0.88F}.Normalized(),
            Spark::Vector3{0.95F, 0.92F, 0.88F},
            0.95F,
            Spark::Vector3{0.10F, 0.12F, 0.18F},
            true,
            pr,
            pu,
            sceneTime,
            Spark::SceneSpriteSortMode::SortOrderThenWorldY);
}

}  // namespace Spark

