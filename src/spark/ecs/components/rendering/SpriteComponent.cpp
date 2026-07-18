#include "spark/ecs/components/rendering/SpriteComponent.hpp"

namespace Spark {

SpriteComponent::SpriteComponent(
        SharedPtr<Texture2D> inTexture,
        const Vector4& inTint,
        const Vector4& inUvRect,
        std::int32_t inSortOrder) noexcept
        : texture(MoveTemp(inTexture)), tint(inTint), uvRect(inUvRect), sortOrder(inSortOrder) {}

}  // namespace Spark
