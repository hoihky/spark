# Locates nethost + hostfxr headers and libnethost from the installed .NET SDK host pack.

function (spark_find_dotnet_host out_include_dir out_nethost_lib)
    find_program(SPARK_DOTNET_EXECUTABLE dotnet)
    if (NOT SPARK_DOTNET_EXECUTABLE)
        message(FATAL_ERROR
                "dotnet not found on PATH (required when SPARK_BUILD_SCRIPT_HOST=ON)")
    endif ()

    # dotnet executable lives in <DOTNET_ROOT>/dotnet (not in sdk/.../dotnet).
    get_filename_component(_dotnet_root "${SPARK_DOTNET_EXECUTABLE}" DIRECTORY)
    if (NOT EXISTS "${_dotnet_root}/packs")
        get_filename_component(_dotnet_root "${_dotnet_root}" DIRECTORY)
    endif ()

    if (NOT EXISTS "${_dotnet_root}/packs")
        message(FATAL_ERROR
                "Could not find dotnet packs under ${_dotnet_root} (from ${SPARK_DOTNET_EXECUTABLE})")
    endif ()

    if (APPLE)
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(_rid "osx-arm64")
        else ()
            set(_rid "osx-x64")
        endif ()
        set(_nethost_name "libnethost.dylib")
    elseif (WIN32)
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
            set(_rid "win-arm64")
        else ()
            set(_rid "win-x64")
        endif ()
        set(_nethost_name "nethost.lib")
    else ()
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(_rid "linux-arm64")
        else ()
            set(_rid "linux-x64")
        endif ()
        set(_nethost_name "libnethost.so")
    endif ()

    # Modern SDK layout: packs/Microsoft.NETCore.App.Host.<rid>/<version>/runtimes/<rid>/native/
    set(_host_pack "${_dotnet_root}/packs/Microsoft.NETCore.App.Host.${_rid}")
    set(_native_dir "")

    if (EXISTS "${_host_pack}")
        file(GLOB _version_dirs RELATIVE "${_host_pack}" "${_host_pack}/*")
        list(SORT _version_dirs COMPARE NATURAL ORDER DESCENDING)
        foreach (_ver IN LISTS _version_dirs)
            set(_candidate "${_host_pack}/${_ver}/runtimes/${_rid}/native")
            if (EXISTS "${_candidate}/${_nethost_name}")
                set(_native_dir "${_candidate}")
                break ()
            endif ()
        endforeach ()
    endif ()

    # Legacy layout: packs/Microsoft.NETCore.App.Host.<major.minor.patch>/runtimes/<rid>/native/
    if (_native_dir STREQUAL "")
        file(GLOB _legacy_packs "${_dotnet_root}/packs/Microsoft.NETCore.App.Host.*")
        list(SORT _legacy_packs COMPARE NATURAL ORDER DESCENDING)
        foreach (_pack IN LISTS _legacy_packs)
            if (_pack MATCHES "\\.Host\\.${_rid}$")
                continue ()
            endif ()
            set(_candidate "${_pack}/runtimes/${_rid}/native")
            if (EXISTS "${_candidate}/${_nethost_name}")
                set(_native_dir "${_candidate}")
                break ()
            endif ()
            file(GLOB _ver_dirs RELATIVE "${_pack}" "${_pack}/*")
            list(SORT _ver_dirs COMPARE NATURAL ORDER DESCENDING)
            foreach (_ver IN LISTS _ver_dirs)
                set(_candidate "${_pack}/${_ver}/runtimes/${_rid}/native")
                if (EXISTS "${_candidate}/${_nethost_name}")
                    set(_native_dir "${_candidate}")
                    break ()
                endif ()
            endforeach ()
            if (NOT _native_dir STREQUAL "")
                break ()
            endif ()
        endforeach ()
    endif ()

    if (_native_dir STREQUAL "" OR NOT EXISTS "${_native_dir}/${_nethost_name}")
        message(FATAL_ERROR
                "nethost not found for RID '${_rid}' under ${_dotnet_root}/packs.\n"
                "Install the .NET SDK (includes Microsoft.NETCore.App.Host.${_rid}), e.g.:\n"
                "  brew install --cask dotnet-sdk\n"
                "or: https://dotnet.microsoft.com/download")
    endif ()

    if (NOT EXISTS "${_native_dir}/nethost.h")
        message(FATAL_ERROR "nethost.h not found in ${_native_dir}")
    endif ()

    set(${out_include_dir} "${_native_dir}" PARENT_SCOPE)
    set(${out_nethost_lib} "${_native_dir}/${_nethost_name}" PARENT_SCOPE)
    set(SPARK_DOTNET_ROOT "${_dotnet_root}" PARENT_SCOPE)
    message(STATUS "Spark scripting: nethost from ${_native_dir}")
endfunction ()
