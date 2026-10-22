# SparkDemo, SparkEditor, tests, and warning flags (after SparkEngine exists).

if (SPARK_BUILD_ENGINE_DEMOS)
    target_compile_definitions(SparkEngine PUBLIC SPARK_HAS_ENGINE_DEMOS=1)
else ()
    target_compile_definitions(SparkEngine PUBLIC SPARK_HAS_ENGINE_DEMOS=0)
endif ()

if (SPARK_BUILD_GAMEPLAY)
    target_compile_definitions(SparkEngine PUBLIC SPARK_HAS_GAMEPLAY=1)
else ()
    target_compile_definitions(SparkEngine PUBLIC SPARK_HAS_GAMEPLAY=0)
endif ()

if (SPARK_BUILD_GAMEPLAY)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SparkGameplay.cmake)
    spark_add_gameplay_library(SparkEngine)
endif ()

if (SPARK_BUILD_DEMO)
    add_executable(SparkDemo src/main.cpp)
    target_link_libraries(SparkDemo PRIVATE SparkEngine)
    if (SPARK_BUILD_GAMEPLAY)
        target_link_libraries(SparkDemo PRIVATE SparkGameplay)
    endif ()
endif ()

if (SPARK_BUILD_SPARK_EDITOR)
    add_subdirectory(spark_editor)
endif ()

if (SPARK_BUILD_TESTS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SparkTests.cmake)
    spark_add_physics_tests()
endif ()

set(SPARK_WARNING_TARGETS SparkEngine)
if (SPARK_BUILD_DEMO)
    list(APPEND SPARK_WARNING_TARGETS SparkDemo)
endif ()
if (SPARK_BUILD_SPARK_EDITOR)
    list(APPEND SPARK_WARNING_TARGETS SparkEditor)
endif ()
if (SPARK_BUILD_GAMEPLAY)
    list(APPEND SPARK_WARNING_TARGETS SparkGameplay)
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
