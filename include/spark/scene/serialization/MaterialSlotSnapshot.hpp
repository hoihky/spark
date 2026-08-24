#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/render/scene/SceneShadingModel.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"

#include "spark/scene/material/MaterialAsset.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class GameWorldAssetCache;
class MaterialComponent;
class MultiMaterialComponent;
class Texture2D;

/**
 * Canonical PBR slot payload for scene serialization.
 * Shared by <c>MaterialComponent</c> (v3) and <c>MultiMaterialComponent</c> (per-slot v1 records).
 */
class MaterialSlotSnapshot {
public:
    struct Data {
        Utf8String baseColorPath;
        Utf8String normalPath;
        Utf8String metallicRoughnessPath;
        Utf8String emissivePath;
        Vector3 tint{Vector3::One};
        float metallic = 0.0F;
        float roughness = 0.45F;
        float metallicFactor = 1.0F;
        float roughnessFactor = 1.0F;
        float occlusionStrength = 1.0F;
        Vector3 emissiveColor{};
        float emissiveIntensity = 0.0F;
        Vector3 emissiveFactor{Vector3::One};
        bool doubleSided = false;
        float opacity = 1.0F;
        float alphaCutoff = 0.0F;
        SceneShadingModel shadingModel = SceneShadingModel::LitPbr;
    };

    static void CaptureFromMaterial(
            const MaterialComponent& material,
            const GameObject& owner,
            const SceneCaptureContext& ctx,
            Data& out);
    static void CaptureFromSlot(
            const MultiMaterialComponent::Slot& slot,
            const SceneCaptureContext& ctx,
            const GameObject& owner,
            Data& out);
    static void CaptureFromAsset(
            const MaterialAsset& asset,
            Data& out,
            const char* relativeToDir = nullptr,
            const GameWorldAssetCache* textureKeys = nullptr);

    /** Appends one v1 slot record (quoted texture paths + scalar fields) to <c>out</c>. */
    static void AppendSlotV1(const Data& data, Utf8String& out);

    /** Parses one slot; advances <c>cursor</c> past the record on success. */
    [[nodiscard]] static bool TryParseSlotV1(const char*& cursor, Data& out);

    static void ApplyToMaterial(
            MaterialComponent& material,
            const Data& data,
            GameObject& owner,
            GameWorld& world,
            const SceneApplyContext& ctx);

    static void ApplyToSlot(
            MultiMaterialComponent::Slot& slot,
            const Data& data,
            GameObject& owner,
            GameWorld& world,
            const SceneApplyContext& ctx);
};

}  // namespace Spark
