#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

class GameObject;

namespace ComponentSnapshotPayload {

[[nodiscard]] bool KindTagEquals(const Utf8String& kind, const char* tag) noexcept;

/** Reads a double-quoted string; advances <c>cursor</c> past the closing quote. */
[[nodiscard]] bool ParseLeadingQuotedString(const char*& cursor, char* out, std::size_t outCap) noexcept;

void AppendQuotedString(Utf8String& out, const char* text);

/** Writes a GameObject entity id for cross-reference fields (0 = none). */
void AppendEntityRef(Utf8String& out, const GameObject* object) noexcept;

/** Parses one entity id token; returns false when the token is missing or invalid. */
[[nodiscard]] bool ParseEntityRef(const char*& cursor, std::uint64_t& outEntityId) noexcept;

/** Advances past <c>count</c> whitespace-delimited tokens (safe when <c>cursor</c> starts on whitespace). */
void SkipTokens(const char*& cursor, int count) noexcept;

/** Resolves an entity id captured during scene apply (requires <c>ctx.entityLookup</c>). */
[[nodiscard]] GameObject* ResolveEntityRef(
        const SceneApplyContext& ctx,
        std::uint64_t entityId) noexcept;

}  // namespace ComponentSnapshotPayload

}  // namespace Spark
