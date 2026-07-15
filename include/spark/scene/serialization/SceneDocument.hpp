#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"

#include <cstdint>

namespace Spark {

/** One serialized component line: kind tag + opaque payload (space-separated fields). */
struct ComponentRecord {
    Utf8String kind;
    Utf8String payload;
};

/** One ECS entity snapshot with stable id for parent linking. */
struct EntityRecord {
    std::uint64_t id = 0;
    Utf8String name;
    std::int64_t parentId = -1;
    Array<ComponentRecord> components;
};

/** Optional document metadata (v4 header lines). */
struct SceneDocumentHeader {
    Utf8String name;
    /** Relative assets root written into the file; empty = use apply-time default. */
    Utf8String assetsRoot;
    std::uint64_t sceneUid = 0;
};

/** In-memory scene document. Writers emit v4; readers accept v3 and v4. */
struct SceneDocument {
    static constexpr const char* kMagic = "spark_scene_v4";
    static constexpr const char* kMagicV3 = "spark_scene_v3";

    SceneDocumentHeader header;
    Array<EntityRecord> entities;
};

}  // namespace Spark
