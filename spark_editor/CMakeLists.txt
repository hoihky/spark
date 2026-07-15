cmake_minimum_required(VERSION 3.28)

# Standalone: open spark_editor/ in CLion. Embedded: add_subdirectory from engine root.
set(_SPARK_EDITOR_STANDALONE FALSE)
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_SPARK_EDITOR_STANDALONE TRUE)
endif ()

if (_SPARK_EDITOR_STANDALONE)
    project(SparkEditorProject LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    if (NOT DEFINED SPARK_ROOT OR SPARK_ROOT STREQUAL "")
        set(SPARK_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." CACHE PATH
                "Path to the Spark engine repository")
    endif ()

    get_filename_component(SPARK_ROOT_ABS "${SPARK_ROOT}" ABSOLUTE)
    if (NOT EXISTS "${SPARK_ROOT_ABS}/CMakeLists.txt")
        message(FATAL_ERROR
                "SPARK_ROOT must point at the Spark engine repo (missing: ${SPARK_ROOT_ABS}/CMakeLists.txt)")
    endif ()

    # Slim shared engine: no shell demos, include spark/editor module.
    set(SPARK_BUILD_ENGINE_SHARED ON CACHE BOOL "Build SparkEngine as a shared library" FORCE)
    set(SPARK_BUILD_ENGINE_DEMOS OFF CACHE BOOL "Omit demo sources from SparkEngine" FORCE)
    set(SPARK_BUILD_EDITOR ON CACHE BOOL "Compile spark/editor into SparkEngine" FORCE)
    set(SPARK_BUILD_DEMO OFF CACHE BOOL "Do not build SparkDemo from engine tree" FORCE)
    set(SPARK_BUILD_SCRIPT_HOST OFF CACHE BOOL "Editor build skips script host by default" FORCE)

    add_subdirectory("${SPARK_ROOT_ABS}" "${CMAKE_BINARY_DIR}/spark_engine")
endif ()

if (NOT TARGET SparkEngine)
    message(FATAL_ERROR
            "SparkEditor requires SparkEngine. Build from spark_editor/ standalone, "
            "or enable -DSPARK_BUILD_SPARK_EDITOR=ON in the engine root CMake profile.")
endif ()

if (NOT TARGET SparkEditor)
    add_executable(SparkEditor
            src/main.cpp
            src/EditorGame.cpp)

    target_include_directories(SparkEditor PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include")

    target_link_libraries(SparkEditor PRIVATE SparkEngine)

    if (_SPARK_EDITOR_STANDALONE)
        set(_spark_engine_out "${CMAKE_BINARY_DIR}/spark_engine")
    else ()
        set(_spark_engine_out "${CMAKE_BINARY_DIR}")
    endif ()

    if (APPLE OR UNIX)
        set_target_properties(SparkEditor PROPERTIES BUILD_RPATH "${_spark_engine_out}")
    endif ()

    if (WIN32)
        add_custom_command(
                TARGET SparkEditor
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:SparkEngine>"
                        "$<TARGET_FILE_DIR:SparkEditor>"
                COMMENT "Copy SparkEngine DLL next to SparkEditor")
    endif ()
endif ()
