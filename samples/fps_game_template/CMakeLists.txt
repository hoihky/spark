cmake_minimum_required(VERSION 3.28)
project(SparkFpsTemplate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# samples/fps_game_template → engine repo root is two levels up.
if(NOT DEFINED SPARK_ROOT OR SPARK_ROOT STREQUAL "")
    set(SPARK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH
            "Path to the Spark engine repository (directory containing Spark's top-level CMakeLists.txt)")
endif()

get_filename_component(SPARK_ROOT_ABS "${SPARK_ROOT}" ABSOLUTE)
if(NOT EXISTS "${SPARK_ROOT_ABS}/CMakeLists.txt")
    message(FATAL_ERROR "SPARK_ROOT must point at the Spark engine repo (missing: ${SPARK_ROOT_ABS}/CMakeLists.txt)")
endif()

set(SPARK_BUILD_ENGINE_SHARED ON CACHE BOOL "Build SparkEngine as a shared library" FORCE)
set(SPARK_BUILD_DEMO OFF CACHE BOOL "Build SparkDemo (engine sample executable)" FORCE)
add_subdirectory("${SPARK_ROOT_ABS}" "${CMAKE_BINARY_DIR}/spark_engine")

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/FpsGame.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE SparkEngine)

set(_spark_out "${CMAKE_BINARY_DIR}/spark_engine")
if(APPLE OR UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_RPATH "${_spark_out}")
endif()

if(WIN32)
    add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:SparkEngine>"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copy SparkEngine DLL next to ${PROJECT_NAME}")
endif()
