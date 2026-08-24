# meshoptimizer decode for EXT_meshopt_compression / KHR_meshopt_compression.

function(spark_setup_gltf_meshopt target)
    option(SPARK_ENABLE_GLTF_MESHOPT "Decode EXT/KHR_meshopt_compression in glTF buffers" ON)
    if (NOT SPARK_ENABLE_GLTF_MESHOPT)
        target_compile_definitions(${target} PUBLIC SPARK_ENABLE_GLTF_MESHOPT=0)
        return()
    endif ()

    include(FetchContent)
    FetchContent_Declare(
            meshoptimizer
            GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
            GIT_TAG v0.22
    )
    FetchContent_MakeAvailable(meshoptimizer)

    if (TARGET meshoptimizer::meshoptimizer)
        target_link_libraries(${target} PUBLIC meshoptimizer::meshoptimizer)
    else ()
        target_link_libraries(${target} PUBLIC meshoptimizer)
    endif ()

    target_compile_definitions(${target} PUBLIC SPARK_ENABLE_GLTF_MESHOPT=1)
endfunction()
