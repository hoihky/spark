#pragma once

/** Umbrella include for the entity–component–signal layer. */

#include "spark/ecs/GameComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"

// Core
#include "spark/ecs/components/core/TransformComponent.hpp"

// Rendering
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/components/rendering/RenderLayerComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/components/rendering/SortingGroupComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/rendering/SpriteLighting2DComponent.hpp"
#include "spark/ecs/components/rendering/TerrainComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"

// Lighting
#include "spark/ecs/components/lighting/DirectionalLightComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/lighting/SpotLightComponent.hpp"

// Camera
#include "spark/ecs/components/camera/Camera2DComponent.hpp"
#include "spark/ecs/components/camera/Camera2DRigComponent.hpp"
#include "spark/ecs/components/camera/CameraComponent.hpp"

// Physics
#include "spark/ecs/components/physics/CollisionComponent.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/DistanceJoint3DComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsMaterial3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"

// Animation
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/animation/Character3DAnimFsmComponent.hpp"
#include "spark/ecs/components/animation/Sprite2DCharacterAnimFsmComponent.hpp"
#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"

// AI / audio / UI / world
#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ecs/components/audio/SoundCueComponent.hpp"
#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/ecs/components/world/SceneSpatialPolicyComponent.hpp"
