#include "spark/scene/serialization/GltfInstanceOverrides.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/GltfInstanceNodeComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/serialization/MaterialSlotSnapshot.hpp"

#include <cstdio>
#include <cstring>

namespace Spark::GltfInstanceOverrides {

namespace {

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

void ForEachTaggedDescendant(const GameObject& root, const auto& visitor) {
    const auto walk = [&](const GameObject& node, auto&& self) -> void {
        if (const GltfInstanceNodeComponent* tag = node.GetComponent<GltfInstanceNodeComponent>()) {
            visitor(node, tag->GetNodeIndex());
        }
        const Array<GameObject*>& children = node.GetChildren();
        for (std::size_t i = 0; i < children.GetSize(); ++i) {
            if (children[i] != nullptr) {
                self(*children[i], self);
            }
        }
    };
    const Array<GameObject*>& children = root.GetChildren();
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            walk(*children[i], walk);
        }
    }
}

GameObject* FindTaggedNode(GameObject& root, const std::uint32_t nodeIndex) {
    GameObject* found = nullptr;
    ForEachTaggedDescendant(root, [&](const GameObject& node, const std::uint32_t index) {
        if (index == nodeIndex && found == nullptr) {
            found = const_cast<GameObject*>(&node);
        }
    });
    return found;
}

void SkipWhitespace(const char*& cursor) noexcept {
    while (*cursor == ' ' || *cursor == '\t') {
        ++cursor;
    }
}

}  // namespace

void AppendCapturedOverrides(
        const GameObject& prefabRoot,
        const SceneCaptureContext& ctx,
        Utf8String& payload) {
    ForEachTaggedDescendant(prefabRoot, [&](const GameObject& node, const std::uint32_t nodeIndex) {
        if (const MeshComponent* mesh = node.GetComponent<MeshComponent>()) {
            const Vector3& albedo = mesh->GetAlbedo();
            char record[128]{};
            std::snprintf(
                    record,
                    sizeof(record),
                    " mesh %u %.6f %.6f %.6f",
                    nodeIndex,
                    albedo.x,
                    albedo.y,
                    albedo.z);
            payload.AppendUtf8(record);
        }
        if (const MaterialComponent* material = node.GetComponent<MaterialComponent>()) {
            char indexBuf[16]{};
            std::snprintf(indexBuf, sizeof(indexBuf), "%u", nodeIndex);
            payload.AppendUtf8(" mat ");
            payload.AppendUtf8(indexBuf);
            if (material->HasMaterialAsset()) {
                payload.AppendUtf8(" v4 \"");
                payload.AppendUtf8(material->GetMaterialAssetKey().CStr());
                payload.AppendUtf8("\"");
            }
            MaterialSlotSnapshot::Data slotData{};
            MaterialSlotSnapshot::CaptureFromMaterial(*material, node, ctx, slotData);
            MaterialSlotSnapshot::AppendSlotV1(slotData, payload);
        }
    });
}

void ApplyFromPayloadCursor(
        GameObject& prefabRoot,
        const char* cursor,
        GameWorld& world,
        const SceneApplyContext& ctx) {
    if (cursor == nullptr) {
        return;
    }
    while (true) {
        SkipWhitespace(cursor);
        if (*cursor == '\0') {
            break;
        }
        if (std::strncmp(cursor, "mesh ", 5) == 0) {
            cursor += 5;
            unsigned int nodeIndex = 0U;
            float r = 1.0F;
            float g = 1.0F;
            float b = 1.0F;
            int consumed = 0;
            if (std::sscanf(cursor, "%u %f %f %f%n", &nodeIndex, &r, &g, &b, &consumed) < 4) {
                break;
            }
            cursor += consumed;
            if (GameObject* target = FindTaggedNode(prefabRoot, nodeIndex)) {
                if (MeshComponent* mesh = target->GetComponent<MeshComponent>()) {
                    mesh->SetAlbedo({r, g, b});
                }
            }
            continue;
        }
        if (std::strncmp(cursor, "mat ", 4) == 0) {
            cursor += 4;
            unsigned int nodeIndex = 0U;
            int consumed = 0;
            if (std::sscanf(cursor, "%u%n", &nodeIndex, &consumed) != 1) {
                break;
            }
            cursor += consumed;
            SkipWhitespace(cursor);
            GameObject* target = FindTaggedNode(prefabRoot, nodeIndex);
            if (target == nullptr) {
                break;
            }
            MaterialComponent* material = target->GetComponent<MaterialComponent>();
            if (material == nullptr) {
                material = target->AddComponent<MaterialComponent>();
            }
            if (std::strncmp(cursor, "v4 ", 3) == 0) {
                cursor += 3;
                char assetKey[384]{};
                if (!ParseLeadingQuotedString(cursor, assetKey, sizeof(assetKey))) {
                    break;
                }
                material->SetMaterialAsset(world, assetKey);
                SkipWhitespace(cursor);
            }
            MaterialSlotSnapshot::Data slotData{};
            if (!MaterialSlotSnapshot::TryParseSlotV1(cursor, slotData)) {
                break;
            }
            MaterialSlotSnapshot::ApplyToMaterial(*material, slotData, *target, world, ctx);
            continue;
        }
        break;
    }
}

}  // namespace Spark::GltfInstanceOverrides
