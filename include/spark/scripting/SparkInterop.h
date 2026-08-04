#pragma once

/**
 * Stable C ABI between native Spark and managed (CoreCLR) code.
 * ClangSharp parses this header to emit one-to-one C# types.
 * Regenerate bindings after changes: tools/generate-csharp-bindings.sh
 */

#include "spark/scripting/SparkInteropTypes.h"

#include <stdint.h>

#if defined(_WIN32)
#if defined(SPARK_INTEROP_EXPORTS)
#define SPARK_SCRIPT_API __declspec(dllexport)
#else
#define SPARK_SCRIPT_API __declspec(dllimport)
#endif
#else
#define SPARK_SCRIPT_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* --- Opaque handles --- */
typedef struct SparkEngineContext SparkEngineContext;
typedef struct SparkScene SparkScene;
typedef struct SparkGameWorld SparkGameWorld;
typedef struct SparkGameObject SparkGameObject;
typedef struct SparkGameComponent SparkGameComponent;
typedef struct SparkInput SparkInput;
typedef struct SparkRenderFrame SparkRenderFrame;

typedef struct SparkFrameTiming {
    float deltaTimeSeconds;
    float totalTimeSeconds;
    uint64_t frameIndex;
} SparkFrameTiming;

typedef enum SparkComponentKind {
    SparkComponentKind_Unknown = 0,
    SparkComponentKind_Transform,
    SparkComponentKind_Mesh,
    SparkComponentKind_Collision,
    SparkComponentKind_Material,
    SparkComponentKind_PointLight,
    SparkComponentKind_SkinnedMesh,
    SparkComponentKind_Animator,
    SparkComponentKind_TextOverlay,
    SparkComponentKind_UiCanvas,
    SparkComponentKind_Sky,
    SparkComponentKind_ParticleEmitter,
    SparkComponentKind_Terrain,
    SparkComponentKind_Sprite,
    SparkComponentKind_Tilemap,
    SparkComponentKind_BoxCollider2D,
    SparkComponentKind_Rigidbody2D,
    SparkComponentKind_SpriteAnimator,
    SparkComponentKind_CircleCollider2D,
    SparkComponentKind_SpriteLighting2D,
    SparkComponentKind_BoxCollider3D,
    SparkComponentKind_SphereCollider3D,
    SparkComponentKind_Rigidbody3D,
    SparkComponentKind_PhysicsMaterial3D,
    SparkComponentKind_DistanceJoint3D,
    SparkComponentKind_SceneSpatialPolicy,
    SparkComponentKind_AiAgent,
    SparkComponentKind_SoundCue,
    SparkComponentKind_Sprite2DCharacterAnimFsm,
    SparkComponentKind_Character3DAnimFsm,
    SparkComponentKind_SpotLight,
    SparkComponentKind_DirectionalLight,
    SparkComponentKind_Camera,
    SparkComponentKind_Camera2D,
    SparkComponentKind_Camera2DRig,
    SparkComponentKind_BlendMode,
    SparkComponentKind_RenderLayer,
    SparkComponentKind_SortingGroup,
} SparkComponentKind;

typedef void (*SparkGameOnAttachFn)(void* userData, SparkEngineContext* context);
typedef void (*SparkGameOnDetachFn)(void* userData);
typedef void (*SparkGameOnUpdateFn)(void* userData, const SparkFrameTiming* timing, SparkEngineContext* context);
typedef void (*SparkGameOnRenderFn)(void* userData, SparkRenderFrame* frame, SparkEngineContext* context);

typedef struct SparkManagedGameCallbacks {
    void* userData;
    SparkGameOnAttachFn onAttach;
    SparkGameOnDetachFn onDetach;
    SparkGameOnUpdateFn onUpdate;
    SparkGameOnRenderFn onRender;
} SparkManagedGameCallbacks;

typedef struct SparkHostApi {
    uint32_t structSize;
    int (*registerManagedGame)(const SparkManagedGameCallbacks* callbacks);
} SparkHostApi;

typedef int (*SparkScriptEntryInitializeFn)(const SparkHostApi* hostApi);

SPARK_SCRIPT_API int spark_script_host_run(int argc, const char* const* argv);

