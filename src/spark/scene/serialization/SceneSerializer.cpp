#include "spark/scene/serialization/SceneSerializer.hpp"

#include "spark/core/HashMap.hpp"
#include "spark/core/Utility.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/SceneInstanceId.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"

#include <cstdio>
#include <cstring>
#include <functional>

namespace Spark {

namespace {

constexpr ComponentKind kCaptureOrder[] = {
        ComponentKind::Transform,
        ComponentKind::Mesh,
        ComponentKind::Material,
        ComponentKind::SkinnedMesh,
        ComponentKind::Animator,
        ComponentKind::DirectionalLight,
        ComponentKind::PointLight,
        ComponentKind::SpotLight,
        ComponentKind::Camera,
        ComponentKind::Camera2D,
        ComponentKind::Camera2DRig,
        ComponentKind::Sky,
        ComponentKind::Sprite,
        ComponentKind::RenderLayer,
        ComponentKind::SortingGroup,
        ComponentKind::SceneSpatialPolicy,
        ComponentKind::TextOverlay,
        ComponentKind::ParticleEmitter,
        ComponentKind::Terrain,
        ComponentKind::BoxCollider3D,
        ComponentKind::SphereCollider3D,
        ComponentKind::CapsuleCollider3D,
        ComponentKind::Rigidbody3D,
        ComponentKind::CharacterController3D,
        ComponentKind::TriggerVolume3D,
        ComponentKind::PhysicsMaterial3D,
        ComponentKind::AudioListener,
        ComponentKind::Billboard,
        ComponentKind::AnimationEventReceiver,
        ComponentKind::AttachmentSocket,
        ComponentKind::CameraFollow3D,
        ComponentKind::SpringArm3D,
        ComponentKind::PolygonCollider2D,
        ComponentKind::Health,
        ComponentKind::Damageable,
        ComponentKind::DecalProjector,
};

bool WriteEscapedName(std::FILE* f, const Utf8String& name) {
    std::fputc('"', f);
    const char* p = name.CStr();
    while (p != nullptr && *p != '\0') {
        if (*p == '"' || *p == '\\') {
            std::fputc('\\', f);
        }
        std::fputc(*p++, f);
    }
    std::fputc('"', f);
    return true;
}

void AppendHeaderLines(const SceneDocument& document, Utf8String& out) {
    if (!document.header.name.IsEmpty()) {
        out.AppendUtf8("H name \"");
        const char* p = document.header.name.CStr();
        while (p != nullptr && *p != '\0') {
            if (*p == '"' || *p == '\\') {
                out.AppendUtf8("\\");
            }
            char ch[2] = {*p, '\0'};
            out.AppendUtf8(ch);
            ++p;
        }
        out.AppendUtf8("\"\n");
    }
    if (!document.header.assetsRoot.IsEmpty()) {
        out.AppendUtf8("H assets_root \"");
        out.AppendUtf8(document.header.assetsRoot);
        out.AppendUtf8("\"\n");
    }
    if (document.header.sceneUid != 0) {
        char buf[64]{};
        std::snprintf(buf, sizeof(buf), "H scene_uid %llu\n", static_cast<unsigned long long>(document.header.sceneUid));
        out.AppendUtf8(buf);
    }
}

void WriteHeaderLinesToFile(std::FILE* f, const SceneDocument& document) {
    if (!document.header.name.IsEmpty()) {
        std::fprintf(f, "H name ");
        WriteEscapedName(f, document.header.name);
        std::fputc('\n', f);
    }
    if (!document.header.assetsRoot.IsEmpty()) {
        std::fprintf(f, "H assets_root ");
        WriteEscapedName(f, document.header.assetsRoot);
        std::fputc('\n', f);
    }
    if (document.header.sceneUid != 0) {
        std::fprintf(f, "H scene_uid %llu\n", static_cast<unsigned long long>(document.header.sceneUid));
    }
}

bool ReadQuotedString(const char*& cursor, char* out, std::size_t outCap);

bool ParseHeaderLine(const char* line, SceneDocumentHeader& header) {
    if (line == nullptr || line[0] != 'H' || line[1] != ' ') {
        return false;
    }
    const char* cursor = line + 2;
    char key[64]{};
    if (std::sscanf(cursor, "%63s", key) != 1) {
        return false;
    }
    while (*cursor != '\0' && *cursor != ' ') {
        ++cursor;
    }
    while (*cursor == ' ') {
        ++cursor;
    }
    if (std::strcmp(key, "name") == 0) {
        char nameBuf[256]{};
        if (ReadQuotedString(cursor, nameBuf, sizeof(nameBuf))) {
            header.name = Utf8String(nameBuf);
        }
        return true;
    }
    if (std::strcmp(key, "assets_root") == 0) {
        char rootBuf[384]{};
        if (ReadQuotedString(cursor, rootBuf, sizeof(rootBuf))) {
            header.assetsRoot = Utf8String(rootBuf);
        }
        return true;
    }
    if (std::strcmp(key, "scene_uid") == 0) {
        unsigned long long uid = 0;
        if (std::sscanf(cursor, "%llu", &uid) == 1) {
            header.sceneUid = static_cast<std::uint64_t>(uid);
        }
        return true;
    }
    return true;
}

bool ReadQuotedString(const char*& cursor, char* out, std::size_t outCap) {
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
        if (*cursor == '\\' && cursor[1] != '\0') {
            ++cursor;
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

bool ParseEntityHeader(const char* line, EntityRecord& entity) {
    if (line == nullptr || line[0] != 'E' || line[1] != ' ') {
        return false;
    }
    const char* cursor = line + 2;
    char nameBuf[256]{};
    unsigned long long id = 0;
    long long parent = -1;
    if (std::sscanf(cursor, "%llu %lld", &id, &parent) < 2) {
        return false;
    }
    while (*cursor != '\0' && *cursor != ' ') {
        ++cursor;
    }
    while (*cursor == ' ') {
        ++cursor;
    }
    while (*cursor != '\0' && *cursor != ' ') {
        ++cursor;
    }
    while (*cursor == ' ') {
        ++cursor;
    }
    if (!ReadQuotedString(cursor, nameBuf, sizeof(nameBuf))) {
        entity.name = Utf8String("Entity");
    } else {
        entity.name = Utf8String(nameBuf);
    }
    entity.id = static_cast<std::uint64_t>(id);
    entity.parentId = parent;
    return true;
}

bool ParseComponentLine(const char* line, ComponentRecord& component) {
    if (line == nullptr || line[0] != 'C' || line[1] != ' ') {
        return false;
    }
    const char* cursor = line + 2;
    char kindBuf[64]{};
    std::size_t ki = 0;
    while (*cursor != '\0' && *cursor != ' ' && ki + 1 < sizeof(kindBuf)) {
        kindBuf[ki++] = *cursor++;
    }
    kindBuf[ki] = '\0';
    while (*cursor == ' ') {
        ++cursor;
    }
    component.kind = Utf8String(kindBuf);
    component.payload = Utf8String(cursor);
    return !component.kind.IsEmpty();
}

}  // namespace

SceneSerializer::SceneSerializer(const ComponentSnapshotRegistry& inRegistry) : registry(inRegistry) {}

SceneDocument SceneSerializer::Capture(
        const GameWorld& world,
        const SceneCaptureContext& ctx,
        const std::function<bool(const GameObject*)>& includeEntity) const {
    SceneDocument doc;
    world.ForEachGameObject([&](const GameObject* object) {
        if (object == nullptr) {
            return;
        }
        if (includeEntity && !includeEntity(object)) {
            return;
        }
        EntityRecord entity;
        entity.id = object->GetId();
        entity.name = object->GetName();
        entity.parentId = object->GetParent() != nullptr ? static_cast<std::int64_t>(object->GetParent()->GetId())
                                                         : -1;

        for (const ComponentKind kind : kCaptureOrder) {
            const IComponentSnapshotHandler* handler = registry.Find(kind);
            if (handler == nullptr || object->TryGetComponentByKind(kind) == nullptr) {
                continue;
            }
            ComponentRecord record;
            if (handler->TryCapture(*object, ctx, record)) {
                entity.components.PushBack(MoveTemp(record));
            }
        }
        if (!entity.components.IsEmpty()) {
            doc.entities.PushBack(MoveTemp(entity));
        }
    });
    return doc;
}

bool SceneSerializer::WriteToString(const SceneDocument& document, Utf8String& out) const {
    out.Clear();
    out.AppendUtf8(SceneDocument::kMagic);
    out.AppendUtf8("\n");
    char countBuf[32]{};
    std::snprintf(countBuf, sizeof(countBuf), "%zu\n", document.entities.GetSize());
    out.AppendUtf8(countBuf);
    AppendHeaderLines(document, out);
    for (std::size_t ei = 0; ei < document.entities.GetSize(); ++ei) {
        const EntityRecord& e = document.entities[ei];
        char header[128]{};
        std::snprintf(header, sizeof(header), "E %llu %lld ", static_cast<unsigned long long>(e.id), e.parentId);
        out.AppendUtf8(header);
        out.AppendUtf8("\"");
        const char* np = e.name.CStr();
        while (np != nullptr && *np != '\0') {
            if (*np == '"' || *np == '\\') {
                out.AppendUtf8("\\");
            }
            char ch[2] = {*np, '\0'};
            out.AppendUtf8(ch);
            ++np;
        }
        out.AppendUtf8("\"\n");
        for (std::size_t ci = 0; ci < e.components.GetSize(); ++ci) {
            out.AppendUtf8("C ");
            out.AppendUtf8(e.components[ci].kind.CStr());
            out.AppendUtf8(" ");
            out.AppendUtf8(e.components[ci].payload.CStr());
            out.AppendUtf8("\n");
        }
    }
    return true;
}

bool SceneSerializer::WriteToFile(const SceneDocument& document, const char* path) const {
    if (path == nullptr) {
        return false;
    }
    std::FILE* f = std::fopen(path, "w");
    if (f == nullptr) {
        return false;
    }
    std::fprintf(f, "%s\n", SceneDocument::kMagic);
    std::fprintf(f, "%zu\n", document.entities.GetSize());
    WriteHeaderLinesToFile(f, document);
    for (std::size_t ei = 0; ei < document.entities.GetSize(); ++ei) {
        const EntityRecord& e = document.entities[ei];
        std::fprintf(f, "E %llu %lld ", static_cast<unsigned long long>(e.id), e.parentId);
        WriteEscapedName(f, e.name);
        std::fputc('\n', f);
        for (std::size_t ci = 0; ci < e.components.GetSize(); ++ci) {
            std::fprintf(
                    f,
                    "C %s %s\n",
                    e.components[ci].kind.CStr(),
                    e.components[ci].payload.CStr());
        }
    }
    std::fclose(f);
    return true;
}

bool SceneDeserializer::ReadFromString(const char* text, SceneDocument& out) const {
    out.entities.Clear();
    out.header = SceneDocumentHeader{};
    if (text == nullptr) {
        return false;
    }
    const char* cursor = text;
    char magic[64]{};
    if (std::sscanf(cursor, "%63s", magic) != 1) {
        return false;
    }
    const bool isV4 = std::strcmp(magic, SceneDocument::kMagic) == 0;
    const bool isV3 = std::strcmp(magic, SceneDocument::kMagicV3) == 0;
    if (!isV4 && !isV3) {
        return false;
    }
    cursor = std::strchr(cursor, '\n');
    if (cursor == nullptr) {
        return false;
    }
    ++cursor;
    std::size_t entityCount = 0;
    if (std::sscanf(cursor, "%zu", &entityCount) != 1) {
        return false;
    }
    cursor = std::strchr(cursor, '\n');
    if (cursor == nullptr) {
        return false;
    }
    ++cursor;

    EntityRecord current;
    bool inEntity = false;
    while (*cursor != '\0') {
        const char* lineStart = cursor;
        const char* lineEnd = std::strchr(cursor, '\n');
        if (lineEnd == nullptr) {
            lineEnd = cursor + std::strlen(cursor);
        }
        const std::size_t lineLen = static_cast<std::size_t>(lineEnd - lineStart);
        char line[2048]{};
        const std::size_t copyLen = lineLen < sizeof(line) - 1 ? lineLen : sizeof(line) - 1;
        std::memcpy(line, lineStart, copyLen);
        line[copyLen] = '\0';

        if (line[0] == 'H' && line[1] == ' ') {
            if (!inEntity) {
                ParseHeaderLine(line, out.header);
            }
        } else if (line[0] == 'E' && line[1] == ' ') {
            if (inEntity) {
                out.entities.PushBack(MoveTemp(current));
                current = EntityRecord{};
            }
            if (!ParseEntityHeader(line, current)) {
                return false;
            }
            inEntity = true;
        } else if (line[0] == 'C' && line[1] == ' ' && inEntity) {
            ComponentRecord component;
            if (!ParseComponentLine(line, component)) {
                return false;
            }
            current.components.PushBack(MoveTemp(component));
        }

        if (*lineEnd == '\0') {
            break;
        }
        cursor = lineEnd + 1;
    }
    if (inEntity) {
        out.entities.PushBack(MoveTemp(current));
    }
    return out.entities.GetSize() == entityCount;
}

bool SceneDeserializer::ReadFromFile(const char* path, SceneDocument& out) const {
    if (path == nullptr) {
        return false;
    }
    std::FILE* f = std::fopen(path, "r");
    if (f == nullptr) {
        return false;
    }
    std::fseek(f, 0, SEEK_END);
    const long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz <= 0) {
        std::fclose(f);
        return false;
    }
    Array<char> buf;
    buf.Resize(static_cast<std::size_t>(sz) + 1);
    const std::size_t readN = std::fread(buf.GetData(), 1, static_cast<std::size_t>(sz), f);
    std::fclose(f);
    buf[readN] = '\0';
    return ReadFromString(buf.GetData(), out);
}

bool SceneDeserializer::Apply(
        const SceneDocument& document,
        GameWorld& world,
        const SceneApplyContext& ctx,
        HashMap<std::uint64_t, GameObject*>* outIdToObject) const {
    HashMap<std::uint64_t, GameObject*> idToObject;
    for (std::size_t ei = 0; ei < document.entities.GetSize(); ++ei) {
        const EntityRecord& record = document.entities[ei];
        GameObject* object = world.CreateGameObject();
        object->GetName() = record.name;
        if (ctx.sceneInstanceId != kInvalidSceneInstanceId) {
            object->SetSceneInstanceId(ctx.sceneInstanceId);
        }
        idToObject.Add(record.id, object);
        if (ctx.onEntityCreated != nullptr) {
            ctx.onEntityCreated(object, ctx.entityUserData);
        }
    }

    for (std::size_t ei = 0; ei < document.entities.GetSize(); ++ei) {
        const EntityRecord& record = document.entities[ei];
        GameObject* const* found = idToObject.Find(record.id);
        if (found == nullptr || *found == nullptr) {
            return false;
        }
        GameObject* object = *found;
        for (std::size_t ci = 0; ci < record.components.GetSize(); ++ci) {
            const ComponentRecord& component = record.components[ci];
            const IComponentSnapshotHandler* handler = registry.FindByTag(component.kind.CStr());
            if (handler == nullptr) {
                continue;
            }
            if (!handler->TryRestore(*object, component, world, ctx)) {
                if (std::strcmp(component.kind.CStr(), "skinned_mesh") == 0) {
                    continue;
                }
                return false;
            }
        }
    }

    for (std::size_t ei = 0; ei < document.entities.GetSize(); ++ei) {
        const EntityRecord& record = document.entities[ei];
        if (record.parentId < 0) {
            continue;
        }
        GameObject* const* childFound = idToObject.Find(record.id);
        GameObject* const* parentFound = idToObject.Find(static_cast<std::uint64_t>(record.parentId));
        if (childFound == nullptr || parentFound == nullptr || *childFound == nullptr || *parentFound == nullptr) {
            return false;
        }
        if (!world.SetParent(*childFound, *parentFound)) {
            return false;
        }
    }
    if (outIdToObject != nullptr) {
        *outIdToObject = MoveTemp(idToObject);
    }
    return true;
}

SceneDeserializer::SceneDeserializer(const ComponentSnapshotRegistry& inRegistry) : registry(inRegistry) {}

}  // namespace Spark
