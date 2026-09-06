#include "spark/scene/assets/gltf/GltfAnimationEvents.hpp"

#include "spark/animation/AnimationClipEvent.hpp"
#include "spark/animation/Skeleton.hpp"
#include "spark/core/Array.hpp"
#include "spark/core/Utility.hpp"
#include "spark/core/Utf8String.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Spark {

namespace {

const char* SkipWs(const char* p) noexcept {
    while (p != nullptr && *p != '\0' && std::isspace(static_cast<unsigned char>(*p)) != 0) {
        ++p;
    }
    return p;
}

bool MatchLiteral(const char*& p, const char* literal) noexcept {
    p = SkipWs(p);
    if (p == nullptr || literal == nullptr) {
        return false;
    }
    const std::size_t n = std::strlen(literal);
    if (std::strncmp(p, literal, n) != 0) {
        return false;
    }
    p += n;
    return true;
}

bool ParseJsonString(const char*& p, Utf8String& out) noexcept {
    p = SkipWs(p);
    if (p == nullptr || *p != '"') {
        return false;
    }
    ++p;
    out.Clear();
    while (*p != '\0' && *p != '"') {
        if (*p == '\\' && p[1] != '\0') {
            ++p;
        }
        const char ch[2] = {*p, '\0'};
        out.AppendUtf8(ch);
        ++p;
    }
    if (*p != '"') {
        return false;
    }
    ++p;
    return true;
}

bool ParseJsonNumber(const char*& p, float& out) noexcept {
    p = SkipWs(p);
    if (p == nullptr || *p == '\0') {
        return false;
    }
    char* end = nullptr;
    const float v = strtof(p, &end);
    if (end == p) {
        return false;
    }
    out = v;
    p = end;
    return true;
}

bool ParseEventObject(const char*& p, AnimationClipEvent& out) noexcept {
    p = SkipWs(p);
    if (!MatchLiteral(p, "{")) {
        return false;
    }
    bool haveTime = false;
    bool haveName = false;
    while (true) {
        p = SkipWs(p);
        if (*p == '}') {
            ++p;
            break;
        }
        Utf8String key{};
        if (!ParseJsonString(p, key)) {
            return false;
        }
        if (!MatchLiteral(p, ":")) {
            return false;
        }
        if (key == Utf8String("time")) {
            if (!ParseJsonNumber(p, out.timeSeconds)) {
                return false;
            }
            haveTime = true;
        } else if (key == Utf8String("name")) {
            if (!ParseJsonString(p, out.name)) {
                return false;
            }
            haveName = true;
        } else {
            p = SkipWs(p);
            if (*p == '"') {
                Utf8String ignored{};
                if (!ParseJsonString(p, ignored)) {
                    return false;
                }
            } else if (*p == '{' || *p == '[') {
                int depth = 1;
                const char open = *p;
                const char close = (open == '{') ? '}' : ']';
                ++p;
                while (*p != '\0' && depth > 0) {
                    if (*p == open) {
                        ++depth;
                    } else if (*p == close) {
                        --depth;
                    } else if (*p == '"') {
                        ++p;
                        while (*p != '\0' && *p != '"') {
                            if (*p == '\\' && p[1] != '\0') {
                                ++p;
                            }
                            ++p;
                        }
                    }
                    if (depth > 0) {
                        ++p;
                    }
                }
            } else {
                while (*p != '\0' && *p != ',' && *p != '}') {
                    ++p;
                }
            }
        }
        p = SkipWs(p);
        if (*p == ',') {
            ++p;
        }
    }
    return haveTime && haveName && !out.name.IsEmpty();
}

bool ParseEventsArray(const char*& p, Array<AnimationClipEvent>& out) noexcept {
    p = SkipWs(p);
    if (!MatchLiteral(p, "[")) {
        return false;
    }
    while (true) {
        p = SkipWs(p);
        if (*p == ']') {
            ++p;
            return true;
        }
        AnimationClipEvent event{};
        if (!ParseEventObject(p, event)) {
            return false;
        }
        out.PushBack(MoveTemp(event));
        p = SkipWs(p);
        if (*p == ',') {
            ++p;
            continue;
        }
        if (*p == ']') {
            ++p;
            return true;
        }
        return false;
    }
}

bool FindJsonArrayAfterKey(const char* json, const char* key, const char*& arrayStart) noexcept {
    if (json == nullptr || key == nullptr) {
        return false;
    }
    const std::size_t keyLen = std::strlen(key);
    for (const char* p = json; p[0] != '\0'; ++p) {
        if (p[0] != '"') {
            continue;
        }
        ++p;
        bool match = true;
        for (std::size_t i = 0; i < keyLen; ++i) {
            if (p[i] == '\0' || p[i] != key[i]) {
                match = false;
                break;
            }
        }
        if (!match || p[keyLen] != '"') {
            continue;
        }
        p += keyLen + 1;
        p = SkipWs(p);
        if (*p != ':') {
            continue;
        }
        ++p;
        p = SkipWs(p);
        if (*p != '[') {
            continue;
        }
        arrayStart = p;
        return true;
    }
    return false;
}

bool ParseSparkEventsJson(const char* json, Array<AnimationClipEvent>& out) noexcept {
    if (json == nullptr) {
        return false;
    }
    const char* arrayStart = nullptr;
    if (!FindJsonArrayAfterKey(json, "sparkEvents", arrayStart)) {
        return false;
    }
    const char* cursor = arrayStart;
    return ParseEventsArray(cursor, out);
}

bool ParseSidecarJson(const char* json, Skeleton& skeleton) noexcept {
    if (json == nullptr) {
        return false;
    }
    const char* clipsArray = nullptr;
    if (!FindJsonArrayAfterKey(json, "clips", clipsArray)) {
        return false;
    }
    const char* cursor = clipsArray;
    if (!MatchLiteral(cursor, "[")) {
        return false;
    }
    while (true) {
        cursor = SkipWs(cursor);
        if (*cursor == ']') {
            ++cursor;
            return true;
        }
        if (!MatchLiteral(cursor, "{")) {
            return false;
        }
        Utf8String clipName{};
        Array<AnimationClipEvent> events{};
        while (true) {
            cursor = SkipWs(cursor);
            if (*cursor == '}') {
                ++cursor;
                break;
            }
            Utf8String key{};
            if (!ParseJsonString(cursor, key)) {
                return false;
            }
            if (!MatchLiteral(cursor, ":")) {
                return false;
            }
            if (key == Utf8String("name")) {
                if (!ParseJsonString(cursor, clipName)) {
                    return false;
                }
            } else if (key == Utf8String("events")) {
                if (!ParseEventsArray(cursor, events)) {
                    return false;
                }
            } else {
                cursor = SkipWs(cursor);
                if (*cursor == '"') {
                    Utf8String ignored{};
                    if (!ParseJsonString(cursor, ignored)) {
                        return false;
                    }
                } else if (*cursor == '{' || *cursor == '[') {
                    int depth = 1;
                    const char open = *cursor;
                    const char close = (open == '{') ? '}' : ']';
                    ++cursor;
                    while (*cursor != '\0' && depth > 0) {
                        if (*cursor == open) {
                            ++depth;
                        } else if (*cursor == close) {
                            --depth;
                        } else if (*cursor == '"') {
                            ++cursor;
                            while (*cursor != '\0' && *cursor != '"') {
                                if (*cursor == '\\' && cursor[1] != '\0') {
                                    ++cursor;
                                }
                                ++cursor;
                            }
                        }
                        if (depth > 0) {
                            ++cursor;
                        }
                    }
                } else {
                    while (*cursor != '\0' && *cursor != ',' && *cursor != '}') {
                        ++cursor;
                    }
                }
            }
            cursor = SkipWs(cursor);
            if (*cursor == ',') {
                ++cursor;
            }
        }
        if (!clipName.IsEmpty() && !events.IsEmpty()) {
            const std::int32_t clipIdx = skeleton.FindClipIndexByNameCaseInsensitive(clipName.CStr());
            if (clipIdx >= 0) {
                skeleton.SetClipEvents(static_cast<std::uint32_t>(clipIdx), MoveTemp(events));
            }
        }
        cursor = SkipWs(cursor);
        if (*cursor == ',') {
            ++cursor;
            continue;
        }
        if (*cursor == ']') {
            ++cursor;
            return true;
        }
        return false;
    }
}

bool ReadTextFile(const char* path, Array<char>& out) noexcept {
    out.Clear();
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    std::FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return false;
    }
    const long size = std::ftell(file);
    if (size <= 0) {
        std::fclose(file);
        return false;
    }
    if (std::fseek(file, 0, SEEK_SET) != 0) {
        std::fclose(file);
        return false;
    }
    out.Resize(static_cast<std::size_t>(size));
    const std::size_t read = std::fread(out.GetData(), 1, out.GetSize(), file);
    std::fclose(file);
    if (read != out.GetSize()) {
        out.Clear();
        return false;
    }
    return true;
}

