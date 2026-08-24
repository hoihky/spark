# Optional Draco decoder for KHR_draco_mesh_compression (glTF Phase 3).

function(spark_setup_gltf_draco target)
    option(SPARK_ENABLE_GLTF_DRACO "Decode KHR_draco_mesh_compression in glTF meshes" ON)
    if (NOT SPARK_ENABLE_GLTF_DRACO)
        target_compile_definitions(${target} PUBLIC SPARK_ENABLE_GLTF_DRACO=0)
        return()
    endif ()

    set(SPARK_DRACO_VERSION "1.5.7")
    set(SPARK_DRACO_TGZ "${CMAKE_BINARY_DIR}/_deps/draco-${SPARK_DRACO_VERSION}.tar.gz")
    set(SPARK_DRACO_SRC "${CMAKE_BINARY_DIR}/_deps/draco-${SPARK_DRACO_VERSION}")

    # Remove stale FetchContent git checkout (empty .git-only dirs hang configure).
    if (EXISTS "${CMAKE_BINARY_DIR}/_deps/draco-src")
        if (NOT EXISTS "${CMAKE_BINARY_DIR}/_deps/draco-src/CMakeLists.txt")
            message(STATUS "Removing incomplete draco FetchContent checkout")
            file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/draco-src")
        endif ()
    endif ()

    if (EXISTS "${SPARK_DRACO_TGZ}")
        file(SIZE "${SPARK_DRACO_TGZ}" _spark_draco_tgz_size)
        if (_spark_draco_tgz_size LESS 1000)
            file(REMOVE "${SPARK_DRACO_TGZ}")
        endif ()
    endif ()

    if (NOT EXISTS "${SPARK_DRACO_SRC}/CMakeLists.txt")
        if (NOT EXISTS "${SPARK_DRACO_TGZ}")
            message(STATUS "Downloading draco ${SPARK_DRACO_VERSION}")
            file(DOWNLOAD
                    "https://github.com/google/draco/archive/refs/tags/${SPARK_DRACO_VERSION}.tar.gz"
                    "${SPARK_DRACO_TGZ}"
                    TLS_VERIFY ON
                    STATUS _spark_draco_dl_status)
            list(GET _spark_draco_dl_status 0 _spark_draco_dl_rc)
            if (NOT _spark_draco_dl_rc EQUAL 0)
                file(REMOVE "${SPARK_DRACO_TGZ}")
            endif ()
        endif ()
        if (EXISTS "${SPARK_DRACO_TGZ}")
            message(STATUS "Extracting draco ${SPARK_DRACO_VERSION}")
            execute_process(
                    COMMAND ${CMAKE_COMMAND} -E tar xzf "${SPARK_DRACO_TGZ}"
                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/_deps"
                    RESULT_VARIABLE _spark_draco_extract_rc)
            if (NOT _spark_draco_extract_rc EQUAL 0)
                file(REMOVE_RECURSE "${SPARK_DRACO_SRC}")
            endif ()
        endif ()
    endif ()

    if (NOT EXISTS "${SPARK_DRACO_SRC}/CMakeLists.txt")
        message(STATUS "Spark: draco unavailable; KHR_draco_mesh_compression disabled.")
        target_compile_definitions(${target} PUBLIC SPARK_ENABLE_GLTF_DRACO=0)
        return()
    endif ()

    set(DRACO_TESTS OFF CACHE BOOL "" FORCE)
    set(DRACO_UNITY_PLUGIN OFF CACHE BOOL "" FORCE)
    set(DRACO_MAYA_PLUGIN OFF CACHE BOOL "" FORCE)
    set(DRACO_TRANSCODER OFF CACHE BOOL "" FORCE)
    set(DRACO_JS_GLUE OFF CACHE BOOL "" FORCE)
    set(DRACO_WASM OFF CACHE BOOL "" FORCE)

    add_subdirectory("${SPARK_DRACO_SRC}" "${CMAKE_BINARY_DIR}/_deps/spark_draco-build")

    set(DRACO_PLY_READER "${SPARK_DRACO_SRC}/src/draco/io/ply_reader.cc")
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
            ${SPARK_DRACO_SRC}/src
            ${CMAKE_BINARY_DIR})
    if (TARGET draco_static)
        add_dependencies(${target} draco_static)
    endif ()

    target_compile_definitions(${target} PUBLIC SPARK_ENABLE_GLTF_DRACO=1)
endfunction()
