#include "spark/scene/material/MaterialLibraryBinding.hpp"

#include "spark/scene/assets/CachedAssetKind.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/material/MaterialAsset.hpp"

namespace Spark {

void MaterialLibraryBinding::Assign(GameWorld& world, const char* assetKey) {
    Release(world);
    key = (assetKey != nullptr && assetKey[0] != '\0') ? Utf8String(assetKey) : Utf8String{};
    if (key.IsEmpty()) {
        pendingApply = false;
        return;
    }
    world.RetainAsset(CachedAssetKind::Material, key.CStr());
    pendingApply = true;
    if (world.TryGetMaterialByKeyOrPath(key.CStr()) == nullptr) {
        world.RequestMaterial(key.CStr());
    }
}

void MaterialLibraryBinding::Retain(GameWorld& world, const char* assetKey) {
    Release(world);
    key = (assetKey != nullptr && assetKey[0] != '\0') ? Utf8String(assetKey) : Utf8String{};
    pendingApply = false;
    if (!key.IsEmpty()) {
        world.RetainAsset(CachedAssetKind::Material, key.CStr());
    }
}

void MaterialLibraryBinding::Release(GameWorld& world) {
    if (!key.IsEmpty()) {
        world.ReleaseAsset(CachedAssetKind::Material, key.CStr());
        key = {};
    }
    pendingApply = false;
}

bool MaterialLibraryBinding::TryApply(GameWorld& world, const ApplyFn& applyFn) {
    if (!pendingApply || key.IsEmpty() || !applyFn) {
        return false;
    }
    if (const MaterialAsset* asset = world.TryGetMaterialByKeyOrPath(key.CStr())) {
        applyFn(*asset);
        pendingApply = false;
        return true;
    }
    return false;
}

}  // namespace Spark
