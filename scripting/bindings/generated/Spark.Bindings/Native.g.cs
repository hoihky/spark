using System.Runtime.InteropServices;

namespace Spark.Bindings
{
    public partial struct SparkVector2
    {
        public float x;

        public float y;
    }

    public partial struct SparkVector3
    {
        public float x;

        public float y;

        public float z;
    }

    public partial struct SparkVector4
    {
        public float x;

        public float y;

        public float z;

        public float w;
    }

    public partial struct SparkQuaternion
    {
        public float x;

        public float y;

        public float z;

        public float w;
    }

    public unsafe partial struct SparkMatrix4
    {
        [NativeTypeName("float[16]")]
        public fixed float m[16];
    }

    [NativeTypeName("unsigned int")]
    public enum SparkSceneMeshSlot : uint
    {
        SparkSceneMeshSlot_UnitCube = 0,
        SparkSceneMeshSlot_GroundPlane = 1,
        SparkSceneMeshSlot_Custom = 2,
    }

    [NativeTypeName("unsigned int")]
    public enum SparkSceneSkyMode : uint
    {
        SparkSceneSkyMode_None = 0,
        SparkSceneSkyMode_Box = 1,
        SparkSceneSkyMode_Dome = 2,
        SparkSceneSkyMode_Plane = 3,
    }

    [NativeTypeName("unsigned int")]
    public enum SparkSpriteSortMode : uint
    {
        SparkSpriteSortMode_SortOrderOnly = 0,
        SparkSpriteSortMode_SortOrderThenWorldY = 1,
    }

    [NativeTypeName("unsigned int")]
    public enum SparkRigidbodyBodyType2D : uint
    {
        SparkRigidbodyBodyType2D_Kinematic = 0,
        SparkRigidbodyBodyType2D_Static = 1,
        SparkRigidbodyBodyType2D_Dynamic = 2,
    }

    public partial struct SparkPhysicsWorld2DSettings
    {
        public float gravityY;

        public float maxFallSpeed;

        public int resolveDynamicVsDynamic;
    }

    public partial struct SparkCamera2D
    {
        public SparkVector3 position;

        public float rotationRad;

        public float halfExtentY;

        public float clipNearZ;

        public float clipFarZ;
    }

    public partial struct SparkSpriteAnimationClip
    {
        [NativeTypeName("uint32_t")]
        public uint firstFrame;

        [NativeTypeName("uint32_t")]
        public uint frameCount;

        public float framesPerSecond;

        public int loop;
    }

    [NativeTypeName("unsigned int")]
    public enum SparkAnimLoopMode : uint
    {
        SparkAnimLoopMode_Loop = 0,
        SparkAnimLoopMode_Once = 1,
        SparkAnimLoopMode_Hold = 2,
    }

    [NativeTypeName("unsigned int")]
    public enum SparkSprite2DAnimLocomotionSource : uint
    {
        SparkSprite2DAnimLocomotionSource_HorizontalAbsVelX = 0,
        SparkSprite2DAnimLocomotionSource_SpeedSq = 1,
    }

    public partial struct SparkPlatformer2DAssetsInfo
    {
        public int usingKenneyTilesheet;

        public int usingKenneyPlayerAtlas;

        public int usingKenneyGem;

        [NativeTypeName("uint32_t")]
        public uint playerAtlasColumns;
    }

    public partial struct SparkGameObject
    {
    }

    public partial struct SparkPhysicsQueryFilter2D
    {
        [NativeTypeName("uint16_t")]
        public ushort queryCategoryBits;

        [NativeTypeName("uint16_t")]
        public ushort queryMaskBits;

        public int hitSolids;

        public int hitTriggers;
    }

    public unsafe partial struct SparkPhysicsQueryHit2D
    {
        [NativeTypeName("uint32_t")]
        public uint staticColliderIndex;

        public SparkGameObject* owner;
    }

    public partial struct SparkEngineContext
    {
    }

    public partial struct SparkScene
    {
    }

    public partial struct SparkGameWorld
    {
    }

    public partial struct SparkGameComponent
    {
    }