/* --- Engine / context / input --- */
SPARK_SCRIPT_API void spark_context_get_framebuffer_size(
        const SparkEngineContext* context,
        int* outWidth,
        int* outHeight);
SPARK_SCRIPT_API SparkInput* spark_context_get_input(SparkEngineContext* context);
SPARK_SCRIPT_API SparkScene* spark_context_try_get_scene(SparkEngineContext* context);
SPARK_SCRIPT_API void spark_context_set_scene_render_params(
        SparkEngineContext* context,
        const void* params,
        uint32_t paramsByteSize);
SPARK_SCRIPT_API void spark_context_process_ui_input(SparkEngineContext* context);

SPARK_SCRIPT_API int spark_input_is_key_down(const SparkInput* input, int keyCode);
SPARK_SCRIPT_API int spark_input_is_key_pressed_this_frame(const SparkInput* input, int keyCode);
SPARK_SCRIPT_API float spark_input_get_mouse_delta_x(const SparkInput* input);
SPARK_SCRIPT_API float spark_input_get_mouse_delta_y(const SparkInput* input);
SPARK_SCRIPT_API int spark_input_is_mouse_button_down(const SparkInput* input, int button);
SPARK_SCRIPT_API int spark_input_is_mouse_button_pressed_this_frame(const SparkInput* input, int button);
SPARK_SCRIPT_API int spark_input_is_mouse_button_released_this_frame(const SparkInput* input, int button);
SPARK_SCRIPT_API float spark_input_get_scroll_delta_y(const SparkInput* input);
SPARK_SCRIPT_API void spark_input_get_cursor_framebuffer_pixels(
        const SparkInput* input,
        float* outX,
        float* outY,
        int drawableWidth,
        int drawableHeight);
SPARK_SCRIPT_API void spark_input_set_cursor_captured(SparkInput* input, int capture);
SPARK_SCRIPT_API int spark_input_is_cursor_captured(const SparkInput* input);

/* --- Scene / world lifecycle --- */
SPARK_SCRIPT_API SparkGameWorld* spark_scene_get_world(SparkScene* scene);
SPARK_SCRIPT_API void spark_world_update_game_objects(
        SparkGameWorld* world,
        const SparkFrameTiming* timing,
        SparkEngineContext* context);
SPARK_SCRIPT_API void spark_world_process_sound_cues(SparkGameWorld* world, SparkEngineContext* context);
SPARK_SCRIPT_API void spark_world_simulate_game_ai(
        SparkGameWorld* world,
        const SparkFrameTiming* timing,
        SparkEngineContext* context);
SPARK_SCRIPT_API void spark_world_physics_simulate_2d(
        SparkGameWorld* world,
        const SparkFrameTiming* timing,
        const SparkPhysicsWorld2DSettings* settings);
SPARK_SCRIPT_API void spark_world_physics_simulate_3d(SparkGameWorld* world, const SparkFrameTiming* timing);

SPARK_SCRIPT_API SparkGameObject* spark_world_create_game_object(SparkGameWorld* world, const char* utf8Name);
SPARK_SCRIPT_API void spark_world_destroy_game_object(SparkGameWorld* world, SparkGameObject* object);
SPARK_SCRIPT_API int spark_world_load_gltf(SparkGameWorld* world, const char* path);
SPARK_SCRIPT_API int spark_world_load_skinned_gltf(SparkGameWorld* world, const char* path);
SPARK_SCRIPT_API int spark_world_load_texture(SparkGameWorld* world, const char* path);
SPARK_SCRIPT_API int spark_world_register_checkerboard_texture(
        SparkGameWorld* world,
        const char* cacheKey,
        uint32_t size,
        uint32_t tilePixels,
        const SparkVector3* colorA,
        const SparkVector3* colorB);

/** Registers Kenney platformer tilesheet + player atlas (procedural fallbacks when PNGs are missing). */
SPARK_SCRIPT_API int spark_world_register_platformer2d_demo_textures(
        SparkGameWorld* world,
        SparkPlatformer2DAssetsInfo* outInfo);
