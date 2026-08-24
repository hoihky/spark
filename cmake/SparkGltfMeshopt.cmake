# meshoptimizer decode for EXT_meshopt_compression / KHR_meshopt_compression.

function(spark_setup_gltf_meshopt target)
    option(SPARK_ENABLE_GLTF_MESHOPT "Decode EXT/KHR_meshopt_compression in glTF buffers" ON)
    if (NOT SPARK_ENABLE_GLTF_MESHOPT)
        target_compile_definitions(${target} PUBLIC SPARK_ENABLE_GLTF_MESHOPT=0)
        return()
    endif ()

    set(SPARK_MESHOPT_VERSION "v0.22")
    set(SPARK_MESHOPT_TGZ "${CMAKE_BINARY_DIR}/_deps/meshoptimizer-${SPARK_MESHOPT_VERSION}.tar.gz")
    set(SPARK_MESHOPT_SRC "${CMAKE_BINARY_DIR}/_deps/meshoptimizer-0.22")
    if (EXISTS "${CMAKE_BINARY_DIR}/_deps/meshoptimizer-src/CMakeLists.txt")
        set(SPARK_MESHOPT_SRC "${CMAKE_BINARY_DIR}/_deps/meshoptimizer-src")
    endif ()

    if (EXISTS "${CMAKE_BINARY_DIR}/_deps/meshoptimizer-src")
        if (NOT EXISTS "${CMAKE_BINARY_DIR}/_deps/meshoptimizer-src/CMakeLists.txt")
            message(STATUS "Removing incomplete meshoptimizer FetchContent checkout")
            file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/meshoptimizer-src")
        endif ()
    endif ()

    if (EXISTS "${SPARK_MESHOPT_TGZ}")
        file(SIZE "${SPARK_MESHOPT_TGZ}" _spark_meshopt_tgz_size)
        if (_spark_meshopt_tgz_size LESS 1000)
            file(REMOVE "${SPARK_MESHOPT_TGZ}")
        endif ()
    endif ()

    if (NOT EXISTS "${SPARK_MESHOPT_SRC}/CMakeLists.txt")
        if (NOT EXISTS "${SPARK_MESHOPT_TGZ}")
            message(STATUS "Downloading meshoptimizer ${SPARK_MESHOPT_VERSION}")
            file(DOWNLOAD
                    "https://github.com/zeux/meshoptimizer/archive/refs/tags/${SPARK_MESHOPT_VERSION}.tar.gz"
                    "${SPARK_MESHOPT_TGZ}"
                    TLS_VERIFY ON
                    STATUS _spark_meshopt_dl_status)
            list(GET _spark_meshopt_dl_status 0 _spark_meshopt_dl_rc)
            if (NOT _spark_meshopt_dl_rc EQUAL 0)
                file(REMOVE "${SPARK_MESHOPT_TGZ}")
            endif ()
        endif ()
        if (EXISTS "${SPARK_MESHOPT_TGZ}")
            message(STATUS "Extracting meshoptimizer ${SPARK_MESHOPT_VERSION}")
            execute_process(
                    COMMAND ${CMAKE_COMMAND} -E tar xzf "${SPARK_MESHOPT_TGZ}"
                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/_deps"
                    RESULT_VARIABLE _spark_meshopt_extract_rc)
            if (NOT _spark_meshopt_extract_rc EQUAL 0)
                file(REMOVE_RECURSE "${SPARK_MESHOPT_SRC}")
            endif ()
        endif ()
    endif ()

    if (NOT EXISTS "${SPARK_MESHOPT_SRC}/CMakeLists.txt")
        message(STATUS "Spark: meshoptimizer unavailable; meshopt compression disabled.")
        target_compile_definitions(${target} PUBLIC SPARK_ENABLE_GLTF_MESHOPT=0)
        return()
    endif ()

    add_subdirectory("${SPARK_MESHOPT_SRC}" "${CMAKE_BINARY_DIR}/_deps/spark_meshoptimizer-build")

    if (TARGET meshoptimizer::meshoptimizer)
        target_link_libraries(${target} PUBLIC meshoptimizer::meshoptimizer)
    else ()
        target_link_libraries(${target} PUBLIC meshoptimizer)
    endif ()

    target_compile_definitions(${target} PUBLIC SPARK_ENABLE_GLTF_MESHOPT=1)
endfunction()
