cmake_minimum_required(VERSION 3.28)
project(Spark LANGUAGES CXX)

# AppleClang (Xcode) and many CMake versions do not enable C++26 yet; use 23 until they catch up.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies ---
include(FetchContent)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Single-header stb_image (no git clone; avoids bad SHAs / shallow-fetch issues).
set(SPARK_STB_DIR "${CMAKE_BINARY_DIR}/_deps/spark_stb")
set(SPARK_STB_IMAGE_H "${SPARK_STB_DIR}/stb_image.h")
if (NOT EXISTS "${SPARK_STB_IMAGE_H}")
    file(MAKE_DIRECTORY "${SPARK_STB_DIR}")
    message(STATUS "Downloading stb_image.h")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
            "${SPARK_STB_IMAGE_H}"
            TLS_VERIFY ON
    )
endif ()

set(SPARK_STB_IMAGE_WRITE_H "${SPARK_STB_DIR}/stb_image_write.h")
if (NOT EXISTS "${SPARK_STB_IMAGE_WRITE_H}")
    file(MAKE_DIRECTORY "${SPARK_STB_DIR}")
    message(STATUS "Downloading stb_image_write.h")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h"
            "${SPARK_STB_IMAGE_WRITE_H}"
            TLS_VERIFY ON
    )
endif ()

set(SPARK_STB_TRUETYPE_H "${SPARK_STB_DIR}/stb_truetype.h")
if (NOT EXISTS "${SPARK_STB_TRUETYPE_H}")
    message(STATUS "Downloading stb_truetype.h")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h"
            "${SPARK_STB_TRUETYPE_H}"
            TLS_VERIFY ON
    )
endif ()

# Single-file decoders for Ogg Vorbis + MP3 (Spark::TryLoadSoundClipFromFile).
set(SPARK_AUDIO_CODEC_DIR "${CMAKE_BINARY_DIR}/_deps/spark_audio_codecs")
# Upstream ships Vorbis as a single .c header-library (the old .h URL 404s).
set(SPARK_STB_VORBIS_C "${SPARK_AUDIO_CODEC_DIR}/stb_vorbis.c")
if (NOT EXISTS "${SPARK_STB_VORBIS_C}")
    file(MAKE_DIRECTORY "${SPARK_AUDIO_CODEC_DIR}")
    message(STATUS "Downloading stb_vorbis.c")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/nothings/stb/master/stb_vorbis.c"
            "${SPARK_STB_VORBIS_C}"
            TLS_VERIFY ON
    )
endif ()
set(SPARK_MINIMP3_H "${SPARK_AUDIO_CODEC_DIR}/minimp3.h")
if (NOT EXISTS "${SPARK_MINIMP3_H}")
    file(MAKE_DIRECTORY "${SPARK_AUDIO_CODEC_DIR}")
    message(STATUS "Downloading minimp3.h")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/lieff/minimp3/master/minimp3.h"
            "${SPARK_MINIMP3_H}"
            TLS_VERIFY ON
    )
endif ()
set(SPARK_MINIMP3_EX_H "${SPARK_AUDIO_CODEC_DIR}/minimp3_ex.h")
if (NOT EXISTS "${SPARK_MINIMP3_EX_H}")
    file(MAKE_DIRECTORY "${SPARK_AUDIO_CODEC_DIR}")
    message(STATUS "Downloading minimp3_ex.h")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/lieff/minimp3/master/minimp3_ex.h"
            "${SPARK_MINIMP3_EX_H}"
            TLS_VERIFY ON
    )
endif ()

# UI font: must be a real TTF (many KB). Failed downloads often leave HTML or empty files.
set(SPARK_UI_FONT_TTF "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/Roboto-Regular.ttf")
set(SPARK_UI_FONT_MIN_BYTES 10000)

function (spark_try_download_roboto out_path)
    set(_urls
            "https://raw.githubusercontent.com/google/fonts/main/apache/roboto/static/Roboto-Regular.ttf"
            "https://cdn.jsdelivr.net/gh/google/fonts@main/apache/roboto/static/Roboto-Regular.ttf"
    )
    foreach (_url IN LISTS _urls)
        message(STATUS "Fetching Roboto-Regular.ttf …")
        file(DOWNLOAD "${_url}" "${out_path}"
                TLS_VERIFY ON
                STATUS _st
        )
        list(GET _st 0 _rc)
        if (_rc EQUAL 0 AND EXISTS "${out_path}")
            file(SIZE "${out_path}" _sz)
            if (_sz GREATER 10000)
                message(STATUS "Roboto-Regular.ttf OK (${_sz} bytes)")
                return()
            endif ()
            message(WARNING "Downloaded file too small (${_sz} bytes), removing and trying next mirror …")
            file(REMOVE "${out_path}")
        else ()
            list(LENGTH _st _slen)
            if (_slen GREATER 1)
                list(GET _st 1 _msg)
                message(WARNING "Download failed (code ${_rc}): ${_msg}")
            else ()
                message(WARNING "Download failed (code ${_rc})")
            endif ()
        endif ()
    endforeach ()
endfunction ()

set(SPARK_UI_BOLD_FONT_TTF "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/Roboto-Bold.ttf")

function (spark_try_download_roboto_bold out_path)
    set(_urls
            "https://raw.githubusercontent.com/google/fonts/main/apache/roboto/static/Roboto-Bold.ttf"
            "https://cdn.jsdelivr.net/gh/google/fonts@main/apache/roboto/static/Roboto-Bold.ttf"
    )
    foreach (_url IN LISTS _urls)
        message(STATUS "Fetching Roboto-Bold.ttf …")
        file(DOWNLOAD "${_url}" "${out_path}"
                TLS_VERIFY ON
                STATUS _st
        )
        list(GET _st 0 _rc)
        if (_rc EQUAL 0 AND EXISTS "${out_path}")
            file(SIZE "${out_path}" _sz)
            if (_sz GREATER 10000)
                message(STATUS "Roboto-Bold.ttf OK (${_sz} bytes)")
                return()
            endif ()
            message(WARNING "Downloaded Roboto-Bold too small (${_sz} bytes), removing …")
            file(REMOVE "${out_path}")
        endif ()
    endforeach ()