/** Loads Roboto UI fonts from build tree / assets/fonts (required for TextOverlay HUD). */
SPARK_SCRIPT_API int spark_world_mount_platformer_ui_font(SparkGameWorld* world);
/** UV rect for Kenney simplified platformer tilesheet tile number (1-based pack index). */
SPARK_SCRIPT_API void spark_platformer2d_kenney_tile_uv(uint32_t tileOneBased, SparkVector4* outUvRect);

/* --- Scene submit (mirrors SceneSubmit.hpp) --- */
SPARK_SCRIPT_API void spark_scene_fill_standard_lit_from_world(
        SparkGameWorld* world,
        SparkEngineContext* context,
        const SparkMatrix4* viewProjection,
        const SparkVector3* cameraPositionWorld,
        const SparkVector3* lightDirectionWorld,
        const SparkVector3* lightColor,
        float lightIntensity,
        const SparkVector3* ambientColor,
        int enableParticles,
        const SparkVector3* particleCameraRight,
        const SparkVector3* particleCameraUp,
        float sceneTimeSeconds,
        void* outParams,
        uint32_t outParamsByteSize,
        SparkSpriteSortMode spriteSortMode);
SPARK_SCRIPT_API void spark_scene_submit_standard_lit_from_world(
        SparkGameWorld* world,
        SparkEngineContext* context,
        const SparkMatrix4* viewProjection,
        const SparkVector3* cameraPositionWorld,
        const SparkVector3* lightDirectionWorld,
        const SparkVector3* lightColor,
        float lightIntensity,
        const SparkVector3* ambientColor,
        int enableParticles,
        const SparkVector3* particleCameraRight,
        const SparkVector3* particleCameraUp,
        float sceneTimeSeconds,
        SparkSpriteSortMode spriteSortMode);

/* --- UI (mirrors UiScene.hpp) --- */
SPARK_SCRIPT_API void spark_ui_process_canvases_input(
        SparkGameWorld* world,
        SparkInput* input,
        int framebufferWidth,
        int framebufferHeight);
SPARK_SCRIPT_API void spark_ui_paint_canvases(
        SparkGameWorld* world,
        void* params,
        uint32_t paramsByteSize,
        int framebufferWidth,
        int framebufferHeight);
SPARK_SCRIPT_API int spark_ui_consumes_game_pointer(void);

/* --- Math (mirrors Matrix4 helpers) --- */
SPARK_SCRIPT_API void spark_mat4_perspective_vulkan(
        SparkMatrix4* out,
        float verticalFovYRad,
        float aspect,
        float nearZ,
        float farZ);
SPARK_SCRIPT_API void spark_mat4_orthographic_vulkan(
        SparkMatrix4* out,
        float left,
        float right,
        float bottom,
        float top,
        float nearZ,
        float farZ);
SPARK_SCRIPT_API void spark_mat4_mul(SparkMatrix4* out, const SparkMatrix4* a, const SparkMatrix4* b);
SPARK_SCRIPT_API int spark_mat4_try_invert(SparkMatrix4* out, const SparkMatrix4* m);
SPARK_SCRIPT_API void spark_mat4_camera2d_view_projection(
        SparkMatrix4* out,
        float orthoHalfHeight,
        float aspect,
        const SparkVector3* cameraPositionWorld);
SPARK_SCRIPT_API void spark_camera2d_view_projection(
        const SparkCamera2D* camera,
        float framebufferWidth,
        float framebufferHeight,
        SparkMatrix4* out);
SPARK_SCRIPT_API void spark_camera2d_billboard_basis(
        const SparkCamera2D* camera,
        SparkVector3* outRight,
        SparkVector3* outUp);

/* --- GameObject --- */
SPARK_SCRIPT_API uint64_t spark_object_get_id(const SparkGameObject* object);
SPARK_SCRIPT_API int spark_object_get_name(const SparkGameObject* object, char* outUtf8, uint32_t outCapacity);
SPARK_SCRIPT_API void spark_object_set_name(SparkGameObject* object, const char* utf8Name);
SPARK_SCRIPT_API SparkGameComponent* spark_object_try_get_component_by_kind(
        SparkGameObject* object,
        SparkComponentKind kind);
