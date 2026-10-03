# Spark C ABI (SparkInterop) + optional CoreCLR host (SparkScriptHost).

find_program(SPARK_PYTHON3 python3)
if (SPARK_PYTHON3)
    set(SPARK_INTEROP_BINDGEN_H "${CMAKE_SOURCE_DIR}/include/spark/scripting/SparkInteropComponentBindings.generated.h")
    set(SPARK_INTEROP_BINDGEN_CPP "${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropComponentBindings.generated.cpp")
    add_custom_command(
            OUTPUT "${SPARK_INTEROP_BINDGEN_H}" "${SPARK_INTEROP_BINDGEN_CPP}"
            COMMAND ${SPARK_PYTHON3} "${CMAKE_SOURCE_DIR}/tools/spark_script_bindgen.py"
            DEPENDS "${CMAKE_SOURCE_DIR}/tools/spark_script_bindgen.py"
            COMMENT "Generating SPARK_SCRIPT_BIND interop thunks"
            VERBATIM
    )
    add_custom_target(SparkInteropBindingsCodegen DEPENDS
            "${SPARK_INTEROP_BINDGEN_H}"
            "${SPARK_INTEROP_BINDGEN_CPP}"
    )
endif ()

set(SPARK_INTEROP_SOURCES
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInterop.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropInternal.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropMath.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropScene.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropPlatformer.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropPhysics2D.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropComponents.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropFoliage.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropComponentFactory.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropGameplay.cpp
        ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkInteropComponentBindings.generated.cpp
)

add_library(SparkInterop SHARED ${SPARK_INTEROP_SOURCES})
if (TARGET SparkInteropBindingsCodegen)
    add_dependencies(SparkInterop SparkInteropBindingsCodegen)
endif ()
target_compile_definitions(SparkInterop PRIVATE SPARK_INTEROP_EXPORTS)
target_include_directories(SparkInterop PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(SparkInterop PUBLIC SparkEngine)

if (APPLE)
    set_target_properties(SparkInterop PROPERTIES INSTALL_RPATH "@loader_path")
endif ()

find_program(SPARK_DOTNET dotnet)
if (SPARK_DOTNET)
    set(SPARK_BINDINGS_PROJECT "${CMAKE_SOURCE_DIR}/scripting/bindings/generated/Spark.Bindings/Spark.Bindings.csproj")
    set(SPARK_SCRIPTING_PROJECT "${CMAKE_SOURCE_DIR}/scripting/Spark.Scripting/Spark.Scripting.csproj")
    set(SPARK_HELLO_CS_GAME_PROJECT "${CMAKE_SOURCE_DIR}/scripting/samples/HelloCsGame/HelloCsGame.csproj")
    add_custom_target(SparkBindingsBuild
            COMMAND ${SPARK_DOTNET} build "${SPARK_BINDINGS_PROJECT}" -c Release -p:NuGetAudit=false
            COMMAND ${SPARK_DOTNET} build "${SPARK_SCRIPTING_PROJECT}" -c Release -p:NuGetAudit=false
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Building Spark.Bindings + Spark.Scripting (Release)"
    )
    add_dependencies(SparkBindingsBuild SparkInterop)

    add_custom_target(SparkScriptingBuild
            COMMAND ${SPARK_DOTNET} build "${SPARK_HELLO_CS_GAME_PROJECT}" -c Release -p:NuGetAudit=false
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Building HelloCsGame sample (Release)"
    )
    add_dependencies(SparkScriptingBuild SparkBindingsBuild)
endif ()

if (SPARK_BUILD_SCRIPT_HOST)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/DotNetHost.cmake)
    spark_find_dotnet_host(SPARK_DOTNET_HOST_INCLUDE SPARK_NETHOST_LIBRARY)

    add_executable(SparkScriptHost
            ${CMAKE_SOURCE_DIR}/src/spark/scripting/SparkScriptHost.cpp
            ${CMAKE_SOURCE_DIR}/src/spark/scripting/ManagedGameBridge.cpp
            ${CMAKE_SOURCE_DIR}/src/spark/scripting/CoreClrHost.cpp
            ${CMAKE_SOURCE_DIR}/src/spark/scripting/HostFxrLoader.cpp
    )
    target_include_directories(SparkScriptHost PRIVATE ${SPARK_DOTNET_HOST_INCLUDE})
    target_link_libraries(SparkScriptHost PRIVATE SparkEngine SparkInterop ${SPARK_NETHOST_LIBRARY})
    if (APPLE)
        set_target_properties(SparkScriptHost PROPERTIES BUILD_RPATH "@loader_path")
    endif ()
    if (TARGET SparkScriptingBuild)
        add_dependencies(SparkScriptHost SparkScriptingBuild)
    endif ()
endif ()