endfunction ()

set(_spark_need_bold_font TRUE)
if (EXISTS "${SPARK_UI_BOLD_FONT_TTF}")
    file(SIZE "${SPARK_UI_BOLD_FONT_TTF}" _spark_bold_sz)
    if (_spark_bold_sz GREATER SPARK_UI_FONT_MIN_BYTES)
        set(_spark_need_bold_font FALSE)
    else ()
        message(WARNING "Existing ${SPARK_UI_BOLD_FONT_TTF} is too small (${_spark_bold_sz} bytes); re-fetching …")
        file(REMOVE "${SPARK_UI_BOLD_FONT_TTF}")
    endif ()
endif ()
if (_spark_need_bold_font)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts")
    spark_try_download_roboto_bold("${SPARK_UI_BOLD_FONT_TTF}")
endif ()
if (NOT EXISTS "${SPARK_UI_BOLD_FONT_TTF}")
    message(STATUS "Spark: Roboto-Bold.ttf not present — UI bold will use faux stroke until the file is added or CMake can download it.")
endif ()

set(_spark_need_font TRUE)
if (EXISTS "${SPARK_UI_FONT_TTF}")
    file(SIZE "${SPARK_UI_FONT_TTF}" _spark_font_sz)
    if (_spark_font_sz GREATER SPARK_UI_FONT_MIN_BYTES)
        set(_spark_need_font FALSE)
    else ()
        message(WARNING "Existing ${SPARK_UI_FONT_TTF} is too small (${_spark_font_sz} bytes); re-fetching …")
        file(REMOVE "${SPARK_UI_FONT_TTF}")
    endif ()
endif ()
if (_spark_need_font)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts")
    spark_try_download_roboto("${SPARK_UI_FONT_TTF}")
endif ()
if (NOT EXISTS "${SPARK_UI_FONT_TTF}")
    message(WARNING "Spark: Roboto-Regular.ttf is missing — UI overlay text will be disabled. "
            "Add a valid TTF to assets/fonts/ or fix network/TLS and re-run CMake.")
endif ()

# Mirror assets into the build tree so SparkDemo finds fonts when the working directory is the build folder.
set(SPARK_BUILD_ASSETS_DIR "${CMAKE_CURRENT_BINARY_DIR}/spark_runtime_assets")
file(MAKE_DIRECTORY "${SPARK_BUILD_ASSETS_DIR}/fonts")
file(MAKE_DIRECTORY "${SPARK_BUILD_ASSETS_DIR}/scene_editor")
file(MAKE_DIRECTORY "${SPARK_BUILD_ASSETS_DIR}/screenshots")
file(MAKE_DIRECTORY "${SPARK_BUILD_ASSETS_DIR}/recordings")
if (EXISTS "${SPARK_UI_FONT_TTF}")
    file(COPY "${SPARK_UI_FONT_TTF}" DESTINATION "${SPARK_BUILD_ASSETS_DIR}/fonts")
endif ()
if (EXISTS "${SPARK_UI_BOLD_FONT_TTF}")
    file(COPY "${SPARK_UI_BOLD_FONT_TTF}" DESTINATION "${SPARK_BUILD_ASSETS_DIR}/fonts")
endif ()
# Prefer the copied file for the baked-in path when it exists (same bytes as source).
set(SPARK_UI_FONT_PATH "${SPARK_BUILD_ASSETS_DIR}/fonts/Roboto-Regular.ttf")
if (NOT EXISTS "${SPARK_UI_FONT_PATH}")
    set(SPARK_UI_FONT_PATH "${SPARK_UI_FONT_TTF}")
endif ()
set(SPARK_UI_BOLD_FONT_PATH "${SPARK_BUILD_ASSETS_DIR}/fonts/Roboto-Bold.ttf")
if (NOT EXISTS "${SPARK_UI_BOLD_FONT_PATH}")
    set(SPARK_UI_BOLD_FONT_PATH "${SPARK_UI_BOLD_FONT_TTF}")
endif ()

# Equirectangular sky HDRI for Sky demo (Poly Haven CC0; stb loads .hdr as LDR; mirrored into build assets).
set(SPARK_SKY_TEX_NAME "equirect_sky_1k.hdr")
set(SPARK_SKY_TEX_SRC "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/sky/${SPARK_SKY_TEX_NAME}")
if (NOT EXISTS "${SPARK_SKY_TEX_SRC}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/sky")
    message(STATUS "Downloading Poly Haven 1k equirect HDR for sky demo")
    file(DOWNLOAD
            "https://dl.polyhaven.org/file/ph-assets/HDRIs/hdr/1k/kloofendal_43d_clear_1k.hdr"
            "${SPARK_SKY_TEX_SRC}"
            TLS_VERIFY ON
    )
endif ()
file(MAKE_DIRECTORY "${SPARK_BUILD_ASSETS_DIR}/textures/sky")
if (EXISTS "${SPARK_SKY_TEX_SRC}")
    file(COPY "${SPARK_SKY_TEX_SRC}" DESTINATION "${SPARK_BUILD_ASSETS_DIR}/textures/sky")
endif ()
set(SPARK_SKY_TEXTURE_PATH "${SPARK_BUILD_ASSETS_DIR}/textures/sky/${SPARK_SKY_TEX_NAME}")
if (NOT EXISTS "${SPARK_SKY_TEXTURE_PATH}")
    set(SPARK_SKY_TEXTURE_PATH "${SPARK_SKY_TEX_SRC}")
endif ()

set(SPARK_CGLTF_DIR "${CMAKE_BINARY_DIR}/_deps/spark_cgltf")
set(SPARK_CGLTF_H "${SPARK_CGLTF_DIR}/cgltf.h")
if (NOT EXISTS "${SPARK_CGLTF_H}")
    file(MAKE_DIRECTORY "${SPARK_CGLTF_DIR}")
    message(STATUS "Downloading cgltf.h")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/jkuhlmann/cgltf/master/cgltf.h"
            "${SPARK_CGLTF_H}"
            TLS_VERIFY ON
    )