SPARK_SCRIPT_API SparkGameComponent* spark_object_get_or_add_transform(SparkGameObject* object);

/* --- Add components (mirrors GameObject::AddComponent) --- */
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_mesh(
        SparkGameObject* object,
        SparkSceneMeshSlot slot,
        const SparkVector3* albedo);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_material_lit(
        SparkGameObject* object,
        const char* textureKeyOrPath,
        const SparkVector3* tint);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_material_emissive(
        SparkGameObject* object,
        const SparkVector3* emissiveRgb,
        float emissiveIntensity);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_sprite(
        SparkGameObject* object,
        const char* textureKeyOrPath,
        const SparkVector4* tint,
        const SparkVector4* uvRect,
        int sortOrder);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_sprite_animator(SparkGameObject* object);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_sprite_2d_character_anim_fsm(SparkGameObject* object);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_character_3d_anim_fsm(SparkGameObject* object);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_tilemap(
        SparkGameObject* object,
        const char* atlasKeyOrPath,
        uint32_t mapWidth,
        uint32_t mapHeight,
        uint32_t atlasTilesU,
        uint32_t atlasTilesV,
        float tileWorldSize,
        int sortOrderBase);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_point_light(
        SparkGameObject* object,
        const SparkVector3* color,
        float intensity,
        float range);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_spot_light(
        SparkGameObject* object,
        const SparkVector3* color,
        float intensity,
        float range,
        float innerConeDegrees,
        float outerConeDegrees);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_sky(SparkGameObject* object, SparkSceneSkyMode mode);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_text_overlay(SparkGameObject* object);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_ui_canvas(SparkGameObject* object, int sortOrder);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_particle_emitter(SparkGameObject* object);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_box_collider_2d(
        SparkGameObject* object,
        const SparkVector2* halfExtents,
        const SparkVector2* offset);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_circle_collider_2d(
        SparkGameObject* object,
        float radius,
        const SparkVector2* offset);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_rigidbody_2d(
        SparkGameObject* object,
        SparkRigidbodyBodyType2D bodyType,
        float gravityScale);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_box_collider_3d(
        SparkGameObject* object,
        const SparkVector3* halfExtents,
        const SparkVector3* offset);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_sphere_collider_3d(
        SparkGameObject* object,
        float radius,
        const SparkVector3* offset);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_rigidbody_3d(SparkGameObject* object);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_collision_sphere(
        SparkGameObject* object,
        float radius,
        const SparkVector3* localCenter);
SPARK_SCRIPT_API int spark_object_add_skinned_character_from_gltf(
        SparkGameObject* object,
        SparkGameWorld* world,
        const char* gltfPath,
        SparkGameComponent** outSkinnedMesh,
        SparkGameComponent** outAnimator,
        SparkGameComponent** outMaterial);

/* --- Transform --- */
SPARK_SCRIPT_API void spark_transform_get_translation(
        const SparkGameComponent* transform,
        SparkVector3* out);
SPARK_SCRIPT_API void spark_transform_set_translation(SparkGameComponent* transform, const SparkVector3* v);
SPARK_SCRIPT_API void spark_transform_set_rotation(SparkGameComponent* transform, const SparkQuaternion* q);
SPARK_SCRIPT_API void spark_transform_set_scale(SparkGameComponent* transform, const SparkVector3* s);
SPARK_SCRIPT_API void spark_transform_set_uniform_scale(SparkGameComponent* transform, float s);

/* --- Mesh --- */
SPARK_SCRIPT_API void spark_mesh_get_albedo(const SparkGameComponent* mesh, SparkVector3* out);
SPARK_SCRIPT_API void spark_mesh_set_albedo(SparkGameComponent* mesh, const SparkVector3* albedo);
SPARK_SCRIPT_API int spark_mesh_set_mesh_by_key(
        SparkGameComponent* mesh,
        SparkGameWorld* world,
        const char* meshKeyOrPath);

/* --- Material --- */
SPARK_SCRIPT_API void spark_material_set_tint(SparkGameComponent* material, const SparkVector3* tint);
SPARK_SCRIPT_API void spark_material_set_metallic(SparkGameComponent* material, float metallic);
SPARK_SCRIPT_API void spark_material_set_roughness(SparkGameComponent* material, float roughness);
SPARK_SCRIPT_API void spark_material_set_emissive(
        SparkGameComponent* material,
        const SparkVector3* rgb,
        float intensity);