Utf8String SidecarPathForGltf(const char* gltfPath) noexcept {
    Utf8String path(gltfPath != nullptr ? gltfPath : "");
    const char* c = path.CStr();
    const char* dot = std::strrchr(c, '.');
    if (dot != nullptr) {
        Utf8String stem;
        for (const char* p = c; p < dot; ++p) {
            const char ch[2] = {*p, '\0'};
            stem.AppendUtf8(ch);
        }
        path = MoveTemp(stem);
    }
    path.AppendUtf8(".spark-anim-events.json");
    return path;
}

void MergeClipEvents(Skeleton& skeleton, const std::uint32_t clipIndex, Array<AnimationClipEvent>&& events) {
    if (events.IsEmpty() || clipIndex >= skeleton.GetClipCount()) {
        return;
    }
    Array<AnimationClipEvent> merged = skeleton.GetClipEvents(clipIndex);
    for (std::size_t i = 0; i < events.GetSize(); ++i) {
        merged.PushBack(MoveTemp(events[i]));
    }
    skeleton.SetClipEvents(clipIndex, MoveTemp(merged));
}

}  // namespace

void LoadGltfAnimationEvents(cgltf_data* /*data*/, const char* gltfPath, Skeleton& skeleton) noexcept {
    if (gltfPath == nullptr || skeleton.GetClipCount() == 0) {
        return;
    }

    Array<char> sidecarText{};
    const Utf8String sidecarPath = SidecarPathForGltf(gltfPath);
    if (ReadTextFile(sidecarPath.CStr(), sidecarText)) {
        sidecarText.PushBack('\0');
        (void)ParseSidecarJson(sidecarText.GetData(), skeleton);
    }
}

}  // namespace Spark
