# Spark C ABI (SparkInterop) for managed editors and tools.
# CoreCLR game host (SparkScriptHost) is not built here — see SPARK_BUILD_SCRIPT_HOST (future).

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
    add_custom_target(SparkBindingsBuild
            COMMAND ${SPARK_DOTNET} build "${SPARK_BINDINGS_PROJECT}" -c Release
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Building Spark.Bindings (Release)"
    )
    add_dependencies(SparkBindingsBuild SparkInterop)
endif ()
