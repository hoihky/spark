# Optional Draco decoder for KHR_draco_mesh_compression (glTF Phase 3).

function(spark_setup_gltf_draco target)
    option(SPARK_ENABLE_GLTF_DRACO "Decode KHR_draco_mesh_compression in glTF meshes" ON)
    if (NOT SPARK_ENABLE_GLTF_DRACO)
        target_compile_definitions(${target} PUBLIC SPARK_ENABLE_GLTF_DRACO=0)
        return()
    endif ()

    include(FetchContent)
    set(DRACO_TESTS OFF CACHE BOOL "" FORCE)
    set(DRACO_UNITY_PLUGIN OFF CACHE BOOL "" FORCE)
    set(DRACO_MAYA_PLUGIN OFF CACHE BOOL "" FORCE)
    set(DRACO_TRANSCODER OFF CACHE BOOL "" FORCE)
    set(DRACO_JS_GLUE OFF CACHE BOOL "" FORCE)
    set(DRACO_WASM OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
            draco
            GIT_REPOSITORY https://github.com/google/draco.git
            GIT_TAG 1.5.7
    )
    FetchContent_MakeAvailable(draco)

    set(DRACO_PLY_READER "${draco_SOURCE_DIR}/src/draco/io/ply_reader.cc")
    if (EXISTS "${DRACO_PLY_READER}")
        file(READ "${DRACO_PLY_READER}" DRACO_PLY_READER_CONTENT)
        if (NOT DRACO_PLY_READER_CONTENT MATCHES "#include <algorithm>")
            set(DRACO_PLY_READER_CONTENT "#include <algorithm>\n${DRACO_PLY_READER_CONTENT}")
            file(WRITE "${DRACO_PLY_READER}" "${DRACO_PLY_READER_CONTENT}")
        endif ()
    endif ()

    if (TARGET draco::draco)
        target_link_libraries(${target} PUBLIC draco::draco)
    else ()
        target_link_libraries(${target} PUBLIC draco_static)
    endif ()
    target_include_directories(${target} PUBLIC
            ${draco_SOURCE_DIR}/src
            ${CMAKE_BINARY_DIR})
    if (TARGET draco_static)
        add_dependencies(${target} draco_static)
    endif ()

    target_compile_definitions(${target} PUBLIC SPARK_ENABLE_GLTF_DRACO=1)
endfunction()
