#include "spark/demo/MaterialLibraryWorkflow.hpp"

#include "spark/config.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/scene/AssetLoadEvents.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/MaterialAsset.hpp"

#include <cstdio>
#include <format>

namespace Spark {

void MaterialLibraryWorkflow::Reset() {
    assetKey = {};
    filePath = {};
    asyncState = AsyncState::Idle;
    lastMessage = {};
}

void MaterialLibraryWorkflow::SetPaths(Utf8String inAssetKey, Utf8String inFilePath) {
    assetKey = MoveTemp(inAssetKey);
    filePath = MoveTemp(inFilePath);
    asyncState = AsyncState::Idle;
}

void MaterialLibraryWorkflow::SetMessage(Utf8String message) {
    lastMessage = MoveTemp(message);
}

bool MaterialLibraryWorkflow::TrySaveFromComponent(GameWorld& world, const MaterialComponent& source) {
    if (filePath.IsEmpty() || assetKey.IsEmpty()) {
        SetMessage(Utf8String("Material workflow paths are not configured."));
        return false;
    }

    MaterialAsset asset{};
    asset.name = assetKey;
    asset.CaptureFromMaterial(source);
    if (!world.SaveMaterialAsset(filePath.CStr(), asset, SPARK_BUILD_ASSETS_DIR, &world.GetAssetCache())) {
        SetMessage(Utf8String("Failed to write .sparkmat file."));
        return false;
    }

    asyncState = AsyncState::Ready;
    SetMessage(Utf8String(std::format("Saved {} to disk", assetKey.CStr()).c_str()));
    return true;
}

void MaterialLibraryWorkflow::RequestAsyncLoad(GameWorld& world) {
    if (assetKey.IsEmpty()) {
        SetMessage(Utf8String("Material workflow asset key is not configured."));
        asyncState = AsyncState::Failed;
        return;
    }

    (void)world.ReleaseAsset(CachedAssetKind::Material, assetKey.CStr());
    world.InvalidateAssetLoadState(assetKey.CStr(), AssetLoadJobKind::Material);

    asyncState = AsyncState::Pending;
    world.RequestMaterial(assetKey.CStr());
    SetMessage(Utf8String(std::format("Loading {}…", assetKey.CStr()).c_str()));
}

void MaterialLibraryWorkflow::PollAsyncLoad(GameWorld& world, MaterialComponent* previewTarget) {
    if (asyncState != AsyncState::Pending || assetKey.IsEmpty()) {
        return;
    }

    const AssetLoadState state = world.GetAssetLoadState(assetKey.CStr(), AssetLoadJobKind::Material);
    if (state == AssetLoadState::Ready) {
        if (previewTarget != nullptr) {
            previewTarget->SetMaterialAsset(world, assetKey.CStr());
        }
        asyncState = AsyncState::Ready;
        SetMessage(Utf8String(std::format(
                "Library material ready (retain={})",
                GetMaterialRetainCount(world))
                                            .c_str()));
        return;
    }
    if (state == AssetLoadState::Failed) {
        asyncState = AsyncState::Failed;
        SetMessage(Utf8String(std::format("Failed to load {}", assetKey.CStr()).c_str()));
    }
}

void MaterialLibraryWorkflow::ApplyAssetTo(GameWorld& world, MaterialComponent& target) {
    if (assetKey.IsEmpty()) {
        SetMessage(Utf8String("Material workflow asset key is not configured."));
        return;
    }
    target.SetMaterialAsset(world, assetKey.CStr());
    SetMessage(Utf8String(std::format(
            "Applied {} to target (retain={})",
            assetKey.CStr(),
            GetMaterialRetainCount(world))
                                        .c_str()));
}

void MaterialLibraryWorkflow::ReleaseBinding(GameWorld& world, MaterialComponent& target) {
    target.ClearMaterialAsset(world);
    SetMessage(Utf8String(std::format(
            "Released library binding (retain={})",
            GetMaterialRetainCount(world))
                                        .c_str()));
}

bool MaterialLibraryWorkflow::TryBuildAtlas(
        GameWorld& world,
        const char* const* textureKeys,
        const std::size_t count) {
    if (textureKeys == nullptr || count == 0U) {
        SetMessage(Utf8String("No textures selected for atlas build."));
        return false;
    }
    if (!world.BuildTextureAtlas("matshow_demo", textureKeys, count)) {
        SetMessage(Utf8String("Texture atlas build failed."));
        return false;
    }
    SetMessage(Utf8String(std::format(
            "Packed {} texture(s) into atlas/matshow_demo (GPU batching; appearance unchanged).",
            count)
                                        .c_str()));
    return true;
}

std::uint32_t MaterialLibraryWorkflow::GetMaterialRetainCount(GameWorld& world) const {
    if (assetKey.IsEmpty()) {
        return 0U;
    }
    return world.GetAssetRetainCount(CachedAssetKind::Material, assetKey.CStr());
}

}  // namespace Spark
