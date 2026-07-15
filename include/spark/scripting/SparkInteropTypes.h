#pragma once

/**
 * Blittable types mirroring Spark C++ math / scene enums (layout must match managed mirrors).
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SparkVector2 {
    float x;
    float y;
} SparkVector2;

typedef struct SparkVector3 {
    float x;
    float y;
    float z;
} SparkVector3;

typedef struct SparkVector4 {
    float x;
    float y;
    float z;
    float w;
} SparkVector4;

typedef struct SparkQuaternion {
    float x;
    float y;
    float z;
    float w;
} SparkQuaternion;

/** Column-major 4x4 — mirrors Spark::Matrix4. */
typedef struct SparkMatrix4 {
    float m[16];
} SparkMatrix4;

/** Mirrors SceneMeshSlot. */
typedef enum SparkSceneMeshSlot {
    SparkSceneMeshSlot_UnitCube = 0,
    SparkSceneMeshSlot_GroundPlane = 1,
    SparkSceneMeshSlot_Custom = 2,
} SparkSceneMeshSlot;

/** Mirrors SceneSkyMode. */
typedef enum SparkSceneSkyMode {
    SparkSceneSkyMode_None = 0,
    SparkSceneSkyMode_Box = 1,
    SparkSceneSkyMode_Dome = 2,
    SparkSceneSkyMode_Plane = 3,
} SparkSceneSkyMode;

/** Mirrors SceneSpriteSortMode. */
typedef enum SparkSpriteSortMode {
    SparkSpriteSortMode_SortOrderOnly = 0,
    SparkSpriteSortMode_SortOrderThenWorldY = 1,
} SparkSpriteSortMode;

/** Mirrors RigidbodyBodyType2D. */
typedef enum SparkRigidbodyBodyType2D {
    SparkRigidbodyBodyType2D_Kinematic = 0,
    SparkRigidbodyBodyType2D_Static = 1,
    SparkRigidbodyBodyType2D_Dynamic = 2,
} SparkRigidbodyBodyType2D;

/** Mirrors PhysicsWorld2DSettings. */
typedef struct SparkPhysicsWorld2DSettings {
    float gravityY;
    float maxFallSpeed;
    int resolveDynamicVsDynamic;
} SparkPhysicsWorld2DSettings;

/** Mirrors Spark::Camera2D (subset for scripting). */
typedef struct SparkCamera2D {
    SparkVector3 position;
    float rotationRad;
    float halfExtentY;
    float clipNearZ;
    float clipFarZ;
} SparkCamera2D;

/** Mirrors Spark::SpriteAnimationClip. */
typedef struct SparkSpriteAnimationClip {
    uint32_t firstFrame;
    uint32_t frameCount;
    float framesPerSecond;
    int loop;
} SparkSpriteAnimationClip;

/** Mirrors Spark::AnimLoopMode. */
typedef enum SparkAnimLoopMode {
    SparkAnimLoopMode_Loop = 0,
    SparkAnimLoopMode_Once = 1,
    SparkAnimLoopMode_Hold = 2,
} SparkAnimLoopMode;

/** Mirrors Spark::Sprite2DAnimLocomotionSource. */
typedef enum SparkSprite2DAnimLocomotionSource {
    SparkSprite2DAnimLocomotionSource_HorizontalAbsVelX = 0,
    SparkSprite2DAnimLocomotionSource_SpeedSq = 1,
} SparkSprite2DAnimLocomotionSource;

/** Result of <c>spark_world_register_platformer2d_demo_textures</c> (Kenney paths under assets/). */
typedef struct SparkPlatformer2DAssetsInfo {
    int usingKenneyTilesheet;
    int usingKenneyPlayerAtlas;
    int usingKenneyGem;
    uint32_t playerAtlasColumns;
} SparkPlatformer2DAssetsInfo;

typedef struct SparkGameObject SparkGameObject;

/** Mirrors <c>Spark::PhysicsQueryFilter2D</c>. */
typedef struct SparkPhysicsQueryFilter2D {
    uint16_t queryCategoryBits;
    uint16_t queryMaskBits;
    int hitSolids;
    int hitTriggers;
} SparkPhysicsQueryFilter2D;

/** Mirrors <c>Spark::PhysicsQueryHit2D</c>. */
typedef struct SparkPhysicsQueryHit2D {
    uint32_t staticColliderIndex;
    SparkGameObject* owner;
} SparkPhysicsQueryHit2D;

#ifdef __cplusplus
}
#endif
