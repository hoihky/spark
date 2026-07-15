#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"

namespace Spark {

class MaterialComponent;
class MeshComponent;
class TransformComponent;

/**
 * Single LitPbr sphere whose material is edited at runtime (texture toggles, metallic/roughness,
 * emissive presets). Directional + point + spot lights. F1 fly, ESC menu.
 */
class MaterialShowcase3DDemo {
public:
    void Load(GameWorld& w, IEngineContext& context);
    void Unload(GameWorld& w);
    void Simulate(const FrameTiming& timing, IEngineContext& context);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

private:
    void ApplyMaterialState();
    void HandleMaterialInput(IInput& in, float deltaSeconds);

    Array<GameObject*> roots{};
    FlyCamera camera{};

    SharedPtr<Mesh> sphereMesh{};
    SharedPtr<Mesh> groundMesh{};
    SharedPtr<Texture2D> baseColorTex{};
    SharedPtr<Texture2D> normalTex{};
    SharedPtr<Texture2D> emissiveTex{};

    GameObject* showcaseSphere = nullptr;
    MaterialComponent* showcaseMaterial = nullptr;
    MeshComponent* showcaseMesh = nullptr;
    TransformComponent* showcaseTransform = nullptr;

    bool useBaseMap = true;
    bool useNormalMap = false;
    bool useEmissiveMap = false;
    int emissivePreset = 0;
    int tintPreset = 0;
    float metallic = 0.22F;
    float roughness = 0.48F;
    float emissiveIntensity = 0.0F;
    float spinRadians = 0.0F;

    GameObject* helpHud = nullptr;
    TextOverlayComponent* helpText = nullptr;
};

}  // namespace Spark