SPARK_SCRIPT_API int spark_material_set_base_color_texture(
        SparkGameComponent* material,
        SparkGameWorld* world,
        const char* textureKeyOrPath);

/* --- Sprite --- */
SPARK_SCRIPT_API void spark_sprite_set_tint(SparkGameComponent* sprite, const SparkVector4* tint);
SPARK_SCRIPT_API void spark_sprite_set_uv_rect(SparkGameComponent* sprite, const SparkVector4* uvRect);
SPARK_SCRIPT_API void spark_sprite_set_sort_order(SparkGameComponent* sprite, int sortOrder);

/* --- Render layers / sorting groups --- */
SPARK_SCRIPT_API int spark_render_layer_register(const char* name, int sortingOrder);
SPARK_SCRIPT_API int spark_render_layer_find(const char* name);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_render_layer(
        SparkGameObject* object,
        const char* layerName,
        int orderInLayer);
SPARK_SCRIPT_API void spark_render_layer_set_order_in_layer(SparkGameComponent* layer, int orderInLayer);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_sorting_group(SparkGameObject* object, int sortingOrder);
SPARK_SCRIPT_API void spark_sorting_group_set_enabled(SparkGameComponent* group, int enabled);
SPARK_SCRIPT_API void spark_sorting_group_set_sorting_order(SparkGameComponent* group, int sortingOrder);
SPARK_SCRIPT_API void spark_sorting_group_set_sort_at_root_world_y(SparkGameComponent* group, int enabled);

/* --- Camera2D / rigs --- */
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_camera_2d(
        SparkGameObject* object,
        float halfExtentY,
        int priority);
SPARK_SCRIPT_API void spark_camera_2d_set_half_extent_y(SparkGameComponent* camera, float halfExtentY);
SPARK_SCRIPT_API void spark_camera_2d_set_clip_planes(SparkGameComponent* camera, float nearZ, float farZ);
SPARK_SCRIPT_API void spark_camera_2d_set_priority(SparkGameComponent* camera, int priority);
SPARK_SCRIPT_API void spark_camera_2d_set_enabled(SparkGameComponent* camera, int enabled);
SPARK_SCRIPT_API SparkGameComponent* spark_object_add_camera_2d_rig(SparkGameObject* object);
SPARK_SCRIPT_API void spark_camera_2d_rig_set_mode(SparkGameComponent* rig, int mode);
SPARK_SCRIPT_API void spark_camera_2d_rig_set_target(SparkGameComponent* rig, SparkGameObject* target);
SPARK_SCRIPT_API void spark_camera_2d_rig_set_target_offset(
        SparkGameComponent* rig,
        const SparkVector3* offset);
SPARK_SCRIPT_API void spark_camera_2d_rig_set_follow_smooth_rate(SparkGameComponent* rig, float rate);
SPARK_SCRIPT_API void spark_camera_2d_rig_set_look_ahead_scale(SparkGameComponent* rig, float scale);
SPARK_SCRIPT_API void spark_camera_2d_rig_set_bounds(
        SparkGameComponent* rig,
        int useBounds,
        const SparkVector2* boundsMin,
        const SparkVector2* boundsMax);
SPARK_SCRIPT_API void spark_camera_2d_rig_tick(
        SparkGameComponent* rig,
        SparkGameObject* owner,
        float deltaSeconds,
        float framebufferAspect);

/* --- Tilemap --- */
SPARK_SCRIPT_API void spark_tilemap_set_tile(
        SparkGameComponent* tilemap,
        uint32_t x,
        uint32_t y,
        uint16_t tileId);