endif ()

# Khronos "Damaged Helmet" (glTF-Sample-Assets), glTF-Binary — widely used PBR test model.
set(SPARK_DAMAGED_HELMET_GLB "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/DamagedHelmet.glb")
if (NOT EXISTS "${SPARK_DAMAGED_HELMET_GLB}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/models")
    message(STATUS "Downloading Khronos glTF-Sample-Assets DamagedHelmet.glb")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Assets/main/Models/DamagedHelmet/glTF-Binary/DamagedHelmet.glb"
            "${SPARK_DAMAGED_HELMET_GLB}"
            TLS_VERIFY ON
    )
endif ()

# Khronos "Sheen Chair" (KHR_materials_sheen sample; we render base color + lit shading only).
set(SPARK_SHEEN_CHAIR_GLB "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/SheenChair.glb")
if (NOT EXISTS "${SPARK_SHEEN_CHAIR_GLB}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/models")
    message(STATUS "Downloading Khronos glTF-Sample-Assets SheenChair.glb")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Assets/main/Models/SheenChair/glTF-Binary/SheenChair.glb"
            "${SPARK_SHEEN_CHAIR_GLB}"
            TLS_VERIFY ON
    )
endif ()

# Khronos Fox — skinned mesh with Walk / Run / Survey animations (ThreeDDemo uses Walk).
set(SPARK_FOX_GLB "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/Fox.glb")
if (NOT EXISTS "${SPARK_FOX_GLB}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/models")
    message(STATUS "Downloading Khronos glTF-Sample-Models Fox.glb")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/main/2.0/Fox/glTF-Binary/Fox.glb"
            "${SPARK_FOX_GLB}"
            TLS_VERIFY ON
    )
endif ()

# Khronos CesiumMan — skinned walking robot (CharacterCamera demo; glTF-Sample-Assets).
set(SPARK_CESIUM_MAN_GLB "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/CesiumMan.glb")
if (NOT EXISTS "${SPARK_CESIUM_MAN_GLB}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/models")
    message(STATUS "Downloading Khronos glTF-Sample-Assets CesiumMan.glb")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Assets/main/Models/CesiumMan/glTF-Binary/CesiumMan.glb"
            "${SPARK_CESIUM_MAN_GLB}"
            TLS_VERIFY ON
    )
endif ()

find_package(Vulkan REQUIRED)

find_program(GLSLANG_VALIDATOR glslangValidator
        HINTS "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/macOS/bin"
        DOC "Khronos glslangValidator (SPIR-V compiler)")

if (NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found. Install the Vulkan SDK and ensure VULKAN_SDK is set.")
endif ()

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SPV_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SPV_DIR})

set(SHADER_SOURCES
        ${SHADER_DIR}/scene.vert
        ${SHADER_DIR}/scene.frag
        ${SHADER_DIR}/shadow_depth.vert
        ${SHADER_DIR}/particle.vert
        ${SHADER_DIR}/particle.frag
        ${SHADER_DIR}/sprite.vert
        ${SHADER_DIR}/sprite.frag
        ${SHADER_DIR}/text.vert
        ${SHADER_DIR}/text.frag
        ${SHADER_DIR}/ui_solid.vert
        ${SHADER_DIR}/ui_solid.frag
        ${SHADER_DIR}/ui_sprite.vert
        ${SHADER_DIR}/ui_sprite.frag
        ${SHADER_DIR}/tonemap.vert
        ${SHADER_DIR}/tonemap.frag
        ${SHADER_DIR}/post_process.frag
)
set(SPV_FILES "")
foreach (SHADER ${SHADER_SOURCES})
    get_filename_component(NAME_WE ${SHADER} NAME_WE)
    get_filename_component(EXT ${SHADER} EXT)
    string(SUBSTRING ${EXT} 1 -1 EXT_NO_DOT)
    set(SPV ${SPV_DIR}/${NAME_WE}.${EXT_NO_DOT}.spv)
    add_custom_command(
            OUTPUT ${SPV}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SPV_DIR}
            COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.0 -I${SHADER_DIR} ${SHADER} -o ${SPV}
            DEPENDS ${SHADER} ${SHADER_DIR}/clustered_lights.glsl ${SHADER_DIR}/ibl.glsl ${SHADER_DIR}/punctual_shadows.glsl ${SHADER_DIR}/scene_ubo.glsl ${SHADER_DIR}/post_common.glsl
            COMMENT "SPIR-V: ${NAME_WE}.${EXT_NO_DOT}"
            VERBATIM
    )
    list(APPEND SPV_FILES ${SPV})
endforeach ()

add_custom_target(SparkShaders DEPENDS ${SPV_FILES})

