#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/assets/CachedAssetKind.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

class GameWorld;
class MaterialComponent;

/**
 * Facade over <c>GameWorld</c> material-library APIs for demos and lightweight editor tooling.
 * Encapsulates save → async load → apply → release and optional texture-atlas packing.
 */
class MaterialLibraryWorkflow final {
public:
    enum class AsyncState {
        Idle,
        Pending,
        Ready,
        Failed,
    };

    static constexpr const char* kDefaultAssetKey = "materials/matshow_library.sparkmat";

    void Reset();
    void SetPaths(Utf8String assetKey, Utf8String filePath);

    [[nodiscard]] bool TrySaveFromComponent(GameWorld& world, const MaterialComponent& source);
    void RequestAsyncLoad(GameWorld& world);
    void PollAsyncLoad(GameWorld& world, MaterialComponent* previewTarget);
    void ApplyAssetTo(GameWorld& world, MaterialComponent& target);
    void ReleaseBinding(GameWorld& world, MaterialComponent& target);
    [[nodiscard]] bool TryBuildAtlas(GameWorld& world, const char* const* textureKeys, std::size_t count);

    [[nodiscard]] AsyncState GetAsyncState() const noexcept { return asyncState; }
    [[nodiscard]] const Utf8String& GetLastMessage() const noexcept { return lastMessage; }
    [[nodiscard]] const Utf8String& GetAssetKey() const noexcept { return assetKey; }
    [[nodiscard]] std::uint32_t GetMaterialRetainCount(GameWorld& world) const;

private:
    void SetMessage(Utf8String message);

    Utf8String assetKey;
    Utf8String filePath;
    AsyncState asyncState = AsyncState::Idle;
    Utf8String lastMessage;
};

}  // namespace Spark
