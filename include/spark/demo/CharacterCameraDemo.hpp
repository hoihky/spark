#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/animation/Character3DAnimFsmComponent.hpp"

namespace Spark {

enum class CharAvatarModel : std::uint8_t {
    Fox,
    CesiumMan,
};

class CharacterCameraDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);


    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    void ApplyCharacterSkyVisuals();


    void UpdateCharacterSkyTransform();


    void AddPointLight(
            Spark::GameWorld& w,
            Spark::Vector3 position,
            Spark::Vector3 color,
            float intensity,
            float range);

    void ApplyAvatarModel(CharAvatarModel model);

    [[nodiscard]] const Spark::SkinnedGltfAsset& CachedAvatarAsset(CharAvatarModel model) const noexcept;

    [[nodiscard]] bool IsAvatarAssetReady(CharAvatarModel model) const noexcept;

    Spark::Array<Spark::GameObject*> roots{};
    Spark::CharacterCameraRig rig{};
    Spark::SharedPtr<Spark::Mesh> groundAsset;
    Spark::SharedPtr<Spark::Mesh> unitCubeAsset;
    Spark::SharedPtr<Spark::Texture2D> groundDiffTex;
    Spark::SharedPtr<Spark::Mesh> skyBoxMesh;
    Spark::SharedPtr<Spark::Texture2D> skyEquirectTex;
    bool charSkyHasEquirect = false;
    Spark::TransformComponent* skyTransform = nullptr;
    Spark::MeshComponent* charSkyMesh = nullptr;
    Spark::SkyComponent* charSkyComp = nullptr;
    Spark::MaterialComponent* charSkyMat = nullptr;
    Spark::GameObject* characterRoot = nullptr;
    Spark::TransformComponent* characterRootTr = nullptr;
    Spark::GameObject* characterVisual = nullptr;
    Spark::TransformComponent* characterVisualTr = nullptr;
    Spark::AnimatorComponent* playerAnimator = nullptr;
    Spark::Character3DAnimFsmComponent* charAnimFsm = nullptr;
    Spark::SkinnedMeshComponent* characterSkinnedMesh = nullptr;
    Spark::MaterialComponent* characterMaterial = nullptr;
    CharAvatarModel activeAvatarModel = CharAvatarModel::Fox;
    Spark::SkinnedGltfAsset foxAssetCached{};
    Spark::SkinnedGltfAsset cesiumAssetCached{};
    bool foxAssetReady = false;
    bool cesiumAssetReady = false;
    bool useSkinnedAvatar = false;
    /** Local Y on @ref characterVisual so bind-pose feet sit on the parent origin (ground). */
    float characterVisualFootOffsetY = 0.0F;
    /** Extra Y rotation if the glTF forward axis does not match movement (+Z forward in world). */
    float humanModelYawOffset = 0.0F;
    /** Bind-pose upright correction (e.g. Z-up mesh) composed after yaw each frame. */
    Spark::Quaternion humanModelBindFix;
    Spark::Utf8String characterAvatarHudName{};
    Spark::GameObject* fpsHudObject = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    float fpsSmoothed = 0.0F;

};

}  // namespace Spark