set(SPARK_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
file(MAKE_DIRECTORY "${SPARK_ASSETS_DIR}/audio")
file(MAKE_DIRECTORY "${SPARK_BUILD_ASSETS_DIR}/audio")
set(SPARK_MAZE_WAV_SRC "${SPARK_ASSETS_DIR}/audio/maze.wav")
if (EXISTS "${SPARK_MAZE_WAV_SRC}")
    file(COPY "${SPARK_MAZE_WAV_SRC}" DESTINATION "${SPARK_BUILD_ASSETS_DIR}/audio")
endif ()
file(MAKE_DIRECTORY "${SPARK_ASSETS_DIR}/textures/terrain")
file(MAKE_DIRECTORY "${SPARK_BUILD_ASSETS_DIR}/textures/terrain")
# Terrain albedo: prefer committed terrain/*.png; seed from Kenney retro tiles when absent.
set(SPARK_TERRAIN_SOIL_SRC "${SPARK_ASSETS_DIR}/textures/terrain/soil_albedo.png")
if (NOT EXISTS "${SPARK_TERRAIN_SOIL_SRC}")
    set(_spark_terrain_soil_seed "${SPARK_ASSETS_DIR}/textures/floor_ground_dirt.png")
    if (EXISTS "${_spark_terrain_soil_seed}")
        file(COPY "${_spark_terrain_soil_seed}" DESTINATION "${SPARK_ASSETS_DIR}/textures/terrain")
        file(RENAME "${SPARK_ASSETS_DIR}/textures/terrain/floor_ground_dirt.png" "${SPARK_TERRAIN_SOIL_SRC}")
    endif ()
endif ()
set(SPARK_TERRAIN_ALBEDO_SRC "${SPARK_ASSETS_DIR}/textures/terrain/ground_albedo.png")
if (NOT EXISTS "${SPARK_TERRAIN_ALBEDO_SRC}")
    set(_spark_terrain_ground_seed "${SPARK_ASSETS_DIR}/textures/floor_ground_grass_overlay.png")
    if (EXISTS "${_spark_terrain_ground_seed}")
        file(COPY "${_spark_terrain_ground_seed}" DESTINATION "${SPARK_ASSETS_DIR}/textures/terrain")
        file(RENAME "${SPARK_ASSETS_DIR}/textures/terrain/floor_ground_grass_overlay.png" "${SPARK_TERRAIN_ALBEDO_SRC}")
    endif ()
endif ()
if (EXISTS "${SPARK_TERRAIN_SOIL_SRC}")
    file(COPY "${SPARK_TERRAIN_SOIL_SRC}" DESTINATION "${SPARK_BUILD_ASSETS_DIR}/textures/terrain")
endif ()
if (EXISTS "${SPARK_TERRAIN_ALBEDO_SRC}")
    file(COPY "${SPARK_TERRAIN_ALBEDO_SRC}" DESTINATION "${SPARK_BUILD_ASSETS_DIR}/textures/terrain")
endif ()
# Kenney / demo textures and sprite packs — copied beside the binary for runtime path fallbacks.
if (EXISTS "${SPARK_ASSETS_DIR}/textures")
    file(COPY "${SPARK_ASSETS_DIR}/textures" DESTINATION "${SPARK_BUILD_ASSETS_DIR}")
endif ()
foreach (_spark_sprite_pack IN ITEMS kenney_simplified-platformer-pack kenney_tiny-dungeon kenney)
    set(_spark_sprite_src "${SPARK_ASSETS_DIR}/sprites/${_spark_sprite_pack}")
    if (EXISTS "${_spark_sprite_src}")
        file(COPY "${_spark_sprite_src}" DESTINATION "${SPARK_BUILD_ASSETS_DIR}/sprites")
    endif ()
endforeach ()

# Kenney CC0 — Simplified Platformer character (idle + walk frames), merged horizontally in Platformer2D demo.
set(SPARK_KENNEY_PLATFORMER_DIR "${SPARK_ASSETS_DIR}/sprites/kenney")
file(MAKE_DIRECTORY "${SPARK_KENNEY_PLATFORMER_DIR}")
set(_kenney_chr_base "https://raw.githubusercontent.com/kefik/kenney/master/SimplifiedPlatformer/Characters")
foreach (_kenney_pf IN ITEMS platformChar_idle.png platformChar_walk1.png platformChar_walk2.png)
    set(_kenney_dst "${SPARK_KENNEY_PLATFORMER_DIR}/${_kenney_pf}")
    if (NOT EXISTS "${_kenney_dst}")
        message(STATUS "Downloading Kenney platformer character ${_kenney_pf} …")
        file(DOWNLOAD "${_kenney_chr_base}/${_kenney_pf}" "${_kenney_dst}" TLS_VERIFY ON STATUS _ks)
        list(GET _ks 0 _krc)
        if (NOT _krc EQUAL 0)
            message(WARNING "Kenney download failed for ${_kenney_pf} — Platformer2D will use procedural placeholder.")
        endif ()
    endif ()
endforeach ()
if (EXISTS "${SPARK_KENNEY_PLATFORMER_DIR}/platformChar_idle.png")
    file(COPY "${SPARK_KENNEY_PLATFORMER_DIR}" DESTINATION "${SPARK_BUILD_ASSETS_DIR}/sprites")
endif ()

option(SPARK_ENABLE_IMGUI "Integrate Dear ImGui (docking branch) for immediate-mode tool UI" ON)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/spark_config.hpp.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/spark/config.hpp
        @ONLY
)

# If the link step fails with undefined AnimatorComponent / SkinnedMeshComponent / Skeleton /
# TryLoadSkinnedCharacterFromGltf, the build tree is stale: re-run CMake (CLion: Reload CMake Project).
# Shell demos ship with SparkDemo; slim editor/game builds omit these (SPARK_BUILD_ENGINE_DEMOS=OFF).
set(SPARK_ENGINE_DEMO_SOURCES
        src/spark/demo/DemoAssetLoad.cpp
        src/spark/demo/DemoFoundation.cpp
        src/spark/demo/SparkShellDemo.cpp
        src/spark/demo/ShellDemoSceneUtil.cpp
        src/spark/demo/ShellDemoUi.cpp
        src/spark/demo/ThreeDDemo.cpp
        src/spark/demo/ToonShadingDemo.cpp
        src/spark/demo/MaterialShowcase3DDemo.cpp
        src/spark/demo/TimeOfDayDemo.cpp
        src/spark/demo/SkyDemo.cpp
        src/spark/demo/TerrainDemo.cpp
        src/spark/demo/ParticleDemo.cpp
        src/spark/demo/CharacterCameraDemo.cpp
        src/spark/demo/BroadPhase2DDemo.cpp
        src/spark/demo/Platformer2DDemo.cpp
        src/spark/demo/platformer2d/Platformer2DBulletPool.cpp
        src/spark/demo/platformer2d/Platformer2DExplosionFx.cpp
        src/spark/demo/platformer2d/Platformer2DHealthHud.cpp
        src/spark/demo/platformer2d/Platformer2DEnemySquad.cpp
        src/spark/demo/platformer2d/Platformer2DPlayerCombat.cpp
        src/spark/demo/Connect3Demo.cpp
        src/spark/demo/Tetris2DDemo.cpp
        src/spark/demo/SpaceInvaders2DDemo.cpp
        src/spark/demo/RenderLayers2DDemo.cpp
        src/spark/demo/TilemapShowcase2DDemo.cpp
        src/spark/demo/PhysicsBallThrow3DDemo.cpp
        src/spark/demo/Maze3DDemo.cpp
        src/spark/demo/SteeringShowcase3DDemo.cpp
        src/spark/demo/SceneEditor3DDemo.cpp
        src/spark/demo/ImGuiShowcaseDemo.cpp
)

