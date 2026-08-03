# CMake configuration for Spark unit tests (Phase 0 physics foundation).

function(spark_add_physics_tests)
    include(FetchContent)

    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.15.2
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(SparkPhysicsTests
            tests/physics/Collision2DMathTest.cpp
            tests/physics/Collision3DMathTest.cpp
            tests/physics/ShapeNarrowPhaseTest.cpp
            tests/physics/ColliderSnapshotTest.cpp
            tests/physics/ColliderBakePipelineTest.cpp
            tests/physics/DynamicColliderTest.cpp
    )

    target_link_libraries(SparkPhysicsTests PRIVATE SparkEngine GTest::gtest_main)

    target_include_directories(SparkPhysicsTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_BINARY_DIR}/include
    )

    if (NOT MSVC)
        target_compile_options(SparkPhysicsTests PRIVATE
                -Wall -Wextra -Wpedantic
                -Wno-missing-field-initializers
        )
    endif ()

    include(GoogleTest)
    gtest_discover_tests(SparkPhysicsTests)
endfunction()
