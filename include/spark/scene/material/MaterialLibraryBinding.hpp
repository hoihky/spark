#pragma once

#include "spark/core/Utf8String.hpp"

#include <functional>

namespace Spark {

class GameWorld;
class MaterialAsset;

/**
 * Owns retain/release and async-apply state for a single material library key.
 * Shared by <c>MaterialComponent</c> and <c>MultiMaterialComponent::Slot</c> bindings.
 */
class MaterialLibraryBinding {
public:
    using ApplyFn = std::function<void(const MaterialAsset&)>;

    [[nodiscard]] const Utf8String& GetKey() const noexcept { return key; }
    [[nodiscard]] bool HasKey() const noexcept { return !key.IsEmpty(); }
    [[nodiscard]] bool IsPendingApply() const noexcept { return pendingApply; }

    void Assign(GameWorld& world, const char* assetKey);
    /** Retain a library key for cache lifetime without scheduling <c>TryApply</c>. */
    void Retain(GameWorld& world, const char* assetKey);
    void Release(GameWorld& world);
    bool TryApply(GameWorld& world, const ApplyFn& applyFn);

private:
    Utf8String key;
    bool pendingApply = false;
};

}  // namespace Spark