set(SPARK_ENGINE_EDITOR_SOURCES
        src/spark/editor/EditorSelection.cpp
        src/spark/editor/EditorProject.cpp
        src/spark/editor/EditorUiFont.cpp
        src/spark/editor/EditorApplication.cpp
        src/spark/editor/panels/HierarchyPanel.cpp
        src/spark/editor/panels/InspectorPanel.cpp
        src/spark/editor/panels/ProjectBrowserPanel.cpp
        src/spark/editor/panels/EditorDockShell.cpp
)

set(SPARK_ENGINE_SOURCES
        src/Engine.cpp
        src/spark/engine/EngineContext.cpp
        src/spark/engine/GlfwInput.cpp
        src/spark/ecs/GameObject.cpp
        src/spark/ecs/GameObjectQuery.cpp
        src/spark/ecs/components/core/TransformComponent.cpp
        src/spark/ecs/components/rendering/MeshComponent.cpp
        src/spark/ecs/components/rendering/SpriteComponent.cpp
        src/spark/ecs/components/animation/SpriteAnimatorComponent.cpp
        src/spark/ecs/components/animation/Sprite2DCharacterAnimFsmComponent.cpp
        src/spark/ecs/components/animation/Character3DAnimFsmComponent.cpp
        src/spark/ecs/components/rendering/TilemapComponent.cpp
        src/spark/ecs/components/tilemap/TilemapGameplayGridComponent.cpp
        src/spark/ecs/components/tilemap/TilemapTileAnimatorComponent.cpp
        src/spark/ecs/components/tilemap/TilemapAutotileComponent.cpp
        src/spark/ecs/components/tilemap/TilemapObjectLayerComponent.cpp
        src/spark/ecs/components/tilemap/TilemapObjectSpawnComponent.cpp
        src/spark/ecs/components/tilemap/TilemapObjectGizmoComponent.cpp
        src/spark/ecs/components/tilemap/TilemapMapSourceComponent.cpp
        src/spark/scene/tilemap/TilemapObjectQuery.cpp
        src/spark/scene/tilemap/TilemapObjectSpawnRegistry.cpp
        src/spark/scene/tilemap/TilemapFileResolve.cpp
        src/spark/scene/tilemap/TmxImporter.cpp
        src/spark/scene/tilemap/TilemapDocumentApply.cpp
        src/spark/scene/tilemap/TilemapDocumentSerializer.cpp
        src/spark/scene/tilemap/Tileset.cpp
        src/spark/scene/tilemap/TilemapLayerSort.cpp
        src/spark/scene/tilemap/TilemapGameplayRules.cpp
        src/spark/scene/tilemap/TilemapGameplayGrid.cpp
        src/spark/scene/tilemap/TilemapGameplayGridBake.cpp
        src/spark/scene/tilemap/TilemapGridCoordinates.cpp
        src/spark/scene/tilemap/TileAnimationResolve.cpp
        src/spark/scene/tilemap/TileAutotileBake.cpp
        src/spark/ecs/components/rendering/MaterialComponent.cpp
        src/spark/ecs/components/lighting/PointLightComponent.cpp
        src/spark/ecs/components/lighting/DirectionalLightComponent.cpp
        src/spark/ecs/components/lighting/SpotLightComponent.cpp
        src/spark/ecs/components/camera/CameraComponent.cpp
        src/spark/ecs/components/camera/Camera2DComponent.cpp
        src/spark/ecs/components/camera/Camera2DRigComponent.cpp
        src/spark/ecs/components/camera/CameraFollow3DComponent.cpp
        src/spark/ecs/components/camera/SpringArm3DComponent.cpp
        src/spark/ecs/components/rendering/SkinnedMeshComponent.cpp
        src/spark/ecs/components/animation/AnimationEventReceiverComponent.cpp
        src/spark/ecs/components/animation/AttachmentSocketComponent.cpp
        src/spark/ecs/components/animation/AnimatorComponent.cpp
        src/spark/ecs/components/physics/CollisionComponent.cpp
        src/spark/ecs/components/ai/NavMeshAgentComponent.cpp
        src/spark/ecs/components/gameplay/HealthComponent.cpp
        src/spark/ecs/components/gameplay/DamageableComponent.cpp
        src/spark/ecs/components/physics/2d/PolygonCollider2DComponent.cpp
        src/spark/ecs/components/physics/3d/TriggerVolume3DComponent.cpp
        src/spark/animation/Skeleton.cpp
        src/spark/animation/LocomotionClipSet.cpp
        src/spark/scene/FlyCamera.cpp
        src/spark/scene/Camera2D.cpp
        src/spark/scene/Camera.cpp
        src/spark/scene/CharacterCameraRig.cpp
        src/spark/scene/GameWorld.cpp
        src/spark/scene/GameWorldAssetCache.cpp
        src/spark/scene/SpatialMath.cpp
        src/spark/scene/SceneSpatialCull.cpp
        src/spark/scene/SceneSpriteTileCull.cpp
        src/spark/scene/SceneTileAtlas.cpp
        src/spark/scene/SceneTilemapSubmit.cpp
        src/spark/scene/RenderLayerRegistry.cpp
        src/spark/scene/DrawableSortResolver.cpp
        src/spark/scene/mesh.cpp
        src/spark/scene/MeshRaycast.cpp
        src/spark/scene/SceneRaycast.cpp
        src/spark/scene/TerrainMeshGenerator.cpp
        src/spark/scene/mesh_gltf.cpp
        src/spark/scene/SkinnedMesh.cpp
        src/spark/scene/skinned_mesh_gltf.cpp
        src/spark/scene/cgltf_impl.cpp
        src/spark/scene/Texture2D.cpp
        src/spark/scene/stb_image_impl.cpp
        src/spark/text/Font.cpp
        src/spark/ecs/components/rendering/TextOverlayComponent.cpp
        src/spark/ecs/components/ui/GuiCanvasComponent.cpp
        src/spark/ecs/components/rendering/SkyComponent.cpp
        src/spark/ecs/components/rendering/BillboardComponent.cpp
        src/spark/ecs/components/rendering/ParticleEmitterComponent.cpp
        src/spark/ecs/components/rendering/TerrainComponent.cpp
        src/spark/gui/GuiInputState.cpp
        src/spark/gui/GuiLayoutMetrics.cpp
        src/spark/gui/GuiWidgetPopups.cpp
        src/spark/gui/GuiContextMenu.cpp
        src/spark/gui/TextEditShared.cpp
        src/spark/gui/EditorLayoutStore.cpp
        src/spark/gui/GuiPaintContext.cpp
        src/spark/gui/GuiSpriteSlice.cpp
        src/spark/gui/GuiTheme.cpp
        src/spark/gui/GuiThemeCatalog.cpp
        src/spark/gui/Widget.cpp
        src/spark/gui/RadioGroup.cpp
        src/spark/gui/controls/AlbumCard.cpp
        src/spark/gui/controls/Button.cpp
        src/spark/gui/controls/Carousel.cpp
        src/spark/gui/controls/CheckBox.cpp
        src/spark/gui/controls/Dialog.cpp
        src/spark/gui/controls/DockLayout.cpp
        src/spark/gui/controls/DockWorkspace.cpp
        src/spark/gui/DockLayoutModel.cpp
        src/spark/gui/DockLayoutStore.cpp
        src/spark/gui/docking/DockPanel.cpp
        src/spark/gui/docking/DockSidePane.cpp
        src/spark/gui/docking/DockFrameLayout.cpp
        src/spark/gui/docking/DockManager.cpp
        src/spark/gui/docking/DockLayoutState.cpp
        src/spark/gui/controls/Dropdown.cpp
        src/spark/gui/controls/EditorSidebarLayout.cpp
        src/spark/gui/controls/GridPanel.cpp
        src/spark/gui/controls/GroupBox.cpp
        src/spark/gui/controls/Image.cpp
        src/spark/gui/controls/Label.cpp
        src/spark/gui/controls/List.cpp
        src/spark/gui/controls/MenuBar.cpp
        src/spark/gui/controls/MultiSelectList.cpp
        src/spark/gui/controls/NumericBox.cpp
        src/spark/gui/controls/NumericStepper.cpp
        src/spark/gui/controls/Panel.cpp
        src/spark/gui/controls/ProgressBar.cpp
        src/spark/gui/controls/RadioButton.cpp
        src/spark/gui/controls/ScrollBar.cpp
        src/spark/gui/controls/ScrollPanel.cpp
        src/spark/gui/controls/VStackForm.cpp
        src/spark/gui/controls/Separator.cpp
        src/spark/gui/controls/Slider.cpp
        src/spark/gui/controls/Splitter.cpp
        src/spark/gui/controls/StackPanel.cpp
        src/spark/gui/controls/Switch.cpp
        src/spark/gui/controls/TabControl.cpp
        src/spark/gui/controls/TextBox.cpp
        src/spark/gui/controls/TextArea.cpp
        src/spark/gui/controls/TileSwatch.cpp
        src/spark/gui/controls/TreeView.cpp
        src/spark/gui/controls/WrapPanel.cpp
        src/spark/gui/controls/WrappingLabel.cpp
        src/spark/gui/GuiScene.cpp
        src/spark/gui/toolkit/GuiToolkitSettings.cpp
        src/spark/gui/api/GuiSystem.cpp
        src/spark/gui/api/SparkNativeGuiBackend.cpp
        src/spark/gui/api/SparkNativeImmediateGuiFrame.cpp
        src/spark/gui/api/DearImGuiGuiBackend.cpp
        src/spark/imgui/ImGuiLayerRegistry.cpp
        src/spark/imgui/ImGuiLayerFactory.cpp
        src/spark/render/platform/window.cpp
        src/spark/render/core/VulkanDeviceContext.cpp
        src/spark/render/core/VulkanFrameCapture.cpp
        src/spark/render/core/VulkanFrameSync.cpp
        src/spark/render/scene/VulkanSceneDescriptors.cpp
        src/spark/render/core/VulkanRenderer.cpp
        src/spark/render/shadow/VulkanDirectionalShadowCascadeMath.cpp
        src/spark/render/shadow/VulkanDirectionalShadowPass.cpp
        src/spark/render/shadow/VulkanPunctualShadowPass.cpp
        src/spark/render/shadow/VulkanShadowCastDraw.cpp
        src/spark/render/post/VulkanHdrTonemapPass.cpp
        src/spark/render/post/VulkanScreenSpaceEffectsPass.cpp
        src/spark/render/sprites2d/VulkanParticlePass.cpp
        src/spark/render/gpu/VulkanBlendAttachment.cpp
        src/spark/render/sprites2d/VulkanSpritePass.cpp
        src/spark/render/sprites2d/VulkanTilemapPass.cpp
        src/spark/render/sprites2d/Vulkan2DCompositePass.cpp
        src/spark/render/scene/VulkanSceneMeshDraw.cpp
        src/spark/render/scene/VulkanSceneOpaquePass.cpp
        src/spark/render/scene/VulkanScenePipeline.cpp
        src/spark/render/scene/VulkanSceneTextureUploader.cpp
        src/spark/render/scene/VulkanCustomMeshPool.cpp
        src/spark/render/scene/VulkanDeferredUploadBatch.cpp
        src/spark/render/scene/VulkanSceneUniformWriter.cpp
        src/spark/render/scene/VulkanClusteredForwardLights.cpp
        src/spark/render/capture/VulkanScreenshotCapture.cpp
        src/spark/render/capture/VulkanVideoCapture.cpp
        src/spark/render/capture/FrameCaptureWatermark.cpp
        src/spark/media/VideoRecorder.cpp
        src/spark/render/gpu/VulkanSpvShaderLoader.cpp
        src/spark/render/ui/VulkanScreenUiClip.cpp
        src/spark/render/ui/VulkanScreenUiPass.cpp
        src/spark/render/gpu/VulkanGpuInstance.cpp
        src/spark/render/gpu/VulkanGpuPhysicalDevice.cpp
        src/spark/render/gpu/VulkanGpuSwapchain.cpp
        src/spark/render/gpu/VulkanGpuBufferImage.cpp
        src/spark/render/gpu/VulkanGpuMeshInterleaved.cpp
        src/spark/render/present/VulkanPresentRenderPass.cpp
        src/spark/render/lighting/SceneLightingProfile.cpp
        src/spark/render/lighting/SceneLightingResolver.cpp
        src/spark/render/present/VulkanDepthResources.cpp
        src/spark/render/present/VulkanPresentationSwapchain.cpp
        src/spark/render/present/VulkanPresentationFramebuffers.cpp
        src/spark/scene/RenderVolumes.cpp
        src/spark/scene/VolumeRegions.cpp
        src/spark/scene/SceneSubmit.cpp
        src/spark/scene/SceneSubmitDrawPartition.cpp
        src/spark/scene/SceneSubmitLighting.cpp
        src/spark/scene/SceneSubmitMaterial.cpp
        src/spark/scene/serialization/ComponentSnapshotRegistry.cpp
        src/spark/scene/serialization/ComponentSnapshotHandlersExtended.cpp
        src/spark/scene/serialization/ComponentSnapshotHandlersMore.cpp
        src/spark/scene/serialization/ComponentSnapshotHandlersRendering.cpp
        src/spark/scene/serialization/SceneSerializer.cpp
        src/spark/scene/GameWorldAssetLoader.cpp
        src/spark/scene/SceneManager.cpp
        src/spark/physics/BroadPhase2D.cpp
        src/spark/physics/BroadPhase3D.cpp
        src/spark/physics/colliders/Collider2D.cpp
        src/spark/physics/colliders/Collider3D.cpp
        src/spark/physics/colliders/ColliderBakePipeline2D.cpp
        src/spark/physics/colliders/ColliderBakePipeline3D.cpp
        src/spark/physics/colliders/ColliderBakeStrategies2D.cpp
        src/spark/physics/colliders/ColliderBakeStrategies3D.cpp
        src/spark/physics/colliders/DynamicCollider2D.cpp
        src/spark/physics/colliders/DynamicCollider3D.cpp
        src/spark/physics/colliders/DynamicBody2D.cpp
        src/spark/physics/colliders/DynamicBody3D.cpp
        src/spark/physics/core/ColliderFilter.cpp
        src/spark/physics/core/ColliderMaterial.cpp
        src/spark/physics/shapes/BoxShape2D.cpp
        src/spark/physics/shapes/CircleShape2D.cpp
        src/spark/physics/shapes/ConvexPolygonShape2D.cpp
        src/spark/physics/shapes/ShapeContact2DDetail.cpp
        src/spark/physics/shapes/NarrowPhase2D.cpp
        src/spark/physics/shapes/ShapeFactory2D.cpp
        src/spark/physics/shapes/BoxShape3D.cpp
        src/spark/physics/shapes/SphereShape3D.cpp
        src/spark/physics/shapes/CapsuleShape3D.cpp
        src/spark/physics/shapes/ShapeContact3DDetail.cpp
        src/spark/physics/shapes/NarrowPhase3D.cpp
        src/spark/physics/shapes/ShapeFactory3D.cpp
        src/spark/physics/Collision2D.cpp
        src/spark/physics/TilemapCollider2D.cpp
        src/spark/physics/TilemapCollisionBake.cpp
        src/spark/physics/PhysicsMaterial2D.cpp
        src/spark/physics/MeshCollider3D.cpp
        src/spark/physics/PhysicsJoints.cpp
        src/spark/physics/PolygonCollider2D.cpp
        src/spark/physics/SpatialHashGrid2D.cpp
        src/spark/physics/PhysicsSubsystem.cpp
        src/spark/physics/PhysicsWorld2D.cpp
        src/spark/physics/simulation/ContactResolver2D.cpp
        src/spark/physics/simulation/ContactResolver3D.cpp
        src/spark/physics/simulation/JointSolver2D.cpp
        src/spark/physics/simulation/JointSolver3D.cpp
        src/spark/physics/simulation/RigidbodyIntegrator2D.cpp
        src/spark/physics/simulation/RigidbodyIntegrator3D.cpp
        src/spark/physics/simulation/SweptCcd3D.cpp
        src/spark/physics/simulation/TriggerDispatcher2D.cpp
        src/spark/physics/PhysicsQueries2D.cpp
        src/spark/physics/Collision3D.cpp
        src/spark/physics/SpatialHashGrid3D.cpp
        src/spark/physics/PhysicsWorld3D.cpp
        src/spark/physics/CharacterController3D.cpp
        src/spark/physics/TriggerVolume3D.cpp
        src/spark/ai/fsm/FiniteStateMachine.cpp
        src/spark/ai/steering/SteeringBehaviors.cpp
        src/spark/ai/steering/SteeringBehaviors3D.cpp
        src/spark/ai/path/GridPathfinder.cpp
        src/spark/ai/goap/GoapPlanner.cpp
        src/spark/ai/fuzzy/FuzzyLogic.cpp
        src/spark/ai/GameAiSubsystem.cpp
        src/spark/ai/NavigationSubsystem.cpp
        src/spark/ai/PerceptionSubsystem.cpp
        src/spark/ecs/components/ai/AiAgentComponent.cpp
        src/spark/ecs/components/audio/SoundCueComponent.cpp
        src/spark/audio/AmbientAudio.cpp
        src/spark/audio/AudioSpatial.cpp
        src/spark/audio/SoundClip.cpp
        src/spark/audio/WavDecoder.cpp
        src/spark/audio/SoundMixer.cpp
        src/spark/audio/SoundEngine.cpp
        src/spark/audio/SoundSubsystem.cpp
        src/spark/audio/SoundFileLoader.cpp
        src/spark/audio/DecodeOggStb.cpp
        src/spark/audio/DecodeMp3.cpp
)

