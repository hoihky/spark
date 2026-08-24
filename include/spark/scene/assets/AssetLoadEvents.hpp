#pragma once

#include "spark/core/Utf8String.hpp"

#include <cstdint>

namespace Spark {

enum class AssetLoadJobKind : std::uint8_t {
    Gltf,
    SkinnedGltf,
    Texture,
    MeshObj,
    Material,
};

enum class AssetLoadState : std::uint8_t {
    None,
    Queued,
    Loading,
    Ready,
    Failed,
};

/** Completion notification for one async asset job (dispatched on the main thread from <c>Pump</c>). */
struct AssetLoadEvent {
    Utf8String path;
    AssetLoadJobKind kind = AssetLoadJobKind::Gltf;
    AssetLoadState state = AssetLoadState::None;
    Utf8String errorMessage;

    [[nodiscard]] bool Succeeded() const noexcept { return state == AssetLoadState::Ready; }
    [[nodiscard]] bool Failed() const noexcept { return state == AssetLoadState::Failed; }
};

using AssetLoadCallbackFn = void (*)(const AssetLoadEvent& event, void* userData);

/** Lightweight callback binding (no heap allocation). */
struct AssetLoadCallback {
    AssetLoadCallbackFn fn = nullptr;
    void* userData = nullptr;

    [[nodiscard]] bool IsBound() const noexcept { return fn != nullptr; }

    void Invoke(const AssetLoadEvent& event) const noexcept {
        if (fn != nullptr) {
            fn(event, userData);
        }
    }
};

/** Optional global observer for loading screens / telemetry. Not owned by the loader. */
class IAssetLoadListener {
public:
    virtual ~IAssetLoadListener() = default;
    virtual void OnAssetLoadCompleted(const AssetLoadEvent& event) = 0;
};

}  // namespace Spark