/* --- Lights --- */
SPARK_SCRIPT_API void spark_point_light_set_color(SparkGameComponent* light, const SparkVector3* color);
SPARK_SCRIPT_API void spark_point_light_set_intensity(SparkGameComponent* light, float intensity);
SPARK_SCRIPT_API void spark_point_light_set_range(SparkGameComponent* light, float range);
SPARK_SCRIPT_API void spark_point_light_set_enabled(SparkGameComponent* light, int enabled);
SPARK_SCRIPT_API void spark_spot_light_set_color(SparkGameComponent* light, const SparkVector3* color);
SPARK_SCRIPT_API void spark_spot_light_set_intensity(SparkGameComponent* light, float intensity);
SPARK_SCRIPT_API void spark_spot_light_set_range(SparkGameComponent* light, float range);
SPARK_SCRIPT_API void spark_spot_light_set_cones(
        SparkGameComponent* light,
        float innerConeDegrees,
        float outerConeDegrees);
SPARK_SCRIPT_API void spark_spot_light_set_enabled(SparkGameComponent* light, int enabled);

/* --- Sky --- */
SPARK_SCRIPT_API void spark_sky_set_mode(SparkGameComponent* sky, SparkSceneSkyMode mode);
SPARK_SCRIPT_API void spark_sky_set_tint(SparkGameComponent* sky, const SparkVector3* tint);
SPARK_SCRIPT_API void spark_sky_set_enabled(SparkGameComponent* sky, int enabled);

/* --- Text overlay --- */
SPARK_SCRIPT_API void spark_text_overlay_set_text(SparkGameComponent* text, const char* utf8);
SPARK_SCRIPT_API void spark_text_overlay_set_screen_position(SparkGameComponent* text, float x, float y);
SPARK_SCRIPT_API void spark_text_overlay_set_font_size_pixels(SparkGameComponent* text, float px);
SPARK_SCRIPT_API void spark_text_overlay_set_color(SparkGameComponent* text, const SparkVector3* rgb);
SPARK_SCRIPT_API void spark_text_overlay_set_alpha(SparkGameComponent* text, float alpha);
SPARK_SCRIPT_API void spark_text_overlay_set_visible(SparkGameComponent* text, int visible);

/* --- UI canvas --- */
SPARK_SCRIPT_API void spark_ui_canvas_set_enabled(SparkGameComponent* canvas, int enabled);
SPARK_SCRIPT_API void spark_ui_canvas_set_sort_order(SparkGameComponent* canvas, int sortOrder);
SPARK_SCRIPT_API void spark_ui_canvas_clear_root(SparkGameComponent* canvas);

/* --- Rigidbody 2D / 3D --- */
SPARK_SCRIPT_API void spark_rigidbody_2d_get_velocity(const SparkGameComponent* body, SparkVector2* out);
SPARK_SCRIPT_API void spark_rigidbody_2d_set_velocity(SparkGameComponent* body, const SparkVector2* v);
SPARK_SCRIPT_API int spark_rigidbody_2d_is_grounded(const SparkGameComponent* body);
SPARK_SCRIPT_API void spark_rigidbody_3d_get_velocity(const SparkGameComponent* body, SparkVector3* out);
SPARK_SCRIPT_API void spark_rigidbody_3d_set_velocity(SparkGameComponent* body, const SparkVector3* v);
SPARK_SCRIPT_API void spark_rigidbody_3d_get_angular_velocity(const SparkGameComponent* body, SparkVector3* out);
SPARK_SCRIPT_API void spark_rigidbody_3d_set_angular_velocity(SparkGameComponent* body, const SparkVector3* v);
SPARK_SCRIPT_API void spark_rigidbody_3d_set_linear_damping(SparkGameComponent* body, float v);
SPARK_SCRIPT_API void spark_rigidbody_3d_set_angular_damping(SparkGameComponent* body, float v);
SPARK_SCRIPT_API void spark_rigidbody_3d_set_restitution(SparkGameComponent* body, float v);
SPARK_SCRIPT_API void spark_rigidbody_3d_set_inverse_mass(SparkGameComponent* body, float inverseMass);

/* --- Collision filter helpers (mirrors CollisionFilter2D) --- */
SPARK_SCRIPT_API uint16_t spark_collision_filter_2d_layer_bit(uint32_t layerIndex);
SPARK_SCRIPT_API uint16_t spark_collision_filter_2d_all_layers_mask(void);
SPARK_SCRIPT_API uint16_t spark_collision_filter_2d_default_category(void);