if (APPLE)
    list(APPEND SPARK_ENGINE_SOURCES src/spark/audio/SoundOutputApple.cpp)
    list(APPEND SPARK_ENGINE_SOURCES src/spark/media/VideoRecorderMac.mm)
elseif (WIN32)
    list(APPEND SPARK_ENGINE_SOURCES src/spark/audio/SoundOutputWin32.cpp)
else ()
    list(APPEND SPARK_ENGINE_SOURCES src/spark/audio/SoundOutputNull.cpp)
endif ()

option(SPARK_BUILD_TESTS "Build Spark unit tests (GoogleTest)" OFF)
option(SPARK_BUILD_ENGINE_SHARED "Build SparkEngine as a shared library (OFF = static)" OFF)
option(SPARK_BUILD_ENGINE_DEMOS "Compile shell/demo sources into SparkEngine (OFF for slim editor/game DLL)" ON)
option(SPARK_BUILD_EDITOR "Compile spark/editor authoring module into SparkEngine" OFF)
option(SPARK_BUILD_DEMO "Build SparkDemo sample executable (shell + demos)" ON)
option(SPARK_BUILD_SCRIPT_HOST "Build SparkScriptHost + SparkInterop (CoreCLR + C# bindings)" ON)

if (SPARK_BUILD_ENGINE_DEMOS)
    list(APPEND SPARK_ENGINE_SOURCES ${SPARK_ENGINE_DEMO_SOURCES})
