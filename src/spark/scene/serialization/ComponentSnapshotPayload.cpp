#include "spark/scene/serialization/ComponentSnapshotPayload.hpp"

#include "spark/core/HashMap.hpp"
#include "spark/ecs/GameObject.hpp"

#include <cstdio>
#include <cstring>

namespace Spark::ComponentSnapshotPayload {

bool KindTagEquals(const Utf8String& kind, const char* tag) noexcept {
    return tag != nullptr && std::strcmp(kind.CStr(), tag) == 0;
}

bool ParseLeadingQuotedString(const char*& cursor, char* out, const std::size_t outCap) noexcept {
    if (outCap == 0) {
        return false;
    }
    while (*cursor == ' ' || *cursor == '\t') {
        ++cursor;
    }
    if (*cursor != '"') {
        return false;
    }
    ++cursor;
    std::size_t n = 0;
    while (*cursor != '\0' && *cursor != '"') {
        if (*cursor == '\\' && cursor[1] == '"') {
            if (n + 1 < outCap) {
                out[n++] = '"';
            }
            ++cursor;
            continue;
        }
        if (n + 1 < outCap) {
            out[n++] = *cursor;
        }
        ++cursor;
    }
    if (*cursor != '"') {
        return false;
    }
    ++cursor;
    out[n] = '\0';
    return true;
}

void AppendQuotedString(Utf8String& out, const char* text) {
    out.AppendUtf8("\"");
    if (text != nullptr) {
        for (const char* p = text; *p != '\0'; ++p) {
            if (*p == '"' || *p == '\\') {
                out.AppendUtf8("\\");
            }
            char ch[2]{*p, '\0'};
            out.AppendUtf8(ch);
        }
    }
    out.AppendUtf8("\"");
}

void AppendEntityRef(Utf8String& out, const GameObject* object) noexcept {
    char buf[32]{};
    const std::uint64_t id = object != nullptr ? object->GetId() : 0U;
    std::snprintf(buf, sizeof(buf), "%llu ", static_cast<unsigned long long>(id));
    out.AppendUtf8(buf);
}

bool ParseEntityRef(const char*& cursor, std::uint64_t& outEntityId) noexcept {
    while (*cursor == ' ' || *cursor == '\t') {
        ++cursor;
    }
    unsigned long long parsed = 0;
    if (std::sscanf(cursor, "%llu", &parsed) != 1) {
        return false;
    }
    outEntityId = static_cast<std::uint64_t>(parsed);
    while (*cursor != '\0' && *cursor != ' ') {
        ++cursor;
    }
    return true;
}

void SkipTokens(const char*& cursor, const int count) noexcept {
    for (int i = 0; i < count; ++i) {
        while (*cursor == ' ' || *cursor == '\t') {
            ++cursor;
        }
        while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t') {
            ++cursor;
        }
    }
}

GameObject* ResolveEntityRef(const SceneApplyContext& ctx, const std::uint64_t entityId) noexcept {
    if (entityId == 0U || ctx.entityLookup == nullptr) {
        return nullptr;
    }
    if (GameObject* const* found = ctx.entityLookup->Find(entityId)) {
        return *found;
    }
    return nullptr;
}

}  // namespace Spark::ComponentSnapshotPayload
