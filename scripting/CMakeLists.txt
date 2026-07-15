# C# scripting: SparkInterop (C ABI) + SparkScriptHost (nethost/hostfxr CoreCLR host)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/DotNetHost.cmake)
spark_find_dotnet_host(SPARK_DOTNET_HOST_INCLUDE SPARK_NETHOST_LIBRARY)

set(SPARK_SCRIPTING_SOURCES
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInterop.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropInternal.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropMath.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropScene.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropPlatformer.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropPhysics2D.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropComponents.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/ManagedGameBridge.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/CoreClrHost.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/HostFxrLoader.cpp
)

add_library(SparkInterop SHARED ${SPARK_SCRIPTING_SOURCES})
target_compile_definitions(SparkInterop PRIVATE SPARK_INTEROP_EXPORTS)
target_include_directories(SparkInterop
        PUBLIC ${CMAKE_SOURCE_DIR}/include
        PRIVATE ${SPARK_DOTNET_HOST_INCLUDE}
)
target_link_libraries(SparkInterop PUBLIC SparkEngine PRIVATE ${SPARK_NETHOST_LIBRARY})
if (APPLE)
    set_target_properties(SparkInterop PROPERTIES INSTALL_RPATH "@loader_path")
endif ()

add_executable(SparkScriptHost ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkScriptHost.cpp)
target_link_libraries(SparkScriptHost PRIVATE SparkEngine SparkInterop)
if (APPLE)
    set_target_properties(SparkScriptHost PROPERTIES BUILD_RPATH "@loader_path")
endif ()

# Managed sample + SDK (optional; requires dotnet on PATH)
find_program(SPARK_DOTNET dotnet)
if (SPARK_DOTNET)
    set(SPARK_SCRIPTING_DIR "${CMAKE_SOURCE_DIR}/scripting")
    add_custom_target(SparkScriptingBuild
            COMMAND ${SPARK_DOTNET} build "${SPARK_SCRIPTING_DIR}/sdk/Spark.Scripting/Spark.Scripting.csproj" -c Release
            COMMAND ${SPARK_DOTNET} build "${SPARK_SCRIPTING_DIR}/samples/HelloCsGame/HelloCsGame.csproj" -c Release
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Building Spark.Scripting + HelloCsGame (Release, emits .runtimeconfig.json)"
    )
    add_dependencies(SparkScriptHost SparkScriptingBuild)
endif ()