endif ()
if (SPARK_BUILD_EDITOR)
    list(APPEND SPARK_ENGINE_SOURCES ${SPARK_ENGINE_EDITOR_SOURCES})
endif ()

# Warn only on the full demo profile; editor-debug intentionally disables script host.
if (EXISTS "${CMAKE_SOURCE_DIR}/.run/SparkScriptHost.run.xml")
    if (NOT SPARK_BUILD_SCRIPT_HOST AND SPARK_BUILD_DEMO)
        message(WARNING
                "SPARK_BUILD_SCRIPT_HOST is OFF but .run/SparkScriptHost.run.xml exists. "
                "Set -DSPARK_BUILD_SCRIPT_HOST=ON in CMake options and Reload CMake Project.")
    endif ()
endif ()

if (SPARK_BUILD_ENGINE_SHARED)
    add_library(SparkEngine SHARED ${SPARK_ENGINE_SOURCES})
    if (WIN32)
        set_target_properties(SparkEngine PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif ()
else ()
    add_library(SparkEngine STATIC ${SPARK_ENGINE_SOURCES})
endif ()
set_target_properties(SparkEngine PROPERTIES POSITION_INDEPENDENT_CODE ON)
if (SPARK_BUILD_ENGINE_SHARED AND APPLE)
    set_target_properties(SparkEngine PROPERTIES MACOSX_RPATH ON)
endif ()
add_dependencies(SparkEngine SparkShaders)

target_include_directories(SparkEngine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        PRIVATE
        ${SPARK_STB_DIR}
        ${SPARK_AUDIO_CODEC_DIR}
        ${SPARK_CGLTF_DIR}
)

if (APPLE)
    target_compile_definitions(SparkEngine PUBLIC SPARK_PLATFORM_APPLE=1)
endif ()

target_link_libraries(SparkEngine PUBLIC
        glfw
        Vulkan::Vulkan
)

if (SPARK_ENABLE_IMGUI)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SparkImGui.cmake)
    spark_setup_imgui(SparkEngine)