    public partial struct SparkInput
    {
    }

    public partial struct SparkRenderFrame
    {
    }

    public partial struct SparkFrameTiming
    {
        public float deltaTimeSeconds;

        public float totalTimeSeconds;

        [NativeTypeName("uint64_t")]
        public ulong frameIndex;
    }

    [NativeTypeName("unsigned int")]
    public enum SparkComponentKind : uint
    {
        SparkComponentKind_Unknown = 0,
        SparkComponentKind_Transform,
        SparkComponentKind_Mesh,
        SparkComponentKind_Collision,
        SparkComponentKind_Material,
        SparkComponentKind_PointLight,
        SparkComponentKind_SkinnedMesh,
        SparkComponentKind_Animator,
        SparkComponentKind_TextOverlay,
        SparkComponentKind_GuiCanvas,
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
    }

    public static unsafe partial class Native
    {
        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_context_get_framebuffer_size([NativeTypeName("const SparkEngineContext *")] SparkEngineContext* context, int* outWidth, int* outHeight);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkInput* spark_context_get_input(SparkEngineContext* context);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkScene* spark_context_try_get_scene(SparkEngineContext* context);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_context_set_scene_render_params(SparkEngineContext* context, [NativeTypeName("const void *")] void* @params, [NativeTypeName("uint32_t")] uint paramsByteSize);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_context_process_gui_input(SparkEngineContext* context);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_input_is_key_down([NativeTypeName("const SparkInput *")] SparkInput* input, int keyCode);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_input_is_key_pressed_this_frame([NativeTypeName("const SparkInput *")] SparkInput* input, int keyCode);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern float spark_input_get_mouse_delta_x([NativeTypeName("const SparkInput *")] SparkInput* input);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern float spark_input_get_mouse_delta_y([NativeTypeName("const SparkInput *")] SparkInput* input);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_input_is_mouse_button_down([NativeTypeName("const SparkInput *")] SparkInput* input, int button);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_input_is_mouse_button_pressed_this_frame([NativeTypeName("const SparkInput *")] SparkInput* input, int button);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_input_is_mouse_button_released_this_frame([NativeTypeName("const SparkInput *")] SparkInput* input, int button);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern float spark_input_get_scroll_delta_y([NativeTypeName("const SparkInput *")] SparkInput* input);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_input_get_cursor_framebuffer_pixels([NativeTypeName("const SparkInput *")] SparkInput* input, float* outX, float* outY, int drawableWidth, int drawableHeight);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_input_set_cursor_captured(SparkInput* input, int capture);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_input_is_cursor_captured([NativeTypeName("const SparkInput *")] SparkInput* input);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameWorld* spark_scene_get_world(SparkScene* scene);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_world_update_game_objects(SparkGameWorld* world, [NativeTypeName("const SparkFrameTiming *")] SparkFrameTiming* timing, SparkEngineContext* context);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_world_process_sound_cues(SparkGameWorld* world, SparkEngineContext* context);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_world_simulate_game_ai(SparkGameWorld* world, [NativeTypeName("const SparkFrameTiming *")] SparkFrameTiming* timing, SparkEngineContext* context);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_world_physics_simulate_2d(SparkGameWorld* world, [NativeTypeName("const SparkFrameTiming *")] SparkFrameTiming* timing, [NativeTypeName("const SparkPhysicsWorld2DSettings *")] SparkPhysicsWorld2DSettings* settings);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_world_physics_simulate_3d(SparkGameWorld* world, [NativeTypeName("const SparkFrameTiming *")] SparkFrameTiming* timing);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameObject* spark_world_create_game_object(SparkGameWorld* world, [NativeTypeName("const char *")] sbyte* utf8Name);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_world_destroy_game_object(SparkGameWorld* world, SparkGameObject* @object);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_world_load_gltf(SparkGameWorld* world, [NativeTypeName("const char *")] sbyte* path);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_world_load_skinned_gltf(SparkGameWorld* world, [NativeTypeName("const char *")] sbyte* path);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_world_load_texture(SparkGameWorld* world, [NativeTypeName("const char *")] sbyte* path);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_world_register_checkerboard_texture(SparkGameWorld* world, [NativeTypeName("const char *")] sbyte* cacheKey, [NativeTypeName("uint32_t")] uint size, [NativeTypeName("uint32_t")] uint tilePixels, [NativeTypeName("const SparkVector3 *")] SparkVector3* colorA, [NativeTypeName("const SparkVector3 *")] SparkVector3* colorB);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_world_register_platformer2d_demo_textures(SparkGameWorld* world, SparkPlatformer2DAssetsInfo* outInfo);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_world_mount_platformer_ui_font(SparkGameWorld* world);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_platformer2d_kenney_tile_uv([NativeTypeName("uint32_t")] uint tileOneBased, SparkVector4* outUvRect);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_scene_fill_standard_lit_from_world(SparkGameWorld* world, SparkEngineContext* context, [NativeTypeName("const SparkMatrix4 *")] SparkMatrix4* viewProjection, [NativeTypeName("const SparkVector3 *")] SparkVector3* cameraPositionWorld, [NativeTypeName("const SparkVector3 *")] SparkVector3* lightDirectionWorld, [NativeTypeName("const SparkVector3 *")] SparkVector3* lightColor, float lightIntensity, [NativeTypeName("const SparkVector3 *")] SparkVector3* ambientColor, int enableParticles, [NativeTypeName("const SparkVector3 *")] SparkVector3* particleCameraRight, [NativeTypeName("const SparkVector3 *")] SparkVector3* particleCameraUp, float sceneTimeSeconds, void* outParams, [NativeTypeName("uint32_t")] uint outParamsByteSize, SparkSpriteSortMode spriteSortMode);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_scene_submit_standard_lit_from_world(SparkGameWorld* world, SparkEngineContext* context, [NativeTypeName("const SparkMatrix4 *")] SparkMatrix4* viewProjection, [NativeTypeName("const SparkVector3 *")] SparkVector3* cameraPositionWorld, [NativeTypeName("const SparkVector3 *")] SparkVector3* lightDirectionWorld, [NativeTypeName("const SparkVector3 *")] SparkVector3* lightColor, float lightIntensity, [NativeTypeName("const SparkVector3 *")] SparkVector3* ambientColor, int enableParticles, [NativeTypeName("const SparkVector3 *")] SparkVector3* particleCameraRight, [NativeTypeName("const SparkVector3 *")] SparkVector3* particleCameraUp, float sceneTimeSeconds, SparkSpriteSortMode spriteSortMode);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_gui_process_canvases_input(SparkGameWorld* world, SparkInput* input, int framebufferWidth, int framebufferHeight);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_gui_paint_canvases(SparkGameWorld* world, void* @params, [NativeTypeName("uint32_t")] uint paramsByteSize, int framebufferWidth, int framebufferHeight);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_gui_consumes_game_pointer();

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_mat4_perspective_vulkan(SparkMatrix4* @out, float verticalFovYRad, float aspect, float nearZ, float farZ);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_mat4_orthographic_vulkan(SparkMatrix4* @out, float left, float right, float bottom, float top, float nearZ, float farZ);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_mat4_mul(SparkMatrix4* @out, [NativeTypeName("const SparkMatrix4 *")] SparkMatrix4* a, [NativeTypeName("const SparkMatrix4 *")] SparkMatrix4* b);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_mat4_try_invert(SparkMatrix4* @out, [NativeTypeName("const SparkMatrix4 *")] SparkMatrix4* m);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_mat4_camera2d_view_projection(SparkMatrix4* @out, float orthoHalfHeight, float aspect, [NativeTypeName("const SparkVector3 *")] SparkVector3* cameraPositionWorld);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_camera2d_view_projection([NativeTypeName("const SparkCamera2D *")] SparkCamera2D* camera, float framebufferWidth, float framebufferHeight, SparkMatrix4* @out);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_camera2d_billboard_basis([NativeTypeName("const SparkCamera2D *")] SparkCamera2D* camera, SparkVector3* outRight, SparkVector3* outUp);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint64_t")]
        public static extern ulong spark_object_get_id([NativeTypeName("const SparkGameObject *")] SparkGameObject* @object);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_object_get_name([NativeTypeName("const SparkGameObject *")] SparkGameObject* @object, [NativeTypeName("char *")] sbyte* outUtf8, [NativeTypeName("uint32_t")] uint outCapacity);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_object_set_name(SparkGameObject* @object, [NativeTypeName("const char *")] sbyte* utf8Name);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_try_get_component_by_kind(SparkGameObject* @object, SparkComponentKind kind);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_get_or_add_transform(SparkGameObject* @object);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_mesh(SparkGameObject* @object, SparkSceneMeshSlot slot, [NativeTypeName("const SparkVector3 *")] SparkVector3* albedo);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_material_lit(SparkGameObject* @object, [NativeTypeName("const char *")] sbyte* textureKeyOrPath, [NativeTypeName("const SparkVector3 *")] SparkVector3* tint);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_material_emissive(SparkGameObject* @object, [NativeTypeName("const SparkVector3 *")] SparkVector3* emissiveRgb, float emissiveIntensity);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_sprite(SparkGameObject* @object, [NativeTypeName("const char *")] sbyte* textureKeyOrPath, [NativeTypeName("const SparkVector4 *")] SparkVector4* tint, [NativeTypeName("const SparkVector4 *")] SparkVector4* uvRect, int sortOrder);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_sprite_animator(SparkGameObject* @object);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_sprite_2d_character_anim_fsm(SparkGameObject* @object);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_character_3d_anim_fsm(SparkGameObject* @object);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_tilemap(SparkGameObject* @object, [NativeTypeName("const char *")] sbyte* atlasKeyOrPath, [NativeTypeName("uint32_t")] uint mapWidth, [NativeTypeName("uint32_t")] uint mapHeight, [NativeTypeName("uint32_t")] uint atlasTilesU, [NativeTypeName("uint32_t")] uint atlasTilesV, float tileWorldSize, int sortOrderBase);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_point_light(SparkGameObject* @object, [NativeTypeName("const SparkVector3 *")] SparkVector3* color, float intensity, float range);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_spot_light(SparkGameObject* @object, [NativeTypeName("const SparkVector3 *")] SparkVector3* color, float intensity, float range, float innerConeDegrees, float outerConeDegrees);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_sky(SparkGameObject* @object, SparkSceneSkyMode mode);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_text_overlay(SparkGameObject* @object);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_gui_canvas(SparkGameObject* @object, int sortOrder);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_particle_emitter(SparkGameObject* @object);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_box_collider_2d(SparkGameObject* @object, [NativeTypeName("const SparkVector2 *")] SparkVector2* halfExtents, [NativeTypeName("const SparkVector2 *")] SparkVector2* offset);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_circle_collider_2d(SparkGameObject* @object, float radius, [NativeTypeName("const SparkVector2 *")] SparkVector2* offset);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_rigidbody_2d(SparkGameObject* @object, SparkRigidbodyBodyType2D bodyType, float gravityScale);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_box_collider_3d(SparkGameObject* @object, [NativeTypeName("const SparkVector3 *")] SparkVector3* halfExtents, [NativeTypeName("const SparkVector3 *")] SparkVector3* offset);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_sphere_collider_3d(SparkGameObject* @object, float radius, [NativeTypeName("const SparkVector3 *")] SparkVector3* offset);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_rigidbody_3d(SparkGameObject* @object);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern SparkGameComponent* spark_object_add_collision_sphere(SparkGameObject* @object, float radius, [NativeTypeName("const SparkVector3 *")] SparkVector3* localCenter);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_object_add_skinned_character_from_gltf(SparkGameObject* @object, SparkGameWorld* world, [NativeTypeName("const char *")] sbyte* gltfPath, SparkGameComponent** outSkinnedMesh, SparkGameComponent** outAnimator, SparkGameComponent** outMaterial);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_transform_get_translation([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* transform, SparkVector3* @out);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_transform_set_translation(SparkGameComponent* transform, [NativeTypeName("const SparkVector3 *")] SparkVector3* v);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_transform_set_rotation(SparkGameComponent* transform, [NativeTypeName("const SparkQuaternion *")] SparkQuaternion* q);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_transform_set_scale(SparkGameComponent* transform, [NativeTypeName("const SparkVector3 *")] SparkVector3* s);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_transform_set_uniform_scale(SparkGameComponent* transform, float s);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_mesh_get_albedo([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* mesh, SparkVector3* @out);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_mesh_set_albedo(SparkGameComponent* mesh, [NativeTypeName("const SparkVector3 *")] SparkVector3* albedo);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_mesh_set_mesh_by_key(SparkGameComponent* mesh, SparkGameWorld* world, [NativeTypeName("const char *")] sbyte* meshKeyOrPath);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_material_set_tint(SparkGameComponent* material, [NativeTypeName("const SparkVector3 *")] SparkVector3* tint);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_material_set_metallic(SparkGameComponent* material, float metallic);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_material_set_roughness(SparkGameComponent* material, float roughness);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_material_set_emissive(SparkGameComponent* material, [NativeTypeName("const SparkVector3 *")] SparkVector3* rgb, float intensity);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_material_set_base_color_texture(SparkGameComponent* material, SparkGameWorld* world, [NativeTypeName("const char *")] sbyte* textureKeyOrPath);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_set_tint(SparkGameComponent* sprite, [NativeTypeName("const SparkVector4 *")] SparkVector4* tint);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_set_uv_rect(SparkGameComponent* sprite, [NativeTypeName("const SparkVector4 *")] SparkVector4* uvRect);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_set_sort_order(SparkGameComponent* sprite, int sortOrder);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_tilemap_set_tile(SparkGameComponent* tilemap, [NativeTypeName("uint32_t")] uint x, [NativeTypeName("uint32_t")] uint y, [NativeTypeName("uint16_t")] ushort tileId);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_point_light_set_color(SparkGameComponent* light, [NativeTypeName("const SparkVector3 *")] SparkVector3* color);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_point_light_set_intensity(SparkGameComponent* light, float intensity);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_point_light_set_range(SparkGameComponent* light, float range);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_point_light_set_enabled(SparkGameComponent* light, int enabled);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_spot_light_set_color(SparkGameComponent* light, [NativeTypeName("const SparkVector3 *")] SparkVector3* color);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_spot_light_set_intensity(SparkGameComponent* light, float intensity);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_spot_light_set_range(SparkGameComponent* light, float range);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_spot_light_set_cones(SparkGameComponent* light, float innerConeDegrees, float outerConeDegrees);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_spot_light_set_enabled(SparkGameComponent* light, int enabled);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sky_set_mode(SparkGameComponent* sky, SparkSceneSkyMode mode);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sky_set_tint(SparkGameComponent* sky, [NativeTypeName("const SparkVector3 *")] SparkVector3* tint);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sky_set_enabled(SparkGameComponent* sky, int enabled);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_text_overlay_set_text(SparkGameComponent* text, [NativeTypeName("const char *")] sbyte* utf8);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_text_overlay_set_screen_position(SparkGameComponent* text, float x, float y);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_text_overlay_set_font_size_pixels(SparkGameComponent* text, float px);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_text_overlay_set_color(SparkGameComponent* text, [NativeTypeName("const SparkVector3 *")] SparkVector3* rgb);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_text_overlay_set_alpha(SparkGameComponent* text, float alpha);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_text_overlay_set_visible(SparkGameComponent* text, int visible);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_gui_canvas_set_enabled(SparkGameComponent* canvas, int enabled);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_gui_canvas_set_sort_order(SparkGameComponent* canvas, int sortOrder);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_gui_canvas_clear_root(SparkGameComponent* canvas);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_rigidbody_2d_get_velocity([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* body, SparkVector2* @out);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_rigidbody_2d_set_velocity(SparkGameComponent* body, [NativeTypeName("const SparkVector2 *")] SparkVector2* v);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_rigidbody_2d_is_grounded([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* body);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_rigidbody_3d_get_velocity([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* body, SparkVector3* @out);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_rigidbody_3d_set_velocity(SparkGameComponent* body, [NativeTypeName("const SparkVector3 *")] SparkVector3* v);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_rigidbody_3d_get_angular_velocity([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* body, SparkVector3* @out);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_rigidbody_3d_set_angular_velocity(SparkGameComponent* body, [NativeTypeName("const SparkVector3 *")] SparkVector3* v);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_rigidbody_3d_set_linear_damping(SparkGameComponent* body, float v);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_rigidbody_3d_set_angular_damping(SparkGameComponent* body, float v);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_rigidbody_3d_set_restitution(SparkGameComponent* body, float v);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_rigidbody_3d_set_inverse_mass(SparkGameComponent* body, float inverseMass);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint16_t")]
        public static extern ushort spark_collision_filter_2d_layer_bit([NativeTypeName("uint32_t")] uint layerIndex);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint16_t")]
        public static extern ushort spark_collision_filter_2d_all_layers_mask();

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint16_t")]
        public static extern ushort spark_collision_filter_2d_default_category();

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint32_t")]
        public static extern uint spark_physics_query_overlap_circle_world_2d(SparkGameWorld* world, float centerX, float centerY, float radius, [NativeTypeName("const SparkPhysicsQueryFilter2D *")] SparkPhysicsQueryFilter2D* filter, SparkPhysicsQueryHit2D* outHits, [NativeTypeName("uint32_t")] uint maxHits, float cellWorldSize);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint32_t")]
        public static extern uint spark_physics_query_overlap_arc_world_statics_2d(SparkGameWorld* world, float originX, float originY, float radius, float dirX, float dirY, float halfAngleRadians, [NativeTypeName("const SparkPhysicsQueryFilter2D *")] SparkPhysicsQueryFilter2D* filter, SparkPhysicsQueryHit2D* outHits, [NativeTypeName("uint32_t")] uint maxHits, float cellWorldSize);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_box_collider_2d_set_is_trigger(SparkGameComponent* collider, int isTrigger);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_box_collider_2d_set_half_extents(SparkGameComponent* collider, [NativeTypeName("const SparkVector2 *")] SparkVector2* halfExtents);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_box_collider_2d_set_category_bits(SparkGameComponent* collider, [NativeTypeName("uint16_t")] ushort bits);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_box_collider_2d_set_mask_bits(SparkGameComponent* collider, [NativeTypeName("uint16_t")] ushort bits);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_circle_collider_2d_set_is_trigger(SparkGameComponent* collider, int isTrigger);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_circle_collider_2d_set_radius(SparkGameComponent* collider, float radius);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_circle_collider_2d_set_category_bits(SparkGameComponent* collider, [NativeTypeName("uint16_t")] ushort bits);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_circle_collider_2d_set_mask_bits(SparkGameComponent* collider, [NativeTypeName("uint16_t")] ushort bits);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint32_t")]
        public static extern uint spark_animator_get_clip_index([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* animator);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern float spark_animator_get_time_seconds([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* animator);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern float spark_animator_get_speed([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* animator);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_animator_set_clip_index(SparkGameComponent* animator, [NativeTypeName("uint32_t")] uint clipIndex);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_animator_set_speed(SparkGameComponent* animator, float speed);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_animator_set_time_seconds(SparkGameComponent* animator, float timeSeconds);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint32_t")]
        public static extern uint spark_animator_get_clip_count([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* animator);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint32_t")]
        public static extern uint spark_animator_get_loop_mode([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* animator);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_animator_set_loop_mode(SparkGameComponent* animator, [NativeTypeName("uint32_t")] uint loopMode);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_animator_is_clip_finished([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* animator);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_animator_set_clip_index_with_crossfade(SparkGameComponent* animator, [NativeTypeName("uint32_t")] uint clipIndex, float crossfadeDurationSec);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("int32_t")]
        public static extern int spark_animator_find_clip_index_by_name([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* animator, [NativeTypeName("const char *")] sbyte* name);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_animator_get_clip_name([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* animator, [NativeTypeName("uint32_t")] uint clipIndex, [NativeTypeName("char *")] sbyte* outUtf8, [NativeTypeName("uint32_t")] uint outCapacity);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_animator_set_uniform_grid(SparkGameComponent* animator, [NativeTypeName("uint32_t")] uint columns, [NativeTypeName("uint32_t")] uint rows);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_animator_clear_clips(SparkGameComponent* animator);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_animator_add_clip(SparkGameComponent* animator, [NativeTypeName("const SparkSpriteAnimationClip *")] SparkSpriteAnimationClip* clip);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_animator_set_clip_index(SparkGameComponent* animator, [NativeTypeName("uint32_t")] uint clipIndex);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint32_t")]
        public static extern uint spark_sprite_animator_get_clip_index([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* animator);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint32_t")]
        public static extern uint spark_sprite_animator_get_clip_count([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* animator);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern int spark_sprite_animator_is_current_clip_finished([NativeTypeName("const SparkGameComponent *")] SparkGameComponent* animator);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_animator_compute_uniform_grid_uv([NativeTypeName("uint32_t")] uint columns, [NativeTypeName("uint32_t")] uint rows, [NativeTypeName("uint32_t")] uint linearFrame, SparkVector4* outUvRect);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_2d_fsm_set_locomotion_clips(SparkGameComponent* fsm, [NativeTypeName("uint32_t")] uint idleClipIndex, [NativeTypeName("uint32_t")] uint moveClipIndex);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_2d_fsm_set_combat_clips(SparkGameComponent* fsm, [NativeTypeName("uint32_t")] uint attackClipIndex, [NativeTypeName("uint32_t")] uint hurtClipIndex);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_2d_fsm_set_move_speed_threshold(SparkGameComponent* fsm, float worldUnits);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_2d_fsm_set_locomotion_source(SparkGameComponent* fsm, SparkSprite2DAnimLocomotionSource source);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_2d_fsm_request_hurt(SparkGameComponent* fsm);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_sprite_2d_fsm_request_attack(SparkGameComponent* fsm);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_char_3d_fsm_set_locomotion_clips(SparkGameComponent* fsm, [NativeTypeName("uint32_t")] uint idleClipIndex, [NativeTypeName("uint32_t")] uint walkClipIndex, [NativeTypeName("uint32_t")] uint runClipIndex);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_char_3d_fsm_configure_locomotion_from_skeleton(SparkGameComponent* fsm, [NativeTypeName("const SparkGameComponent *")] SparkGameComponent* animator, [NativeTypeName("uint32_t")] uint walkClipFallback);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_char_3d_fsm_set_attack_clip(SparkGameComponent* fsm, [NativeTypeName("uint32_t")] uint attackClipIndex);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_char_3d_fsm_set_walk_speed_threshold(SparkGameComponent* fsm, float metersPerSecond);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_char_3d_fsm_set_run_speed_threshold(SparkGameComponent* fsm, float metersPerSecond);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_char_3d_fsm_set_crossfade_duration(SparkGameComponent* fsm, float seconds);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_char_3d_fsm_set_locomotion_driving_enabled(SparkGameComponent* fsm, int enabled);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_char_3d_fsm_set_locomotion_input(SparkGameComponent* fsm, int moving, int sprint);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_char_3d_fsm_set_manual_clip(SparkGameComponent* fsm, [NativeTypeName("uint32_t")] uint clipIndex, [NativeTypeName("uint32_t")] uint loopMode);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_char_3d_fsm_clear_manual_clip(SparkGameComponent* fsm);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_char_3d_fsm_request_attack(SparkGameComponent* fsm);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_particle_emitter_set_enabled(SparkGameComponent* emitter, int enabled);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_particle_emitter_set_rate(SparkGameComponent* emitter, float particlesPerSecond);

        [DllImport("SparkInterop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void spark_particle_emitter_set_max_particles(SparkGameComponent* emitter, int maxParticles);
    }
}