/* --- 2D physics queries (mirrors PhysicsQueries2D.hpp) --- */
SPARK_SCRIPT_API uint32_t spark_physics_query_overlap_circle_world_2d(
        SparkGameWorld* world,
        float centerX,
        float centerY,
        float radius,
        const SparkPhysicsQueryFilter2D* filter,
        SparkPhysicsQueryHit2D* outHits,
        uint32_t maxHits,
        float cellWorldSize);
SPARK_SCRIPT_API uint32_t spark_physics_query_overlap_arc_world_statics_2d(
        SparkGameWorld* world,
        float originX,
        float originY,
        float radius,
        float dirX,
        float dirY,
        float halfAngleRadians,
        const SparkPhysicsQueryFilter2D* filter,
        SparkPhysicsQueryHit2D* outHits,
        uint32_t maxHits,
        float cellWorldSize);

/* --- Box collider 2D --- */
SPARK_SCRIPT_API void spark_box_collider_2d_set_is_trigger(SparkGameComponent* collider, int isTrigger);
SPARK_SCRIPT_API void spark_box_collider_2d_set_half_extents(SparkGameComponent* collider, const SparkVector2* halfExtents);
SPARK_SCRIPT_API void spark_box_collider_2d_set_category_bits(SparkGameComponent* collider, uint16_t bits);
SPARK_SCRIPT_API void spark_box_collider_2d_set_mask_bits(SparkGameComponent* collider, uint16_t bits);

/* --- Circle collider 2D --- */
SPARK_SCRIPT_API void spark_circle_collider_2d_set_is_trigger(SparkGameComponent* collider, int isTrigger);
SPARK_SCRIPT_API void spark_circle_collider_2d_set_radius(SparkGameComponent* collider, float radius);
SPARK_SCRIPT_API void spark_circle_collider_2d_set_category_bits(SparkGameComponent* collider, uint16_t bits);
SPARK_SCRIPT_API void spark_circle_collider_2d_set_mask_bits(SparkGameComponent* collider, uint16_t bits);

/* --- Animator (mirrors AnimatorComponent) --- */
SPARK_SCRIPT_API uint32_t spark_animator_get_clip_index(const SparkGameComponent* animator);
SPARK_SCRIPT_API float spark_animator_get_time_seconds(const SparkGameComponent* animator);
SPARK_SCRIPT_API float spark_animator_get_speed(const SparkGameComponent* animator);
SPARK_SCRIPT_API void spark_animator_set_clip_index(SparkGameComponent* animator, uint32_t clipIndex);
SPARK_SCRIPT_API void spark_animator_set_speed(SparkGameComponent* animator, float speed);
SPARK_SCRIPT_API void spark_animator_set_time_seconds(SparkGameComponent* animator, float timeSeconds);
SPARK_SCRIPT_API uint32_t spark_animator_get_clip_count(const SparkGameComponent* animator);
SPARK_SCRIPT_API uint32_t spark_animator_get_loop_mode(const SparkGameComponent* animator);
SPARK_SCRIPT_API void spark_animator_set_loop_mode(SparkGameComponent* animator, uint32_t loopMode);
SPARK_SCRIPT_API int spark_animator_is_clip_finished(const SparkGameComponent* animator);
SPARK_SCRIPT_API void spark_animator_set_clip_index_with_crossfade(
        SparkGameComponent* animator,
        uint32_t clipIndex,
        float crossfadeDurationSec);
SPARK_SCRIPT_API int32_t spark_animator_find_clip_index_by_name(const SparkGameComponent* animator, const char* name);
SPARK_SCRIPT_API int spark_animator_get_clip_name(
        const SparkGameComponent* animator,
        uint32_t clipIndex,
        char* outUtf8,
        uint32_t outCapacity);

/* --- Sprite animator (2D flipbook) --- */
SPARK_SCRIPT_API void spark_sprite_animator_set_uniform_grid(
        SparkGameComponent* animator,
        uint32_t columns,
        uint32_t rows);
SPARK_SCRIPT_API void spark_sprite_animator_clear_clips(SparkGameComponent* animator);
SPARK_SCRIPT_API void spark_sprite_animator_add_clip(
        SparkGameComponent* animator,
        const SparkSpriteAnimationClip* clip);