else ()
    target_compile_definitions(SparkEngine PUBLIC SPARK_ENABLE_IMGUI=0)
endif ()

if (APPLE)
    target_link_libraries(SparkEngine PUBLIC
            "-framework AudioToolbox"
            "-framework AVFoundation"
            "-framework CoreMedia"
            "-framework CoreVideo"
            "-framework VideoToolbox")
endif ()

if (WIN32)
    target_link_libraries(SparkEngine PUBLIC ole32 uuid)
endif ()

if (SPARK_BUILD_DEMO)
    add_executable(SparkDemo src/main.cpp)
    target_link_libraries(SparkDemo PRIVATE SparkEngine)
endif ()

if (SPARK_BUILD_SCRIPT_HOST)
    add_subdirectory(scripting)
elseif (SPARK_BUILD_DEMO)
    message(STATUS "SparkScriptHost disabled (SPARK_BUILD_SCRIPT_HOST=OFF). "
            "Set -DSPARK_BUILD_SCRIPT_HOST=ON and Reload CMake for .run/SparkScriptHost.run.xml")
endif ()

if (SPARK_BUILD_TESTS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SparkTests.cmake)
    spark_add_physics_tests()
endif ()

# CLion / strict warnings
set(SPARK_WARNING_TARGETS SparkEngine)
if (SPARK_BUILD_DEMO)
    list(APPEND SPARK_WARNING_TARGETS SparkDemo)
endif ()
if (MSVC)
    foreach (_t IN LISTS SPARK_WARNING_TARGETS)
        target_compile_options(${_t} PRIVATE /W4 /permissive-)
    endforeach ()
else ()
    foreach (_t IN LISTS SPARK_WARNING_TARGETS)
        target_compile_options(${_t} PRIVATE
                -Wall -Wextra -Wpedantic
                -Wno-missing-field-initializers
        )
    endforeach ()
endif ()
