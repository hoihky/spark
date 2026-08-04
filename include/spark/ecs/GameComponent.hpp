#pragma once

#include "spark/ecs/Signal.hpp"
#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class GameObject;
class IEngineContext;

enum class ComponentKind : std::uint32_t {
    Unknown = 0,
    Transform,
    Mesh,
    Collision,
    Material,
    PointLight,
    SkinnedMesh,
    Animator,
    TextOverlay,
    UiCanvas,
    Sky,
    ParticleEmitter,
    Terrain,
    Sprite,
    Tilemap,
    TilemapCollider2D,
    BoxCollider2D,
    Rigidbody2D,
    SpriteAnimator,
    CircleCollider2D,
    SpriteLighting2D,
    BlendMode,
    BoxCollider3D,
    SphereCollider3D,
    CapsuleCollider3D,
    Rigidbody3D,
    PhysicsMaterial3D,
    DistanceJoint3D,
    SceneSpatialPolicy,
    AiAgent,
    SoundCue,
    Sprite2DCharacterAnimFsm,
    Character3DAnimFsm,
    SpotLight,
    DirectionalLight,
    Camera,
    Camera2D,
    Camera2DRig,
    RenderLayer,
    SortingGroup,
    CharacterController3D,
    TriggerVolume3D,
    AudioListener,
    Billboard,
    AnimationEventReceiver,
    AttachmentSocket,
    CameraFollow3D,
    SpringArm3D,
    PolygonCollider2D,
    Health,
    Damageable,
    DecalProjector,
    PhysicsMaterial2D,
    MeshCollider3D,
    NavMeshAgent,
    PatrolPath,
    PerceptionSensor,
    AmbientZone,
    FogVolume,
    PostProcessVolume,
    HingeJoint3D,
    SpringJoint3D,
    DistanceJoint2D,
    HingeJoint2D,
    TilemapGameplayGrid,
    TilemapTileAnimator,
    TilemapAutotile,
    TilemapObjectLayer,
    TilemapObjectSpawn,
    TilemapObjectGizmo,
    TilemapMapSource,
    TimeOfDayDriver,
};

/** Typical <c>GameComponent::UpdatePriority</c> values (lower runs first). */
namespace ComponentUpdatePriority {
constexpr int AnimationDriver = 100;
constexpr int AnimatorPlayback = 200;
}  // namespace ComponentUpdatePriority

/**
 * Base of all components on a GameObject. Sibling components communicate via EmitSignal / OnSignal.
 * Simulation hooks: OnAttach, OnDetach, OnUpdate; rendering is usually driven by MeshComponent + Scene.
 */
class GameComponent {
public:
    GameComponent() = default;
    GameComponent(const GameComponent&) = delete;
    GameComponent& operator=(const GameComponent&) = delete;
    virtual ~GameComponent() = default;

    [[nodiscard]] virtual ComponentKind Kind() const noexcept = 0;

    virtual void OnAttach(GameObject& owner) { (void)owner; }
    virtual void OnDetach(GameObject& owner) { (void)owner; }
    virtual void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) {
        (void)timing;
        (void)owner;
        (void)context;
    }
    virtual void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) {
        (void)owner;
        (void)id;
        (void)payload;
    }

    /**
     * Lower values run earlier in <c>GameObject::UpdateComponents</c> (default 0).
     * Animation drivers (e.g. FSM) should run before <c>AnimatorComponent</c> playback.
     */
    [[nodiscard]] virtual int UpdatePriority() const noexcept { return 0; }

    [[nodiscard]] GameObject* GetOwner() const noexcept { return owner; }

private:
    friend class GameObject;

    void InternalSetOwner(GameObject* o) noexcept { owner = o; }

    GameObject* owner = nullptr;
};

}  // namespace Spark
