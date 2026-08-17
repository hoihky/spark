option(SPARK_TEXTURE_COMPRESSION "Build BC7/ASTC encoders (used for KTX2 and optional runtime scene arrays)" ON)
option(SPARK_SCENE_RUNTIME_BLOCK_COMPRESSION
        "Use BC7/ASTC scene texture arrays at runtime (slow upload; prefer pre-baked KTX2)" OFF)

function(spark_setup_texture_compression target_name)
    if (NOT SPARK_TEXTURE_COMPRESSION)
        target_compile_definitions(${target_name} PRIVATE
                SPARK_TEXTURE_COMPRESSION=0
                SPARK_HAS_BC7ENC=0
                SPARK_HAS_ASTCENC=0
                SPARK_SCENE_RUNTIME_BLOCK_COMPRESSION=0)
        return()
    endif ()

    set(SPARK_BC7ENC_DIR "${CMAKE_BINARY_DIR}/_deps/spark_bc7enc")
    set(SPARK_BC7ENC_C "${SPARK_BC7ENC_DIR}/bc7enc.c")
    set(SPARK_BC7ENC_H "${SPARK_BC7ENC_DIR}/bc7enc.h")
    if (NOT EXISTS "${SPARK_BC7ENC_C}")
        file(MAKE_DIRECTORY "${SPARK_BC7ENC_DIR}")
        message(STATUS "Downloading bc7enc.c")
        file(DOWNLOAD
                "https://raw.githubusercontent.com/richgel999/bc7enc/master/bc7enc.c"
                "${SPARK_BC7ENC_C}"
                TLS_VERIFY ON)
    endif ()
    if (NOT EXISTS "${SPARK_BC7ENC_H}")
        file(MAKE_DIRECTORY "${SPARK_BC7ENC_DIR}")
        message(STATUS "Downloading bc7enc.h")
        file(DOWNLOAD
                "https://raw.githubusercontent.com/richgel999/bc7enc/master/bc7enc.h"
                "${SPARK_BC7ENC_H}"
                TLS_VERIFY ON)
    endif ()

    if (NOT TARGET spark_bc7enc)
        add_library(spark_bc7enc STATIC "${SPARK_BC7ENC_C}")
        target_include_directories(spark_bc7enc PUBLIC "${SPARK_BC7ENC_DIR}")
        if (MSVC)
            target_compile_options(spark_bc7enc PRIVATE /W0)
        else ()
            target_compile_options(spark_bc7enc PRIVATE -w)
        endif ()
    endif ()

    set(SPARK_ASTC_TGZ "${CMAKE_BINARY_DIR}/_deps/astc-encoder-5.0.1.tar.gz")
    set(SPARK_ASTC_SRC "${CMAKE_BINARY_DIR}/_deps/astc-encoder-5.0.1")
    set(SPARK_HAS_ASTCENC OFF)
    if (EXISTS "${SPARK_ASTC_TGZ}")
        file(SIZE "${SPARK_ASTC_TGZ}" _spark_astc_tgz_size)
        if (_spark_astc_tgz_size LESS 1000)
            file(REMOVE "${SPARK_ASTC_TGZ}")
        endif ()
    endif ()
    if (NOT EXISTS "${SPARK_ASTC_SRC}/CMakeLists.txt")
        if (NOT EXISTS "${SPARK_ASTC_TGZ}")
            message(STATUS "Downloading astc-encoder 5.0.1")
            file(DOWNLOAD
                    "https://github.com/ARM-software/astc-encoder/archive/refs/tags/5.0.1.tar.gz"
                    "${SPARK_ASTC_TGZ}"
                    TLS_VERIFY ON
                    STATUS _spark_astc_dl_status)
            list(GET _spark_astc_dl_status 0 _spark_astc_dl_rc)
            if (NOT _spark_astc_dl_rc EQUAL 0)
                file(REMOVE "${SPARK_ASTC_TGZ}")
            endif ()
        endif ()
        if (EXISTS "${SPARK_ASTC_TGZ}")
            message(STATUS "Extracting astc-encoder")
            execute_process(
                    COMMAND ${CMAKE_COMMAND} -E tar xzf "${SPARK_ASTC_TGZ}"
                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/_deps"
                    RESULT_VARIABLE _spark_astc_extract_rc)
            if (_spark_astc_extract_rc EQUAL 0 AND EXISTS "${SPARK_ASTC_SRC}/CMakeLists.txt")
                set(SPARK_HAS_ASTCENC ON)
            endif ()
        endif ()
    else ()
        set(SPARK_HAS_ASTCENC ON)
    endif ()
    if (NOT SPARK_HAS_ASTCENC)
        message(STATUS "Spark: astc-encoder unavailable; runtime ASTC encode disabled (BC7 / RGBA8 mips remain).")
    endif ()

    if (SPARK_HAS_ASTCENC)
        set(ASTCENC_ISA_NATIVE ON CACHE BOOL "" FORCE)
        set(ASTCENC_CLI OFF CACHE BOOL "" FORCE)
        set(ASTCENC_UNITTEST OFF CACHE BOOL "" FORCE)
        set(ASTCENC_DECOMPRESSOR OFF CACHE BOOL "" FORCE)
        add_subdirectory("${SPARK_ASTC_SRC}" "${CMAKE_BINARY_DIR}/_deps/spark_astc_encoder-build")
    endif ()

    target_compile_definitions(${target_name} PRIVATE SPARK_TEXTURE_COMPRESSION=1 SPARK_HAS_BC7ENC=1)
    if (SPARK_SCENE_RUNTIME_BLOCK_COMPRESSION)
        target_compile_definitions(${target_name} PRIVATE SPARK_SCENE_RUNTIME_BLOCK_COMPRESSION=1)
        message(STATUS "Spark: runtime BC7/ASTC scene texture arrays enabled (upload may stall).")
    else ()
        target_compile_definitions(${target_name} PRIVATE SPARK_SCENE_RUNTIME_BLOCK_COMPRESSION=0)
    endif ()
    target_include_directories(${target_name} PRIVATE "${SPARK_BC7ENC_DIR}")
    target_link_libraries(${target_name} PRIVATE spark_bc7enc)

    if (SPARK_HAS_ASTCENC)
        target_compile_definitions(${target_name} PRIVATE SPARK_HAS_ASTCENC=1)
        target_include_directories(${target_name} PRIVATE "${SPARK_ASTC_SRC}/Source")
        target_link_libraries(${target_name} PRIVATE astcenc-native-static)
    else ()
        target_compile_definitions(${target_name} PRIVATE SPARK_HAS_ASTCENC=0)
    endif ()
endfunction()
