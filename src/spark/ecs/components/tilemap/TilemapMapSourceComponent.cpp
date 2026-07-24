#include "spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp"

#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectSpawnComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/tilemap/TilemapDocumentSerializer.hpp"
#include "spark/scene/tilemap/TilemapFileResolve.hpp"
#include "spark/scene/tilemap/TmxImporter.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>

namespace Spark {

namespace {

std::int64_t FileTimestampNs(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return -1;
    }
    std::error_code ec{};
    const auto time = std::filesystem::last_write_time(std::filesystem::path(path), ec);
    if (ec) {
        return -1;
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
}

}  // namespace

void TilemapMapSourceComponent::SetTmxPath(const char* path) noexcept {
    tmxPath = Utf8String(path != nullptr ? path : "");
    lastSourceTimestampNs_ = -1;
}

void TilemapMapSourceComponent::SetSparkMapPath(const char* path) noexcept {
    sparkMapPath = Utf8String(path != nullptr ? path : "");
    lastSourceTimestampNs_ = -1;
}

bool TilemapMapSourceComponent::ImportFromSources(GameObject& owner, GameWorld& world, Utf8String& outError) {
    TilemapDocument document{};
    if (!sparkMapPath.IsEmpty()) {
        TilemapDocumentSerializer serializer{};
        const Utf8String sparkPath = ResolveTilemapAssetPath(sparkMapPath.CStr());
        if (sparkPath.IsEmpty() || !serializer.ReadFromFile(sparkPath.CStr(), document)) {
            outError = Utf8String("Failed to read sparkmap");
            outError.AppendUtf8(sparkMapPath.CStr());
            return false;
        }
    } else if (!tmxPath.IsEmpty()) {
        TmxImporter importer{};
        const TmxImportResult imported = importer.ImportFromFile(tmxPath.CStr(), document);
        if (!imported.success) {
            outError = imported.errorMessage;
            return false;
        }
    } else {
        outError = Utf8String("No TMX or sparkmap path set");
        return false;
    }

    const TilemapDocumentApplyResult applied = ApplyTilemapDocument(document, owner, world, applyOptions);
    if (!applied.success) {
        outError = applied.errorMessage;
        return false;
    }

    if (TilemapGameplayGridComponent* grid = owner.GetComponent<TilemapGameplayGridComponent>()) {
        grid->RequestRebake();
        grid->RebakeIfNeeded(owner);
    }
    if (TilemapObjectSpawnComponent* spawn = owner.GetComponent<TilemapObjectSpawnComponent>()) {
        spawn->RespawnAll(owner, world);
    }

    TouchSourceTimestamp();
    return true;
}

void TilemapMapSourceComponent::TouchSourceTimestamp() {
  const char* path = !sparkMapPath.IsEmpty() ? sparkMapPath.CStr() : tmxPath.CStr();
    lastSourceTimestampNs_ = FileTimestampNs(path);
}

bool TilemapMapSourceComponent::ImportNow(GameObject& owner, GameWorld& world) {
    Utf8String error{};
    const bool ok = ImportFromSources(owner, world, error);
    lastError_ = ok ? Utf8String{} : error;
    return ok;
}

void TilemapMapSourceComponent::OnAttach(GameObject& owner) {
    if (!importOnAttach) {
        return;
    }
    GameWorld& world = owner.GetWorld();
    static_cast<void>(ImportNow(owner, world));
}

void TilemapMapSourceComponent::OnUpdate(
        const FrameTiming& /*timing*/,
        GameObject& owner,
        IEngineContext& /*context*/) {
    if (!hotReload) {
        return;
    }
    const char* path = !sparkMapPath.IsEmpty() ? sparkMapPath.CStr() : tmxPath.CStr();
    const std::int64_t stamp = FileTimestampNs(path);
    if (stamp < 0 || stamp == lastSourceTimestampNs_) {
        return;
    }
    GameWorld& world = owner.GetWorld();
    static_cast<void>(ImportNow(owner, world));
}

}  // namespace Spark