SPARK_SCRIPT_API void spark_sprite_animator_set_clip_index(SparkGameComponent* animator, uint32_t clipIndex);
SPARK_SCRIPT_API uint32_t spark_sprite_animator_get_clip_index(const SparkGameComponent* animator);
SPARK_SCRIPT_API uint32_t spark_sprite_animator_get_clip_count(const SparkGameComponent* animator);
SPARK_SCRIPT_API int spark_sprite_animator_is_current_clip_finished(const SparkGameComponent* animator);
SPARK_SCRIPT_API void spark_sprite_animator_compute_uniform_grid_uv(
        uint32_t columns,
        uint32_t rows,
        uint32_t linearFrame,
        SparkVector4* outUvRect);

/* --- Sprite2D character animation FSM --- */
SPARK_SCRIPT_API void spark_sprite_2d_fsm_set_locomotion_clips(
        SparkGameComponent* fsm,
        uint32_t idleClipIndex,
        uint32_t moveClipIndex);
SPARK_SCRIPT_API void spark_sprite_2d_fsm_set_combat_clips(
        SparkGameComponent* fsm,
        uint32_t attackClipIndex,
        uint32_t hurtClipIndex);
SPARK_SCRIPT_API void spark_sprite_2d_fsm_set_move_speed_threshold(SparkGameComponent* fsm, float worldUnits);
SPARK_SCRIPT_API void spark_sprite_2d_fsm_set_locomotion_source(
        SparkGameComponent* fsm,
        SparkSprite2DAnimLocomotionSource source);
SPARK_SCRIPT_API void spark_sprite_2d_fsm_request_hurt(SparkGameComponent* fsm);
SPARK_SCRIPT_API void spark_sprite_2d_fsm_request_attack(SparkGameComponent* fsm);

/* --- 3D character animation FSM --- */
SPARK_SCRIPT_API void spark_char_3d_fsm_set_locomotion_clips(
        SparkGameComponent* fsm,
        uint32_t idleClipIndex,
        uint32_t walkClipIndex,
        uint32_t runClipIndex);
SPARK_SCRIPT_API void spark_char_3d_fsm_configure_locomotion_from_skeleton(
        SparkGameComponent* fsm,
        const SparkGameComponent* animator,
        uint32_t walkClipFallback);
SPARK_SCRIPT_API void spark_char_3d_fsm_set_attack_clip(SparkGameComponent* fsm, uint32_t attackClipIndex);
SPARK_SCRIPT_API void spark_char_3d_fsm_set_walk_speed_threshold(SparkGameComponent* fsm, float metersPerSecond);
SPARK_SCRIPT_API void spark_char_3d_fsm_set_run_speed_threshold(SparkGameComponent* fsm, float metersPerSecond);
SPARK_SCRIPT_API void spark_char_3d_fsm_set_crossfade_duration(SparkGameComponent* fsm, float seconds);
SPARK_SCRIPT_API void spark_char_3d_fsm_set_locomotion_driving_enabled(SparkGameComponent* fsm, int enabled);
SPARK_SCRIPT_API void spark_char_3d_fsm_set_locomotion_input(SparkGameComponent* fsm, int moving, int sprint);
SPARK_SCRIPT_API void spark_char_3d_fsm_set_manual_clip(
        SparkGameComponent* fsm,
        uint32_t clipIndex,
        uint32_t loopMode);
SPARK_SCRIPT_API void spark_char_3d_fsm_clear_manual_clip(SparkGameComponent* fsm);
SPARK_SCRIPT_API void spark_char_3d_fsm_request_attack(SparkGameComponent* fsm);

/* --- Particle emitter (subset) --- */
SPARK_SCRIPT_API void spark_particle_emitter_set_enabled(SparkGameComponent* emitter, int enabled);
SPARK_SCRIPT_API void spark_particle_emitter_set_rate(SparkGameComponent* emitter, float particlesPerSecond);
SPARK_SCRIPT_API void spark_particle_emitter_set_max_particles(SparkGameComponent* emitter, int maxParticles);

#ifdef __cplusplus
}
#endif
